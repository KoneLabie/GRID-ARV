#pragma once

#include <juce_graphics/juce_graphics.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

// Small animated-GIF frame extractor built on JUCE's GIF decoder. JUCE's normal
// ImageFileFormat::loadFrom() returns only the first GIF frame, so each image
// block is wrapped as a one-frame GIF and composited onto the logical canvas.
// Decoding happens only when an image asset is changed, not on every render tick.
namespace GridARVGif
{
struct Frame
{
    juce::Image image;
    int delayMs = 100;
};

struct LoadResult
{
    // The worker publishes the frame vector before setting finished with release
    // semantics; the OpenGL thread reads it only after an acquire load.
    std::atomic<bool> finished { false };
    bool success = false;
    std::vector<Frame> frames;
};

struct RawFrame
{
    std::vector<std::uint8_t> singleFrameGif;
    int x = 0, y = 0, width = 0, height = 0;
    int delayMs = 100;
    int disposal = 0;
    bool hasTransparency = false;
    int transparentIndex = -1;
};

inline std::uint16_t readLE16 (const std::uint8_t* p)
{
    return static_cast<std::uint16_t> (p[0] | (static_cast<unsigned int> (p[1]) << 8));
}

inline void writeLE16 (std::vector<std::uint8_t>& bytes, std::size_t offset, int value)
{
    bytes[offset] = static_cast<std::uint8_t> (value & 0xff);
    bytes[offset + 1] = static_cast<std::uint8_t> ((value >> 8) & 0xff);
}

inline bool readSubBlocks (const std::uint8_t* data, std::size_t size, std::size_t& pos)
{
    while (pos < size)
    {
        const auto count = static_cast<std::size_t> (data[pos++]);
        if (count == 0)
            return true;
        if (count > size - pos)
            return false;
        pos += count;
    }
    return false;
}

inline bool parse (const std::uint8_t* data, std::size_t size,
                   std::vector<RawFrame>& frames,
                   int& canvasWidth, int& canvasHeight,
                   juce::Colour& backgroundColour)
{
    frames.clear();
    if (data == nullptr || size < 13 || size > 64u * 1024u * 1024u)
        return false;
    if (std::memcmp (data, "GIF87a", 6) != 0 && std::memcmp (data, "GIF89a", 6) != 0)
        return false;

    canvasWidth = static_cast<int> (readLE16 (data + 6));
    canvasHeight = static_cast<int> (readLE16 (data + 8));
    const auto screenPacked = data[10];
    const int backgroundIndex = data[11];
    if (canvasWidth <= 0 || canvasHeight <= 0 || canvasWidth > 8192 || canvasHeight > 8192
        || static_cast<std::uint64_t> (canvasWidth) * static_cast<std::uint64_t> (canvasHeight) > 16u * 1024u * 1024u)
        return false;

    const bool hasGlobalPalette = (screenPacked & 0x80u) != 0;
    const std::size_t globalPaletteBytes = hasGlobalPalette
        ? static_cast<std::size_t> (3u * (2u << (screenPacked & 7u))) : 0u;
    if (13u + globalPaletteBytes > size)
        return false;
    const std::size_t blocksStart = 13u + globalPaletteBytes;

    if (hasGlobalPalette && backgroundIndex < (2 << (screenPacked & 7u)))
    {
        const auto* rgb = data + 13u + static_cast<std::size_t> (backgroundIndex) * 3u;
        backgroundColour = juce::Colour (0xff000000u | (static_cast<std::uint32_t> (rgb[0]) << 16)
                                          | (static_cast<std::uint32_t> (rgb[1]) << 8) | rgb[2]);
    }
    else
        backgroundColour = juce::Colours::transparentBlack;

    std::size_t pos = blocksStart;
    std::vector<std::uint8_t> currentGce;
    int currentDelayMs = 100;
    int currentDisposal = 0;
    bool currentHasTransparency = false;
    int currentTransparentIndex = -1;

    while (pos < size)
    {
        const std::size_t blockStart = pos;
        const std::uint8_t marker = data[pos++];
        if (marker == 0x3b) // trailer
            break;

        if (marker == 0x21) // extension
        {
            if (pos >= size)
                return false;
            const std::uint8_t label = data[pos++];
            if (label == 0xf9) // Graphic Control Extension
            {
                if (pos >= size)
                    return false;
                const std::size_t payloadSize = data[pos++];
                if (payloadSize < 4 || payloadSize > size - pos)
                    return false;
                const std::uint8_t gcePacked = data[pos];
                const int delayHundredths = static_cast<int> (readLE16 (data + pos + 1));
                const int transparentIndex = data[pos + 3];
                pos += payloadSize;
                if (pos >= size || data[pos] != 0)
                    return false;
                ++pos;

                currentGce.assign (data + blockStart, data + pos);
                currentDelayMs = delayHundredths <= 0 ? 100 : juce::jlimit (20, 2000, delayHundredths * 10);
                currentDisposal = (gcePacked >> 2) & 7;
                currentHasTransparency = (gcePacked & 1u) != 0;
                currentTransparentIndex = currentHasTransparency ? transparentIndex : -1;
            }
            else
            {
                // All non-GCE extension payloads are sequences of sub-blocks.
                if (! readSubBlocks (data, size, pos))
                    return false;
            }
            continue;
        }

        if (marker != 0x2c) // image descriptor
            return false;
        if (pos + 9 > size)
            return false;

        const int left = static_cast<int> (readLE16 (data + pos));
        const int top = static_cast<int> (readLE16 (data + pos + 2));
        const int frameWidth = static_cast<int> (readLE16 (data + pos + 4));
        const int frameHeight = static_cast<int> (readLE16 (data + pos + 6));
        const std::uint8_t imagePacked = data[pos + 8];
        if (frameWidth <= 0 || frameHeight <= 0 || left < 0 || top < 0
            || left + frameWidth > canvasWidth || top + frameHeight > canvasHeight)
            return false;

        pos += 9;
        if ((imagePacked & 0x80u) != 0)
        {
            const std::size_t localPaletteBytes = static_cast<std::size_t> (3u * (2u << (imagePacked & 7u)));
            if (localPaletteBytes > size - pos)
                return false;
            pos += localPaletteBytes;
        }
        if (pos >= size) // LZW minimum code size
            return false;
        ++pos;
        if (! readSubBlocks (data, size, pos))
            return false;

        RawFrame frame;
        frame.x = left;
        frame.y = top;
        frame.width = frameWidth;
        frame.height = frameHeight;
        frame.delayMs = currentDelayMs;
        frame.disposal = currentDisposal;
        frame.hasTransparency = currentHasTransparency;
        frame.transparentIndex = currentTransparentIndex;

        // The JUCE GIF decoder renders the first image block in a file. Wrap
        // this block in a miniature GIF with a canvas matching this frame rect.
        auto& mini = frame.singleFrameGif;
        mini.insert (mini.end(), data, data + 13);
        writeLE16 (mini, 6, frameWidth);
        writeLE16 (mini, 8, frameHeight);
        if (globalPaletteBytes > 0)
            mini.insert (mini.end(), data + 13, data + blocksStart);
        if (! currentGce.empty())
            mini.insert (mini.end(), currentGce.begin(), currentGce.end());
        const std::size_t miniFrameStart = mini.size();
        mini.insert (mini.end(), data + blockStart, data + pos);
        // image descriptor follows the comma; make its left/top coordinates 0.
        writeLE16 (mini, miniFrameStart + 1, 0);
        writeLE16 (mini, miniFrameStart + 3, 0);
        mini.push_back (0x3b);
        frames.push_back (std::move (frame));
        if (frames.size() > 2000) // maliciously long animations are rejected
            return false;

        currentGce.clear();
        currentDelayMs = 100;
        currentDisposal = 0;
        currentHasTransparency = false;
        currentTransparentIndex = -1;
    }

    return ! frames.empty();
}

inline bool load (const juce::File& file, std::vector<Frame>& outputFrames)
{
    outputFrames.clear();
    // Reject oversized assets before allocating a MemoryBlock for the whole file.
    if (! file.existsAsFile() || file.getSize() <= 0 || file.getSize() > 64LL * 1024LL * 1024LL)
        return false;
    juce::MemoryBlock data;
    if (! file.loadFileAsData (data))
        return false;

    const auto* bytes = static_cast<const std::uint8_t*> (data.getData());
    std::vector<RawFrame> rawFrames;
    int width = 0, height = 0;
    juce::Colour backgroundColour;
    if (! parse (bytes, data.getSize(), rawFrames, width, height, backgroundColour))
        return false;

    // Keep decoded memory bounded even for GIFs with many large frames. The
    // output size is selected once, and all frame resampling is done off the
    // render loop when the asset changes.
    constexpr std::size_t maxStoredPixels = 48u * 1024u * 1024u;
    constexpr int maxOutputSide = 2048;
    constexpr std::size_t maxOutputFrames = 240;
    const double frameStride = std::max (1.0, std::ceil (static_cast<double> (rawFrames.size())
                                                           / static_cast<double> (maxOutputFrames)));
    const double perFramePixelBudget = static_cast<double> (maxStoredPixels)
                                       / std::min (rawFrames.size(), maxOutputFrames);
    const double budgetScale = std::sqrt (perFramePixelBudget
                                          / (static_cast<double> (width) * static_cast<double> (height)));
    const double sideScale = static_cast<double> (maxOutputSide) / std::max (width, height);
    const double outputScale = std::min ({ 1.0, sideScale, budgetScale });
    const int outWidth = juce::jmax (1, juce::roundToInt (width * outputScale));
    const int outHeight = juce::jmax (1, juce::roundToInt (height * outputScale));

    const bool firstFrameTransparentBackground = rawFrames.front().hasTransparency
                                                  && rawFrames.front().transparentIndex == static_cast<int> (bytes[11]);
    juce::Image canvas (juce::Image::ARGB, outWidth, outHeight, true);
    {
        juce::Graphics g (canvas);
        g.fillAll (firstFrameTransparentBackground ? juce::Colours::transparentBlack : backgroundColour);
    }

    int accumulatedDelayMs = 0;
    const auto stride = static_cast<std::size_t> (frameStride);
    for (std::size_t i = 0; i < rawFrames.size(); ++i)
    {
        auto& raw = rawFrames[i];
        juce::MemoryInputStream stream (raw.singleFrameGif.data(), raw.singleFrameGif.size(), false);
        auto rectImage = juce::ImageFileFormat::loadFrom (stream);
        if (! rectImage.isValid() || rectImage.getWidth() != raw.width || rectImage.getHeight() != raw.height)
            return false;

        const int outX = juce::jlimit (0, outWidth - 1, juce::roundToInt (raw.x * outputScale));
        const int outY = juce::jlimit (0, outHeight - 1, juce::roundToInt (raw.y * outputScale));
        const int outRight = juce::jlimit (outX + 1, outWidth, juce::roundToInt ((raw.x + raw.width) * outputScale));
        const int outBottom = juce::jlimit (outY + 1, outHeight, juce::roundToInt ((raw.y + raw.height) * outputScale));
        const int drawWidth = outRight - outX;
        const int drawHeight = outBottom - outY;
        if (drawWidth <= 0 || drawHeight <= 0)
            return false;
        if (rectImage.getWidth() != drawWidth || rectImage.getHeight() != drawHeight)
            rectImage = rectImage.rescaled (drawWidth, drawHeight, juce::Graphics::highResamplingQuality);

        const auto previousCanvas = raw.disposal == 3 ? canvas.createCopy() : juce::Image();
        {
            juce::Graphics g (canvas);
            g.drawImageAt (rectImage, outX, outY);
        }
        const bool keepOutputFrame = (i % stride == 0) || i + 1 == rawFrames.size();
        if (keepOutputFrame)
        {
            // Delay belongs to the image currently shown. Any skipped raw
            // frames extend the previous displayed frame, not this one.
            outputFrames.push_back ({ canvas.createCopy(), juce::jlimit (20, 5000, raw.delayMs) });
        }
        else if (! outputFrames.empty())
        {
            auto& previous = outputFrames.back();
            previous.delayMs = juce::jlimit (20, 5000, previous.delayMs + raw.delayMs);
        }

        if (raw.disposal == 2)
        {
            // Build a fresh canvas so a transparent logical background really
            // clears prior pixels instead of alpha-blending over them.
            juce::Image next (juce::Image::ARGB, outWidth, outHeight, true);
            juce::Graphics g (next);
            const int right = outX + drawWidth;
            const int bottom = outY + drawHeight;
            auto copyRegion = [&] (int x, int y, int w, int h)
            {
                if (w > 0 && h > 0)
                    g.drawImage (canvas, x, y, w, h, x, y, w, h);
            };
            copyRegion (0, 0, outWidth, outY);
            copyRegion (0, bottom, outWidth, outHeight - bottom);
            copyRegion (0, outY, outX, drawHeight);
            copyRegion (right, outY, outWidth - right, drawHeight);
            const bool transparentBackground = raw.hasTransparency
                                               && raw.transparentIndex == static_cast<int> (bytes[11]);
            g.setColour (transparentBackground ? juce::Colours::transparentBlack : backgroundColour);
            g.fillRect (outX, outY, drawWidth, drawHeight);
            canvas = std::move (next);
        }
        else if (raw.disposal == 3 && previousCanvas.isValid())
        {
            canvas = previousCanvas;
        }
    }

    return ! outputFrames.empty();
}
} // namespace GridARVGif
