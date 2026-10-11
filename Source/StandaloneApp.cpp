// Standalone Grid ARV.
// Listens to any audio INPUT device (microphone, line-in, loopback / virtual cable) and shows the
// same visualizer as the plugin. Settings and the chosen audio device are remembered between runs.
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::File dataDir()
{
    auto d = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("GridARV");
    d.createDirectory();
    return d;
}

// Feeds incoming device audio to the analyzer. Nothing is played back.
class InputTap : public juce::AudioIODeviceCallback
{
public:
    explicit InputTap(AudioAnalyzer& a) : analyzer(a) {}

    void audioDeviceAboutToStart(juce::AudioIODevice* d) override { analyzer.setSampleRate(d->getCurrentSampleRate()); }
    void audioDeviceStopped() override {}

    void audioDeviceIOCallbackWithContext(const float* const* in, int numIn, float* const* out, int numOut,
                                          int numSamples, const juce::AudioIODeviceCallbackContext&) override
    {
        for (int c = 0; c < numOut; ++c)
            if (out[c] != nullptr) juce::FloatVectorOperations::clear(out[c], numSamples);

        if (numIn > 0 && in[0] != nullptr)
            analyzer.push(in[0], (numIn > 1 && in[1] != nullptr) ? in[1] : in[0], numSamples);
    }

private:
    AudioAnalyzer& analyzer;
};

class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(AudioVisualizerProcessor& p, std::function<void()> quit, std::function<void()> audioSettings)
        : DocumentWindow("Grid ARV", juce::Colours::black, DocumentWindow::allButtons),
          onClose(std::move(quit))
    {
        setUsingNativeTitleBar(true);
        auto* editor = new AudioVisualizerEditor(p);
        editor->setResizable(false, false);    // the window handles resizing
        editor->onToggleFullscreen = [this] { setFullScreen(!isFullScreen()); };
        editor->onOpenAudioSettings = std::move(audioSettings);
        setContentOwned(editor, false);
        setResizable(true, true);
        setResizeLimits(640, 360, 8192, 4608);
        centreWithSize(1280, 720);
        setVisible(true);
        editor->grabKeyboardFocus();
    }

    void closeButtonPressed() override { onClose(); }

private:
    std::function<void()> onClose;
};
} // namespace

class VisualizerApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Grid ARV"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        processor = std::make_unique<AudioVisualizerProcessor>();
        processor->standalone = true;

        const auto stateFile = dataDir().getChildFile("standalone.state");
        if (stateFile.existsAsFile())
        {
            juce::MemoryBlock mb;
            if (stateFile.loadFileAsData(mb)) processor->setStateInformation(mb.getData(), (int) mb.getSize());
        }

        tap = std::make_unique<InputTap>(processor->analyzer);
        std::unique_ptr<juce::XmlElement> savedDevice = juce::XmlDocument::parse(dataDir().getChildFile("audio.xml"));
        const auto err = deviceManager.initialise(2, 2, savedDevice.get(), true);
        deviceManager.addAudioCallback(tap.get());

        window = std::make_unique<MainWindow>(*processor, [this] { systemRequestedQuit(); }, [this] { showAudioSettings(); });

        if (err.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Audio device",
                                                   "Could not open an audio input:\n" + err
                                                   + "\n\nOpen \"Audio settings...\" in the panel to choose a device.");
    }

    void shutdown() override
    {
        if (processor != nullptr)
        {
            juce::MemoryBlock mb;
            processor->getStateInformation(mb);
            dataDir().getChildFile("standalone.state").replaceWithData(mb.getData(), mb.getSize());
        }
        if (auto xml = deviceManager.createStateXml())
            xml->writeTo(dataDir().getChildFile("audio.xml"));

        window.reset();                         // destroys the editor (and its GL context) first
        deviceManager.removeAudioCallback(tap.get());
        deviceManager.closeAudioDevice();
        tap.reset();
        processor.reset();
    }

private:
    void showAudioSettings()
    {
        auto* selector = new juce::AudioDeviceSelectorComponent(deviceManager, 0, 2, 0, 2, false, false, true, false);
        selector->setSize(520, 460);

        juce::DialogWindow::LaunchOptions o;
        o.content.setOwned(selector);
        o.dialogTitle = "Audio Settings";
        o.componentToCentreAround = window.get();
        o.dialogBackgroundColour = juce::Colours::darkgrey;
        o.escapeKeyTriggersCloseButton = true;
        o.useNativeTitleBar = true;
        o.resizable = false;
        o.launchAsync();
    }

    juce::AudioDeviceManager deviceManager;
    std::unique_ptr<AudioVisualizerProcessor> processor;
    std::unique_ptr<InputTap> tap;
    std::unique_ptr<MainWindow> window;
};

START_JUCE_APPLICATION(VisualizerApp)
