#include "PluginEditor.h"
#include "TextEngine.h"

using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

//==============================================================================
// Control panel
//==============================================================================
class AudioVisualizerEditor::Panel : public juce::Component
{
public:
    Panel(AudioVisualizerEditor& e, AudioVisualizerProcessor& p) : editor(e), proc(p)
    {
        for (int i = 0; i < kNumModes; ++i) categories.addIfNotAlreadyThere(kModes[i].category);

        addSection("QUICK CONTROLS", true);
        catBox = addPlainCombo("Category");
        modeBox = addPlainCombo("Visual mode");
        for (auto& c : categories) catBox->addItem(c, catBox->getNumItems() + 1);
        catBox->onChange = [this]
        {
            const int c = catBox->getSelectedId() - 1;
            if (c < 0) return;
            fillModes(c);
            for (int i = 0; i < kNumModes; ++i)
                if (categories[c] == juce::String(kModes[i].category)) { proc.requestMode(i); break; }
        };
        modeBox->onChange = [this] { if (modeBox->getSelectedId() > 0) proc.requestMode(modeBox->getSelectedId() - 1); };
        addButtons({ "< Prev", "Next >", "Random" },
                   { [this] { editor.stepMode(-1); }, [this] { editor.stepMode(1); }, [this] { editor.randomMode(); } });
        addCombo(ParamID::palette, "Palette");
        addCombo(ParamID::quality, "Render quality");

        addSection("MOTION & COLOUR", false);
        if (!proc.standalone) addCombo(ParamID::source, "Input source");
        addSlider(ParamID::sens, "Sensitivity");
        addSlider(ParamID::smooth, "Smoothing");
        addSlider(ParamID::speed, "Animation speed");
        addSlider(ParamID::bright, "Brightness");
        addSlider(ParamID::hue, "Hue shift");
        addSlider(ParamID::hueSpin, "Hue cycle");
        addSlider(ParamID::trails, "Trails");
        addSlider(ParamID::zoom, "Trail zoom");
        addSlider(ParamID::rot, "Trail rotation");
        addSlider(ParamID::glow, "Glow");
        addSlider(ParamID::aberr, "Beat aberration");
        addSlider(ParamID::autoBeats, "Auto-cycle (beats)");

        addSection("IMAGE SOURCE & BACKGROUND", false);
        addButtons({ "Load image...", "No image" },
                   { [this] { chooseFiles("Choose images", "*.png;*.jpg;*.jpeg;*.gif;*.bmp"); },
                     [this] { proc.setParamPlain(ParamID::imgSlot, 0.0f); } });
        imageBox = addPlainCombo("Selected image");
        imageBox->onChange = [this]
        {
            proc.setParamPlain(ParamID::imgSlot, (float) juce::jmax(0, imageBox->getSelectedId() - 1));
        };
        addSlider(ParamID::imgMix, "Background mix");
        addSlider(ParamID::imgBeats, "Cycle images (beats)");

        addSection("IMAGE FRAMING & ORIENTATION", false);
        addCombo(ParamID::imgFit, "Fit mode");
        addSlider(ParamID::imgScale, "Image scale");
        addSlider(ParamID::imgRotate, "Rotation (degrees)");
        addToggle(ParamID::imgFlipX, "Flip horizontally");
        addToggle(ParamID::imgFlipY, "Flip vertically");
        addSlider(ParamID::imgOffsetX, "Horizontal position");
        addSlider(ParamID::imgOffsetY, "Vertical position");
        addActionButton("Reset image framing", [this]
        {
            proc.setParamPlain(ParamID::imgFit, 0.0f);
            proc.setParamPlain(ParamID::imgScale, 1.0f);
            proc.setParamPlain(ParamID::imgRotate, 0.0f);
            proc.setParamPlain(ParamID::imgFlipX, 0.0f);
            proc.setParamPlain(ParamID::imgFlipY, 0.0f);
            proc.setParamPlain(ParamID::imgOffsetX, 0.0f);
            proc.setParamPlain(ParamID::imgOffsetY, 0.0f);
        });

        addSection("TEXT & LYRICS", false);
        addToggle(ParamID::textOn, "Show visual text");
        textBox = new juce::TextEditor();
        owned.emplace_back(textBox);
        addAndMakeVisible(textBox);
        styleTextEditor(*textBox);
        textBox->setMultiLine(true, true);
        textBox->setReturnKeyStartsNewLine(true);
        textBox->setTextToShowWhenEmpty("One line per slide. Use | for a line break.\nOr load/drop a .lrc or .srt file for timed lyrics.", juce::Colours::grey);
        textBox->setText(proc.text.getRawText(), false);
        seenTextRev = proc.text.getTextRevision();
        textBox->onTextChange = [this]
        {
            proc.text.setRawText(textBox->getText());
            seenTextRev = proc.text.getTextRevision();
        };
        items.push_back({ Full, textBox, nullptr, nullptr, 112, currentSection });
        addButtons({ "Load lyrics/text...", "Clear text" },
                   { [this] { chooseFiles("Choose lyrics or text", "*.lrc;*.srt;*.txt"); },
                     [this] { textBox->setText({}, true); } });
        fontBox = addPlainCombo("Font");
        fontBox->onChange = [this]
        {
            const int id = fontBox->getSelectedId();
            if (id >= 1 && id <= fontKeys.size()) proc.applyFontKey(fontKeys[id - 1]);
        };
        addButtons({ "Load font...", "Reset font" },
                   { [this] { chooseFiles("Choose a font", "*.ttf;*.otf;*.ttc"); },
                     [this] { proc.applyFontKey({}); syncFont(); } });
        addCombo(ParamID::textAnim, "Animation");
        addCombo(ParamID::textColor, "Text colour");
        addCombo(ParamID::textPos, "Text position");
        addSlider(ParamID::textSize, "Text size");
        addSlider(ParamID::textReact, "Beat reaction");

        addSection("TIMING & ADVANCED", false);
        addSlider(ParamID::textLine, "Line (0 = automatic)");
        addSlider(ParamID::textBeats, "Advance (beats)");
        addToggle(ParamID::followHost, "Follow DAW playhead for lyrics");
        addSlider(ParamID::lyricOff, "Lyric timing offset (s)");
        addButtons({ "Clock: Play", "Stop", "Reset" },
                   { [this] { proc.lyricRunning = true; },
                     [this] { proc.lyricRunning = false; },
                     [this] { proc.lyricClock = 0.0; } });

        addSection("WINDOW & STREAM CAPTURE", false);
        addActionButton("Toggle clean output  (F8)", [this] { editor.toggleCleanOutput(); });
        if (proc.standalone)
        {
            addButtons({ "Audio settings...", "Fullscreen (F)" },
                       { [this] { if (editor.onOpenAudioSettings) editor.onOpenAudioSettings(); },
                         [this] { if (editor.onToggleFullscreen) editor.onToggleFullscreen(); } });
        }

        addSection("STATUS & SHORTCUTS", false);
        status = new juce::Label();
        owned.emplace_back(status);
        addAndMakeVisible(status);
        status->setColour(juce::Label::textColourId, juce::Colour(0xFFE2E8F0));
        status->setFont(vizFontNamed(juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain));
        status->setJustificationType(juce::Justification::topLeft);
        items.push_back({ Full, status, nullptr, nullptr, 50, currentSection });
        auto* help = new juce::Label({}, "Tab: show/hide controls   F8: clean output\nLeft/Right: change mode   R: random   F: fullscreen\nDouble-click the visual to toggle the panel.");
        owned.emplace_back(help);
        addAndMakeVisible(help);
        help->setColour(juce::Label::textColourId, juce::Colour(0xFF9BA9BD));
        help->setFont(vizFontNamed(juce::Font::getDefaultSansSerifFontName(), 11.0f, juce::Font::plain));
        help->setJustificationType(juce::Justification::topLeft);
        items.push_back({ Full, help, nullptr, nullptr, 50, currentSection });

        fillModes(0);
        syncAssets();
    }

    void paint(juce::Graphics& g) override
    {
        juce::ColourGradient bg(juce::Colour(0xF20C101B), 0.0f, 0.0f,
                                juce::Colour(0xF5171025), (float) getWidth(), (float) getHeight(), false);
        g.setGradientFill(bg);
        g.fillAll();
        g.setColour(juce::Colour(0xFF28D7E8).withAlpha(0.8f));
        g.fillRect(0, 0, getWidth(), 2);
    }

    void resized() override
    {
        const int w = getWidth(), pad = 10;
        int y = 8;
        for (auto& it : items)
        {
            const bool visible = it.kind == Header
                || (it.section >= 0 && it.section < (int) sections.size() && sections[(size_t) it.section].expanded);
            setItemVisible(it, visible);
            if (!visible) continue;

            const int x0 = pad, fw = juce::jmax(20, w - pad * 2);
            switch (it.kind)
            {
                case Header:   it.a->setBounds(x0, y, fw, it.h); break;
                case Full:     it.a->setBounds(x0, y, fw, it.h); break;
                case LabelCtl: {
                    const int labelW = (int) (fw * 0.39f);
                    it.a->setBounds(x0, y, labelW, it.h);
                    it.b->setBounds(x0 + labelW, y, fw - labelW, it.h);
                    break;
                }
                case Split2:   {
                    const int hw = (fw - 6) / 2;
                    it.a->setBounds(x0, y, hw, it.h); it.b->setBounds(x0 + hw + 6, y, hw, it.h);
                    break;
                }
                case Split3:   {
                    const int tw = (fw - 12) / 3;
                    it.a->setBounds(x0, y, tw, it.h); it.b->setBounds(x0 + tw + 6, y, tw, it.h);
                    it.c->setBounds(x0 + 2 * (tw + 6), y, tw, it.h);
                    break;
                }
            }
            y += it.h + 5;
        }
    }

    int getTotalHeight() const
    {
        int y = 8;
        for (const auto& it : items)
        {
            const bool visible = it.kind == Header
                || (it.section >= 0 && it.section < (int) sections.size() && sections[(size_t) it.section].expanded);
            if (visible) y += it.h + 5;
        }
        return y + 8;
    }

    // Called from the editor's timer.
    void sync()
    {
        const int m = editor.currentMode();
        if (m != shownMode) { shownMode = m; showMode(m); }

        const int slot = (int) proc.getParamPlain(ParamID::imgSlot);
        if (proc.assets.getImageRevision() != seenAssetRev) syncAssets();
        else if (slot != shownSlot) { shownSlot = slot; imageBox->setSelectedId(slot + 1, juce::dontSendNotification); }

        if (proc.text.getTextRevision() != seenTextRev)
        {
            seenTextRev = proc.text.getTextRevision();
            textBox->setText(proc.text.getRawText(), false);
        }
        if (proc.text.getFontKey() != shownFontKey) syncFont();
        status->setText(editor.getStatusText(), juce::dontSendNotification);
    }

    void syncAssets()
    {
        seenAssetRev = proc.assets.getImageRevision();
        imageBox->clear(juce::dontSendNotification);
        imageBox->addItem("(none)", 1);
        int id = 2;
        for (auto& n : proc.assets.getImageNames()) imageBox->addItem(n, id++);
        shownSlot = (int) proc.getParamPlain(ParamID::imgSlot);
        imageBox->setSelectedId(shownSlot + 1, juce::dontSendNotification);

        fontBox->clear(juce::dontSendNotification);
        fontKeys.clear();
        fontBox->addItem("Default (bold sans)", 1); fontKeys.add(juce::String());
        int fid = 2;
        const auto custom = proc.assets.getFontNames();
        if (custom.size() > 0)
        {
            fontBox->addSeparator();
            fontBox->addSectionHeading("Imported fonts");
            for (auto& n : custom) { fontBox->addItem(n, fid++); fontKeys.add("file:" + n); }
        }
        fontBox->addSeparator();
        fontBox->addSectionHeading("System fonts");
        for (auto& n : juce::Font::findAllTypefaceNames()) { fontBox->addItem(n, fid++); fontKeys.add("sys:" + n); }
        syncFont();
    }

    void syncFont()
    {
        shownFontKey = proc.text.getFontKey();
        const int i = fontKeys.indexOf(shownFontKey);
        fontBox->setSelectedId(i >= 0 ? i + 1 : 1, juce::dontSendNotification);
    }

private:
    enum Kind { Header, Full, LabelCtl, Split2, Split3 };
    struct Item { Kind kind; juce::Component* a; juce::Component* b; juce::Component* c; int h; int section; };
    struct Section { juce::TextButton* button; juce::String title; bool expanded; };

    void setItemVisible(const Item& it, bool visible)
    {
        if (it.a != nullptr) { it.a->setVisible(visible); if (!visible) it.a->setBounds(0, 0, 0, 0); }
        if (it.b != nullptr) { it.b->setVisible(visible); if (!visible) it.b->setBounds(0, 0, 0, 0); }
        if (it.c != nullptr) { it.c->setVisible(visible); if (!visible) it.c->setBounds(0, 0, 0, 0); }
    }

    void addSection(const juce::String& title, bool expanded = false)
    {
        const int idx = (int) sections.size();
        auto* b = new juce::TextButton();
        owned.emplace_back(b);
        addAndMakeVisible(b);
        b->setClickingTogglesState(false);
        b->setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF202338));
        b->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF293149));
        b->setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF6DE7F4));
        b->setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFB6FAFF));
        sections.push_back({ b, title, expanded });
        currentSection = idx;
        updateSectionButton(idx);
        items.push_back({ Header, b, nullptr, nullptr, 29, idx });
        b->onClick = [this, idx]
        {
            sections[(size_t) idx].expanded = !sections[(size_t) idx].expanded;
            updateSectionButton(idx);
            editor.resized();
        };
    }

    void updateSectionButton(int idx)
    {
        auto& sec = sections[(size_t) idx];
        sec.button->setButtonText((sec.expanded ? "v   " : ">   ") + sec.title);
    }

    juce::Label* newLabel(const juce::String& t)
    {
        auto* l = new juce::Label({}, t);
        owned.emplace_back(l);
        addAndMakeVisible(l);
        l->setColour(juce::Label::textColourId, juce::Colour(0xFFD8DFEC));
        l->setFont(vizFontNamed(juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain));
        return l;
    }

    juce::ComboBox* addPlainCombo(const juce::String& label)
    {
        auto* c = new juce::ComboBox();
        owned.emplace_back(c);
        addAndMakeVisible(c);
        styleCombo(*c);
        items.push_back({ LabelCtl, newLabel(label), c, nullptr, 25, currentSection });
        return c;
    }

    void addCombo(const char* id, const juce::String& label)
    {
        auto* c = new juce::ComboBox();
        owned.emplace_back(c);
        addAndMakeVisible(c);
        styleCombo(*c);
        if (auto* cp = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(id)))
            c->addItemList(cp->choices, 1);
        comboAtts.push_back(std::make_unique<ComboAtt>(proc.apvts, id, *c));
        items.push_back({ LabelCtl, newLabel(label), c, nullptr, 25, currentSection });
    }

    void addSlider(const char* id, const juce::String& label)
    {
        auto* s = new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        owned.emplace_back(s);
        addAndMakeVisible(s);
        s->setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 18);
        s->setColour(juce::Slider::trackColourId, juce::Colour(0xFF32CBDC));
        s->setColour(juce::Slider::thumbColourId, juce::Colour(0xFFB9F7FF));
        s->setColour(juce::Slider::backgroundColourId, juce::Colour(0xFF252B3D));
        s->setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFE8EDF6));
        s->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF111522));
        s->setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xFF30384D));
        sliderAtts.push_back(std::make_unique<SliderAtt>(proc.apvts, id, *s));
        items.push_back({ LabelCtl, newLabel(label), s, nullptr, 23, currentSection });
    }

    void addToggle(const char* id, const juce::String& label)
    {
        auto* b = new juce::ToggleButton(label);
        owned.emplace_back(b);
        addAndMakeVisible(b);
        b->setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFDCE5F1));
        b->setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFF35D0E1));
        buttonAtts.push_back(std::make_unique<ButtonAtt>(proc.apvts, id, *b));
        items.push_back({ Full, b, nullptr, nullptr, 24, currentSection });
    }

    void addButtons(std::initializer_list<const char*> names, std::vector<std::function<void()>> actions)
    {
        std::vector<juce::Component*> comps;
        size_t i = 0;
        for (auto* n : names)
        {
            auto* b = new juce::TextButton(n);
            owned.emplace_back(b);
            addAndMakeVisible(b);
            styleActionButton(*b);
            b->onClick = actions[i++];
            comps.push_back(b);
        }
        if (comps.size() == 2) items.push_back({ Split2, comps[0], comps[1], nullptr, 28, currentSection });
        else items.push_back({ Split3, comps[0], comps[1], comps[2], 28, currentSection });
    }

    void addActionButton(const juce::String& name, std::function<void()> action)
    {
        auto* b = new juce::TextButton(name);
        owned.emplace_back(b);
        addAndMakeVisible(b);
        styleActionButton(*b);
        b->onClick = std::move(action);
        items.push_back({ Full, b, nullptr, nullptr, 29, currentSection });
    }

    void styleActionButton(juce::TextButton& b)
    {
        b.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF24283A));
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF33435A));
        b.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFF1F5FC));
        b.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFB7FAFF));
    }

    void styleCombo(juce::ComboBox& c)
    {
        c.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF151A28));
        c.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFE9EEF8));
        c.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xFF333C53));
        c.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xFF69DCEB));
    }

    void styleTextEditor(juce::TextEditor& t)
    {
        t.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF101522));
        t.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xFF30394F));
        t.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xFF37C8DC));
        t.setColour(juce::TextEditor::textColourId, juce::Colour(0xFFE7EDF8));
        t.setColour(juce::TextEditor::highlightColourId, juce::Colour(0xFF2F718D));
        t.setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::white);
        t.setColour(juce::CaretComponent::caretColourId, juce::Colour(0xFFBDF8FF));
    }

    void chooseFiles(const juce::String& title, const juce::String& wildcard)
    {
        chooser = std::make_unique<juce::FileChooser>(title, juce::File(), wildcard);
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
                                 | juce::FileBrowserComponent::canSelectMultipleItems,
                             [this](const juce::FileChooser& fc)
                             {
                                 for (auto& f : fc.getResults()) editor.handleFile(f);
                             });
    }

    void fillModes(int cat)
    {
        modeBox->clear(juce::dontSendNotification);
        for (int i = 0; i < kNumModes; ++i)
            if (categories[cat] == juce::String(kModes[i].category)) modeBox->addItem(kModes[i].name, i + 1);
    }

    void showMode(int m)
    {
        const int cat = categories.indexOf(kModes[m].category);
        if (cat >= 0 && catBox->getSelectedId() != cat + 1)
        {
            catBox->setSelectedId(cat + 1, juce::dontSendNotification);
            fillModes(cat);
        }
        modeBox->setSelectedId(m + 1, juce::dontSendNotification);
    }

    AudioVisualizerEditor& editor;
    AudioVisualizerProcessor& proc;
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<std::unique_ptr<SliderAtt>> sliderAtts;
    std::vector<std::unique_ptr<ComboAtt>> comboAtts;
    std::vector<std::unique_ptr<ButtonAtt>> buttonAtts;
    std::vector<Item> items;
    std::vector<Section> sections;
    juce::StringArray categories, fontKeys;
    juce::ComboBox *catBox = nullptr, *modeBox = nullptr, *imageBox = nullptr, *fontBox = nullptr;
    juce::TextEditor* textBox = nullptr;
    juce::Label* status = nullptr;
    std::unique_ptr<juce::FileChooser> chooser;
    int currentSection = -1;
    int shownMode = -1, shownSlot = -1, seenAssetRev = -1, seenTextRev = -1;
    juce::String shownFontKey = "?";
};

//==============================================================================
// Editor
//==============================================================================
AudioVisualizerEditor::AudioVisualizerEditor(AudioVisualizerProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    panel = std::make_unique<Panel>(*this, proc);
    viewport.setViewedComponent(panel.get(), false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    panelButton.onClick = [this] { togglePanel(); };
    panelButton.setAlpha(0.92f);
    panelButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xEE171D2C));
    panelButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF2A3A50));
    panelButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFB9F6FF));
    panelButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFFFFFFFF));
    addAndMakeVisible(panelButton);

    setWantsKeyboardFocus(true);
    addMouseListener(this, true);   // so mouseMove from children also wakes the UI button
    setResizable(true, true);
    setResizeLimits(480, 300, 4096, 2304);
    setSize(juce::jlimit(480, 4096, proc.editorW.load()), juce::jlimit(300, 2304, proc.editorH.load()));

    renderer = std::make_unique<VisualRenderer>(proc, glContext, *this);
    glContext.setRenderer(renderer.get());
    glContext.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
    glContext.setContinuousRepainting(true);
    glContext.setComponentPaintingEnabled(true);
    glContext.setSwapInterval(1);
    glContext.attachTo(*this);

    lastMoveMs = juce::Time::getMillisecondCounter();
    startTimerHz(15);
}

AudioVisualizerEditor::~AudioVisualizerEditor()
{
    stopTimer();
    glContext.detach();   // must happen before the renderer / components are destroyed
}

void AudioVisualizerEditor::resized()
{
    const int w = getWidth(), h = getHeight();
    const int pw = juce::jmin(380, w / 2 + 40);
    viewport.setBounds(w - pw, 0, pw, h);
    panel->setSize(juce::jmax(100, viewport.getMaximumVisibleWidth()), panel->getTotalHeight());
    panelButton.setBounds((viewport.isVisible() ? w - pw : w) - 44, 6, 38, 28);
    panelButton.setVisible(!cleanOutput && viewport.isVisible());
    proc.editorW = w;
    proc.editorH = h;
}

void AudioVisualizerEditor::togglePanel()
{
    if (cleanOutput)
    {
        cleanOutput = false;
        viewport.setVisible(true);
    }
    else
    {
        viewport.setVisible(!viewport.isVisible());
    }
    resized();
    grabKeyboardFocus();
}

void AudioVisualizerEditor::toggleCleanOutput()
{
    cleanOutput = !cleanOutput;
    // Stream-safe mode hides both the settings panel and its floating UI button.
    // The F8 shortcut restores the panel, keeping the output free of control text.
    viewport.setVisible(!cleanOutput);
    resized();
    grabKeyboardFocus();
}

void AudioVisualizerEditor::timerCallback()
{
    panel->sync();
    const bool recentlyMoved = (juce::Time::getMillisecondCounter() - lastMoveMs) < 3500;
    panelButton.setVisible(!cleanOutput && (recentlyMoved || viewport.isVisible()));
}

juce::String AudioVisualizerEditor::getStatusText() const
{
    juce::String s;
    if (renderer != nullptr)
    {
        s << juce::String(renderer->getFps(), 0) << " fps  |  " << kModes[juce::jlimit(0, kNumModes - 1, renderer->getEffectiveMode())].name;
        const auto err = renderer->getShaderError();
        if (err.isNotEmpty()) s << "\nSHADER ERROR: " << err.upToFirstOccurrenceOf("\n", false, false).substring(0, 160);
    }
    return s;
}

void AudioVisualizerEditor::stepMode(int delta)
{
    proc.requestMode((currentMode() + delta + kNumModes) % kNumModes);
}

void AudioVisualizerEditor::randomMode()
{
    proc.requestMode(juce::Random::getSystemRandom().nextInt(kNumModes));
}

bool AudioVisualizerEditor::keyPressed(const juce::KeyPress& k)
{
    if (k == juce::KeyPress::tabKey)   { togglePanel(); return true; }
    if (k == juce::KeyPress::F8Key || k.getTextCharacter() == 'o' || k.getTextCharacter() == 'O')
    { toggleCleanOutput(); return true; }
    if (k == juce::KeyPress::leftKey)  { stepMode(-1); return true; }
    if (k == juce::KeyPress::rightKey) { stepMode(1); return true; }
    if (k.getTextCharacter() == 'r' || k.getTextCharacter() == 'R') { randomMode(); return true; }
    if (k.getTextCharacter() == 'f' || k.getTextCharacter() == 'F') { if (onToggleFullscreen) { onToggleFullscreen(); return true; } }
    if (k == juce::KeyPress::escapeKey && onToggleFullscreen) { onToggleFullscreen(); return true; }
    return false;
}

void AudioVisualizerEditor::mouseMove(const juce::MouseEvent&) { lastMoveMs = juce::Time::getMillisecondCounter(); }

void AudioVisualizerEditor::mouseDown(const juce::MouseEvent& e)
{
    lastMoveMs = juce::Time::getMillisecondCounter();
    if (e.eventComponent == this) grabKeyboardFocus();
}

void AudioVisualizerEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (e.eventComponent == this) togglePanel();
}

bool AudioVisualizerEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
    {
        juce::File file(f);
        if (AssetLibrary::isImageFile(file) || AssetLibrary::isFontFile(file) || AssetLibrary::isTextFile(file)) return true;
    }
    return false;
}

void AudioVisualizerEditor::filesDropped(const juce::StringArray& files, int, int)
{
    for (auto& f : files) handleFile(juce::File(f));
}

void AudioVisualizerEditor::handleFile(const juce::File& f)
{
    if (AssetLibrary::isImageFile(f))
    {
        const auto dest = proc.assets.importFile(f, false);
        const int i = proc.assets.indexOfImage(dest.getFileName());
        if (i >= 0) proc.setParamPlain(ParamID::imgSlot, (float) (i + 1));
    }
    else if (AssetLibrary::isFontFile(f))
    {
        const auto dest = proc.assets.importFile(f, true);
        proc.applyFontKey("file:" + dest.getFileName());
    }
    else if (AssetLibrary::isTextFile(f))
    {
        proc.text.setRawText(f.loadFileAsString());
        proc.lyricClock = 0.0;
    }
    panel->syncAssets();
}
