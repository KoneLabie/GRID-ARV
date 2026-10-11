#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "Params.h"
#include "AudioAnalyzer.h"
#include "TextEngine.h"

// User images and fonts live in <app data>/GridARV/{Images,Fonts}.
// Dropped files are copied there so projects keep working if the original file moves.
class AssetLibrary
{
public:
    AssetLibrary();

    void refresh();
    juce::File importFile(const juce::File& src, bool isFont);

    int getNumImages() const;
    juce::File getImageFile(int i) const;
    juce::StringArray getImageNames() const;
    int indexOfImage(const juce::String& fileName) const;

    int getNumFonts() const;
    juce::File getFontFile(int i) const;
    juce::StringArray getFontNames() const;
    int indexOfFont(const juce::String& fileName) const;

    int getImageRevision() const { return imageRevision.load(); }
    juce::File getRoot() const { return root; }

    static bool isImageFile(const juce::File& f);
    static bool isFontFile(const juce::File& f);
    static bool isTextFile(const juce::File& f);

private:
    mutable juce::CriticalSection cs;
    juce::File root;
    juce::Array<juce::File> images, fonts;
    std::atomic<int> imageRevision { 0 };
};

class AudioVisualizerProcessor : public juce::AudioProcessor
{
public:
    AudioVisualizerProcessor();
    ~AudioVisualizerProcessor() override = default;

    // --- AudioProcessor ---
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Grid ARV"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // --- helpers used by the UI / renderer ---
    void setParamPlain(const char* id, float plainValue);
    float getParamPlain(const char* id) const;
    void requestMode(int modeIndex);          // UI -> renderer (also updates the host parameter)
    void applyFontKey(const juce::String& key); // "" | "sys:Name" | "file:Name.ttf"

    juce::AudioProcessorValueTreeState apvts;
    AudioAnalyzer analyzer;
    TextEngine text;
    AssetLibrary assets;

    // Host transport info (written on the audio thread, read on the GL thread)
    std::atomic<bool>   hostHasTime { false }, hostPlaying { false };
    std::atomic<double> hostTime { 0.0 }, hostPpq { 0.0 }, hostBpm { 120.0 };

    // Internal lyric clock (standalone, or when the host gives no time)
    std::atomic<bool>   lyricRunning { false };
    std::atomic<double> lyricClock { 0.0 };

    std::atomic<int> modeRequest { -1 };
    std::atomic<int> editorW { 1000 }, editorH { 600 };
    bool standalone = false;   // set by the standalone app

private:
    std::atomic<float>* srcParam = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioVisualizerProcessor)
};
