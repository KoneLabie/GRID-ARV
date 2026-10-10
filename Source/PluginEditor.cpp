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
        // categories (in table order)
        for (int i = 0; i < kNumModes; ++i) categories.addIfNotAlreadyThere(kModes[i].category);

        addSection("VISUAL");
        catBox = addPlainCombo("Category");
        modeBox = addPlainCombo("Mode");
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
        addCombo(ParamID::quality, "Quality");
        if (!proc.standalone) addCombo(ParamID::source, "Input");
        addSlider(ParamID::sens, "Sensitivity");
        addSlider(ParamID::smooth, "Smoothing");
        addSlider(ParamID::speed, "Speed");
        addSlider(ParamID::bright, "Brightness");
        addSlider(ParamID::trails, "Trails");
        addSlider(ParamID::zoom, "Trail zoom");
        addSlider(ParamID::rot, "Trail rotate");
        addSlider(ParamID::glow, "Glow");
        addSlider(ParamID::aberr, "Aberration");
        addSlider(ParamID::hue, "Hue shift");
        addSlider(ParamID::hueSpin, "Hue cycle");
        addSlider(ParamID::autoBeats, "Auto-cycle (beats)");

        addSection("IMAGES  (drop files on the window)");
        addButtons({ "Load image...", "No image" },
                   { [this] { chooseFiles("Choose images", "*.png;*.jpg;*.jpeg;*.gif"); },
                     [this] { proc.setParamPlain(ParamID::imgSlot, 0.0f); } });
        imageBox = addPlainCombo("Image");
        imageBox->onChange = [this] { proc.setParamPlain(ParamID::imgSlot, (float) juce::jmax(0, imageBox->getSelectedId() - 1)); };
        addSlider(ParamID::imgMix, "Background mix");
        addSlider(ParamID::imgBeats, "Cycle (beats)");

        addSection("TEXT / LYRICS");
        addToggle(ParamID::textOn, "Show text");
        textBox = new juce::TextEditor();
        owned.emplace_back(textBox);
        addAndMakeVisible(textBox);
        textBox->setMultiLine(true, true);
        textBox->setReturnKeyStartsNewLine(true);
        textBox->setTextToShowWhenEmpty("One line per slide. Use | for a line break.\nOr drop a .lrc / .srt file for timed lyrics.", juce::Colours::grey);
        textBox->setText(proc.text.getRawText(), false);
        seenTextRev = proc.text.getTextRevision();
        textBox->onTextChange = [this]
        {
            proc.text.setRawText(textBox->getText());
            seenTextRev = proc.text.getTextRevision();
        };
        items.push_back({ Full, textBox, nullptr, nullptr, 120 });

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
        addCombo(ParamID::textColor, "Colour");
        addCombo(ParamID::textPos, "Position");
        addSlider(ParamID::textSize, "Size");
        addSlider(ParamID::textReact, "Beat reaction");
        addSlider(ParamID::textLine, "Line (0 = auto)");
        addSlider(ParamID::textBeats, "Advance (beats)");
        addToggle(ParamID::followHost, "Lyrics follow DAW playhead");
        addSlider(ParamID::lyricOff, "Lyric offset (s)");
        addButtons({ "Clock: Play", "Stop", "Reset" },
                   { [this] { proc.lyricRunning = true; },
                     [this] { proc.lyricRunning = false; },
                     [this] { proc.lyricClock = 0.0; } });

        if (proc.standalone)
        {
            addSection("WINDOW");
            addButtons({ "Audio settings...", "Fullscreen (F)" },
                       { [this] { if (editor.onOpenAudioSettings) editor.onOpenAudioSettings(); },
                         [this] { if (editor.onToggleFullscreen) editor.onToggleFullscreen(); } });
        }

        status = new juce::Label();
        owned.emplace_back(status);
        addAndMakeVisible(status);
        status->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        status->setJustificationType(juce::Justification::topLeft);
        items.push_back({ Full, status, nullptr, nullptr, 56 });

        auto* help = new juce::Label({}, "Keys: Tab = hide/show panel, Left/Right = prev/next mode, R = random, F = fullscreen. Double-click the view to hide the panel.");
        owned.emplace_back(help);
        addAndMakeVisible(help);
        help->setColour(juce::Label::textColourId, juce::Colours::grey);
        help->setJustificationType(juce::Justification::topLeft);
        items.push_back({ Full, help, nullptr, nullptr, 56 });

        fillModes(0);
        syncAssets();
    }

    void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xE60D0E14)); }

    void resized() override
    {
        const int w = getWidth(), pad = 10;
        int y = 8;
        for (auto& it : items)
        {
            const int x0 = pad, fw = w - pad * 2;
            switch (it.kind)
            {
                case Full:     it.a->setBounds(x0, y, fw, it.h); break;
                case LabelCtl: it.a->setBounds(x0, y, (int) (fw * 0.34f), it.h);
                               it.b->setBounds(x0 + (int) (fw * 0.34f), y, fw - (int) (fw * 0.34f), it.h); break;
                case Split2:   { const int hw = (fw - 6) / 2;
                                 it.a->setBounds(x0, y, hw, it.h); it.b->setBounds(x0 + hw + 6, y, hw, it.h); break; }
                case Split3:   { const int tw = (fw - 12) / 3;
                                 it.a->setBounds(x0, y, tw, it.h); it.b->setBounds(x0 + tw + 6, y, tw, it.h);
                                 it.c->setBounds(x0 + 2 * (tw + 6), y, tw, it.h); break; }
            }
            y += it.h + 4;
        }
    }

    int getTotalHeight() const
    {
        int y = 8;
        for (auto& it : items) y += it.h + 4;
        return y + 8;
    }

    // called from the editor's timer
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

        // fonts: default + imported + system
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
    enum Kind { Full, LabelCtl, Split2, Split3 };
    struct Item { Kind kind; juce::Component* a; juce::Component* b; juce::Component* c; int h; };

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

    //------------------------------------------------------------ widget factories
    juce::Label* newLabel(const juce::String& t)
    {
        auto* l = new juce::Label({}, t);
        owned.emplace_back(l);
        addAndMakeVisible(l);
        l->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        return l;
    }

    void addSection(const juce::String& title)
    {
        auto* l = newLabel(title);
        l->setFont(vizFontNamed(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::bold));
        l->setColour(juce::Label::textColourId, juce::Colour(0xFF59D2FF));
        items.push_back({ Full, l, nullptr, nullptr, 22 });
    }

    juce::ComboBox* addPlainCombo(const juce::String& label)
    {
        auto* c = new juce::ComboBox();
        owned.emplace_back(c);
        addAndMakeVisible(c);
        items.push_back({ LabelCtl, newLabel(label), c, nullptr, 24 });
        return c;
    }

    void addCombo(const char* id, const juce::String& label)
    {
        auto* c = new juce::ComboBox();
        owned.emplace_back(c);
        addAndMakeVisible(c);
        if (auto* cp = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(id)))
            c->addItemList(cp->choices, 1);
        comboAtts.push_back(std::make_unique<ComboAtt>(proc.apvts, id, *c));
        items.push_back({ LabelCtl, newLabel(label), c, nullptr, 24 });
    }

    void addSlider(const char* id, const juce::String& label)
    {
        auto* s = new juce::Slider(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        owned.emplace_back(s);
        addAndMakeVisible(s);
        s->setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 18);
        sliderAtts.push_back(std::make_unique<SliderAtt>(proc.apvts, id, *s));
        items.push_back({ LabelCtl, newLabel(label), s, nullptr, 22 });
    }

    void addToggle(const char* id, const juce::String& label)
    {
        auto* b = new juce::ToggleButton(label);
        owned.emplace_back(b);
        addAndMakeVisible(b);
        buttonAtts.push_back(std::make_unique<ButtonAtt>(proc.apvts, id, *b));
        items.push_back({ Full, b, nullptr, nullptr, 24 });
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
            b->onClick = actions[i++];
            comps.push_back(b);
        }
        if (comps.size() == 2) items.push_back({ Split2, comps[0], comps[1], nullptr, 26 });
        else items.push_back({ Split3, comps[0], comps[1], comps[2], 26 });
    }

    AudioVisualizerEditor& editor;
    AudioVisualizerProcessor& proc;
    std::vector<std::unique_ptr<juce::Component>> owned;   // declared before the attachments: destroyed after them
    std::vector<std::unique_ptr<SliderAtt>> sliderAtts;
    std::vector<std::unique_ptr<ComboAtt>> comboAtts;
    std::vector<std::unique_ptr<ButtonAtt>> buttonAtts;
    std::vector<Item> items;
    juce::StringArray categories, fontKeys;
    juce::ComboBox *catBox = nullptr, *modeBox = nullptr, *imageBox = nullptr, *fontBox = nullptr;
    juce::TextEditor* textBox = nullptr;
    juce::Label* status = nullptr;
    std::unique_ptr<juce::FileChooser> chooser;
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
    panelButton.setAlpha(0.8f);
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
    const int pw = juce::jmin(340, w / 2 + 40);
    viewport.setBounds(w - pw, 0, pw, h);
    panel->setSize(juce::jmax(100, viewport.getMaximumVisibleWidth()), panel->getTotalHeight());
    panelButton.setBounds((viewport.isVisible() ? w - pw : w) - 40, 6, 34, 26);
    proc.editorW = w;
    proc.editorH = h;
}

void AudioVisualizerEditor::togglePanel()
{
    viewport.setVisible(!viewport.isVisible());
    resized();
}

void AudioVisualizerEditor::timerCallback()
{
    panel->sync();
    const bool recentlyMoved = (juce::Time::getMillisecondCounter() - lastMoveMs) < 3500;
    panelButton.setVisible(recentlyMoved || viewport.isVisible());
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
