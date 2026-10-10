#pragma once
#include <juce_graphics/juce_graphics.h>
#include <juce_core/juce_core.h>
#include <algorithm>
#include <atomic>
#include <cmath>

// JUCE 7 / JUCE 8 font construction compatibility
inline juce::Font vizFontNamed(const juce::String& name, float h, int style)
{
   #if JUCE_MAJOR_VERSION >= 8
    return juce::Font(juce::FontOptions(name, h, style));
   #else
    return juce::Font(name, h, style);
   #endif
}

inline juce::Font vizFontFromFace(const juce::Typeface::Ptr& tf, float h)
{
   #if JUCE_MAJOR_VERSION >= 8
    return juce::Font(juce::FontOptions(tf).withHeight(h));
   #else
    juce::Font f(tf); f.setHeight(h); return f;
   #endif
}

struct TextLine
{
    double start = -1.0;   // seconds; < 0 means untimed
    double end   = -1.0;   // seconds; < 0 means "until next line"
    juce::String text;
};

// Holds the lyrics / text lines and the chosen font. Thread-safe (message thread edits, GL thread reads).
class TextEngine
{
public:
    // Accepts: plain text (one slide per line, use | for a line break inside a slide),
    //          LRC  ([mm:ss.xx] text),  SRT (numbered blocks with --> timestamps)
    void setRawText(const juce::String& s)
    {
        const juce::ScopedLock sl(cs);
        raw = s;
        parse();
        ++textRevision;
    }

    juce::String getRawText() const { const juce::ScopedLock sl(cs); return raw; }
    int  getNumLines() const { const juce::ScopedLock sl(cs); return lines.size(); }
    bool isTimed() const { const juce::ScopedLock sl(cs); return timed; }
    int  getTextRevision() const { return textRevision.load(); }

    juce::String getLineText(int idx) const
    {
        const juce::ScopedLock sl(cs);
        return juce::isPositiveAndBelow(idx, lines.size()) ? lines.getReference(idx).text : juce::String();
    }

    // manual > 0 : fixed line (1-based). Else timed lyrics follow `clock`; else advance every
    // `beatsPerLine` beats (using beatPos); else show the first line.
    int pickLine(int manual, double clock, double beatPos, int beatsPerLine) const
    {
        const juce::ScopedLock sl(cs);
        const int n = lines.size();
        if (n == 0) return -1;
        if (manual > 0) return juce::jmin(manual - 1, n - 1);
        if (timed)
        {
            int idx = -1;
            for (int i = 0; i < n; ++i)
            {
                if (lines.getReference(i).start <= clock) idx = i; else break;
            }
            if (idx >= 0 && lines.getReference(idx).end >= 0.0 && clock > lines.getReference(idx).end) return -1;
            return idx;
        }
        if (beatsPerLine > 0)
        {
            const int k = (int) std::floor(beatPos / (double) beatsPerLine);
            return ((k % n) + n) % n;
        }
        return 0;
    }

    // ---- font ---------------------------------------------------------------
    void setFont(const juce::String& key, juce::Typeface::Ptr face, const juce::String& systemName)
    {
        const juce::ScopedLock sl(cs);
        fontKey = key; customFace = face; sysName = systemName;
        ++fontRevision;
    }
    juce::String getFontKey() const { const juce::ScopedLock sl(cs); return fontKey; }
    int getFontRevision() const { return fontRevision.load(); }

    juce::Font makeFont(float h) const
    {
        const juce::ScopedLock sl(cs);
        if (customFace != nullptr) return vizFontFromFace(customFace, h);
        if (sysName.isNotEmpty()) return vizFontNamed(sysName, h, juce::Font::plain);
        return vizFontNamed(juce::Font::getDefaultSansSerifFontName(), h, juce::Font::bold);
    }

    // White text on transparent background. The shader colours it and adds glow/shadow.
    juce::Image renderImage(const juce::String& text, float fontPx = 128.0f, int maxWidth = 2000) const
    {
        juce::Font f = makeFont(fontPx);
        juce::GlyphArrangement ga;
        ga.addJustifiedText(f, text, 0.0f, fontPx, (float) maxWidth, juce::Justification::centred);
        const auto b = ga.getBoundingBox(0, -1, true);
        const int pad = 28;
        const int w = juce::jlimit(8, 4096, (int) std::ceil(b.getWidth()) + pad * 2);
        const int h = juce::jlimit(8, 4096, (int) std::ceil(b.getHeight()) + pad * 2);
        juce::Image img(juce::Image::ARGB, w, h, true);
        ga.moveRangeOfGlyphs(0, -1, (float) pad - b.getX(), (float) pad - b.getY());
        juce::Graphics g(img);
        g.setColour(juce::Colours::white);
        ga.draw(g);
        return img;
    }

private:
    static double parseTimestamp(juce::String s)
    {
        s = s.trim().replaceCharacter(',', '.');
        auto parts = juce::StringArray::fromTokens(s, ":", "");
        double t = 0.0;
        for (auto& p : parts) t = t * 60.0 + p.getDoubleValue();
        return t;
    }

    void parse()
    {
        lines.clear();
        timed = false;
        const auto rows = juce::StringArray::fromLines(raw);

        // ---- SRT ----
        bool isSrt = false;
        for (auto& r : rows) if (r.contains("-->")) { isSrt = true; break; }
        if (isSrt)
        {
            for (int i = 0; i < rows.size(); ++i)
            {
                if (!rows[i].contains("-->")) continue;
                TextLine tl;
                tl.start = parseTimestamp(rows[i].upToFirstOccurrenceOf("-->", false, false));
                const auto after = rows[i].fromFirstOccurrenceOf("-->", false, false).trim();
                tl.end = parseTimestamp(after.upToFirstOccurrenceOf(" ", false, false));   // ignores SRT position tags
                juce::StringArray body;
                for (int j = i + 1; j < rows.size() && rows[j].trim().isNotEmpty(); ++j) body.add(rows[j].trim());
                tl.text = body.joinIntoString("\n");
                if (tl.text.isNotEmpty()) lines.add(tl);
            }
            timed = !lines.isEmpty();
            sortLines();
            return;
        }

        // ---- LRC ----
        bool isLrc = false;
        for (auto& r : rows)
        {
            auto t = r.trim();
            if (t.startsWithChar('[') && t.length() > 3 && juce::CharacterFunctions::isDigit(t[1])) { isLrc = true; break; }
        }
        if (isLrc)
        {
            for (auto r : rows)
            {
                r = r.trim();
                juce::Array<double> stamps;
                while (r.startsWithChar('['))
                {
                    const int close = r.indexOfChar(']');
                    if (close < 0) break;
                    const auto tag = r.substring(1, close);
                    if (tag.isNotEmpty() && juce::CharacterFunctions::isDigit(tag[0]) && tag.contains(":"))
                        stamps.add(parseTimestamp(tag));
                    r = r.substring(close + 1).trimStart();
                }
                if (r.isNotEmpty())
                    for (auto st : stamps) { TextLine tl; tl.start = st; tl.text = r.replace("|", "\n"); lines.add(tl); }
            }
            timed = !lines.isEmpty();
            sortLines();
            return;
        }

        // ---- plain text: one slide per non-empty line ----
        for (auto r : rows)
        {
            r = r.trim();
            if (r.isNotEmpty()) { TextLine tl; tl.text = r.replace("|", "\n"); lines.add(tl); }
        }
    }

    void sortLines()
    {
        std::stable_sort(lines.begin(), lines.end(), [](const TextLine& a, const TextLine& b) { return a.start < b.start; });
    }

    mutable juce::CriticalSection cs;
    juce::String raw;
    juce::Array<TextLine> lines;
    bool timed = false;
    juce::String fontKey, sysName;
    juce::Typeface::Ptr customFace;
    std::atomic<int> textRevision { 0 }, fontRevision { 0 };
};
