#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>
#include <functional>
#include "PluginProcessor.h"
#include "VisualRenderer.h"

// The editor IS the OpenGL view. JUCE draws the control panel (normal components) on top of it.
// Used for the VST3/AU/VST2 plugin window and, unchanged, as the content of the standalone window.
class AudioVisualizerEditor : public juce::AudioProcessorEditor,
                              public juce::FileDragAndDropTarget,
                              private juce::Timer
{
public:
    explicit AudioVisualizerEditor(AudioVisualizerProcessor&);
    ~AudioVisualizerEditor() override;

    void paint(juce::Graphics&) override {}
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;

    // Set by the standalone app
    std::function<void()> onToggleFullscreen, onOpenAudioSettings;

    // Used by the panel
    void handleFile(const juce::File& f);
    int  currentMode() const { return renderer != nullptr ? renderer->getEffectiveMode() : 0; }
    void stepMode(int delta);
    void randomMode();
    void togglePanel();
    juce::String getStatusText() const;

private:
    class Panel;
    void timerCallback() override;

    AudioVisualizerProcessor& proc;
    juce::OpenGLContext glContext;
    std::unique_ptr<VisualRenderer> renderer;
    std::unique_ptr<Panel> panel;
    juce::Viewport viewport;
    juce::TextButton panelButton { "UI" };
    juce::uint32 lastMoveMs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioVisualizerEditor)
};
