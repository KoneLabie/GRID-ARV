#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

//==============================================================================
// AssetLibrary
//==============================================================================
static const char* kImageWild = "*.png;*.jpg;*.jpeg;*.gif";
static const char* kFontWild  = "*.ttf;*.otf;*.ttc";

AssetLibrary::AssetLibrary()
{
    root = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("GridARV");
    root.getChildFile("Images").createDirectory();
    root.getChildFile("Fonts").createDirectory();
    refresh();
}

static void scan(const juce::File& dir, const char* wild, juce::Array<juce::File>& out)
{
    out.clear();
    dir.findChildFiles(out, juce::File::findFiles, false, wild);
    std::sort(out.begin(), out.end(), [](const juce::File& a, const juce::File& b)
              { return a.getFileName().compareNatural(b.getFileName()) < 0; });
}

void AssetLibrary::refresh()
{
    const juce::ScopedLock sl(cs);
    scan(root.getChildFile("Images"), kImageWild, images);
    scan(root.getChildFile("Fonts"), kFontWild, fonts);
    ++imageRevision;
}

bool AssetLibrary::isImageFile(const juce::File& f)
{
    return f.hasFileExtension("png;jpg;jpeg;gif");
}
bool AssetLibrary::isFontFile(const juce::File& f) { return f.hasFileExtension("ttf;otf;ttc"); }
bool AssetLibrary::isTextFile(const juce::File& f) { return f.hasFileExtension("lrc;srt;txt"); }

juce::File AssetLibrary::importFile(const juce::File& src, bool isFont)
{
    const auto dir = root.getChildFile(isFont ? "Fonts" : "Images");
    auto dest = dir.getChildFile(src.getFileName());

    if (dest != src)
    {
        if (dest.existsAsFile() && dest.getSize() != src.getSize())
            dest = dest.getNonexistentSibling();
        if (!dest.existsAsFile())
            src.copyFileTo(dest);
    }
    refresh();
    return dest;
}

int AssetLibrary::getNumImages() const { const juce::ScopedLock sl(cs); return images.size(); }
int AssetLibrary::getNumFonts() const { const juce::ScopedLock sl(cs); return fonts.size(); }
juce::File AssetLibrary::getImageFile(int i) const { const juce::ScopedLock sl(cs); return images[i]; }
juce::File AssetLibrary::getFontFile(int i) const { const juce::ScopedLock sl(cs); return fonts[i]; }

juce::StringArray AssetLibrary::getImageNames() const
{
    const juce::ScopedLock sl(cs);
    juce::StringArray s; for (auto& f : images) s.add(f.getFileName()); return s;
}
juce::StringArray AssetLibrary::getFontNames() const
{
    const juce::ScopedLock sl(cs);
    juce::StringArray s; for (auto& f : fonts) s.add(f.getFileName()); return s;
}
int AssetLibrary::indexOfImage(const juce::String& n) const { return getImageNames().indexOf(n); }
int AssetLibrary::indexOfFont(const juce::String& n) const { return getFontNames().indexOf(n); }

//==============================================================================
// Processor
//==============================================================================
AudioVisualizerProcessor::AudioVisualizerProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                         .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts(*this, nullptr, "STATE", createParameterLayout())
{
    srcParam = apvts.getRawParameterValue(ParamID::source);
}

void AudioVisualizerProcessor::prepareToPlay(double sampleRate, int)
{
    analyzer.setSampleRate(sampleRate);
}

bool AudioVisualizerProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (out != l.getMainInputChannelSet()) return false;
    if (l.inputBuses.size() > 1)
    {
        const auto sc = l.getChannelSet(true, 1);
        if (!sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo())
            return false;
    }
    return true;
}

void AudioVisualizerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();

    // Pick the analysis source. The audio itself is passed through untouched.
    const bool useSC = srcParam != nullptr && srcParam->load() > 0.5f && getBusCount(true) > 1
                       && getBus(true, 1) != nullptr && getBus(true, 1)->isEnabled();
    auto src = getBusBuffer(buffer, true, useSC ? 1 : 0);
    if (src.getNumChannels() > 0)
        analyzer.push(src.getReadPointer(0), src.getNumChannels() > 1 ? src.getReadPointer(1) : src.getReadPointer(0), n);

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            hostPlaying.store(pos->getIsPlaying());
            if (auto t = pos->getTimeInSeconds()) { hostTime.store(*t); hostHasTime.store(true); }
            if (auto p = pos->getPpqPosition()) hostPpq.store(*p);
            if (auto b = pos->getBpm()) hostBpm.store(*b);
        }
}

juce::AudioProcessorEditor* AudioVisualizerProcessor::createEditor()
{
    return new AudioVisualizerEditor(*this);
}

//------------------------------------------------------------------------------
void AudioVisualizerProcessor::setParamPlain(const char* id, float v)
{
    if (auto* p = apvts.getParameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(v));
}

float AudioVisualizerProcessor::getParamPlain(const char* id) const
{
    if (auto* p = apvts.getRawParameterValue(id)) return p->load();
    return 0.0f;
}

void AudioVisualizerProcessor::requestMode(int modeIndex)
{
    modeIndex = juce::jlimit(0, kNumModes - 1, modeIndex);
    setParamPlain(ParamID::mode, (float) modeIndex);
    modeRequest.store(modeIndex);
}

void AudioVisualizerProcessor::applyFontKey(const juce::String& key)
{
    if (key.startsWith("file:"))
    {
        const int i = assets.indexOfFont(key.substring(5));
        if (i >= 0)
        {
            juce::MemoryBlock mb;
            if (assets.getFontFile(i).loadFileAsData(mb))
            {
                auto tf = juce::Typeface::createSystemTypefaceFor(mb.getData(), mb.getSize());
                if (tf != nullptr) { text.setFont(key, tf, {}); return; }
            }
        }
        text.setFont({}, nullptr, {});
    }
    else if (key.startsWith("sys:"))
        text.setFont(key, nullptr, key.substring(4));
    else
        text.setFont({}, nullptr, {});
}

//------------------------------------------------------------------------------
void AudioVisualizerProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty("text", text.getRawText(), nullptr);
    state.setProperty("fontKey", text.getFontKey(), nullptr);
    const int slot = (int) getParamPlain(ParamID::imgSlot);
    state.setProperty("imageName", (slot > 0 && slot <= assets.getNumImages()) ? assets.getImageFile(slot - 1).getFileName() : juce::String(), nullptr);
    state.setProperty("edW", editorW.load(), nullptr);
    state.setProperty("edH", editorH.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, dest);
}

void AudioVisualizerProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml == nullptr || !xml->hasTagName(apvts.state.getType())) return;

    auto vt = juce::ValueTree::fromXml(*xml);
    apvts.replaceState(vt);

    text.setRawText(vt.getProperty("text").toString());
    applyFontKey(vt.getProperty("fontKey").toString());

    const auto imageName = vt.getProperty("imageName").toString();
    if (imageName.isNotEmpty())
    {
        const int i = assets.indexOfImage(imageName);
        setParamPlain(ParamID::imgSlot, i >= 0 ? (float) (i + 1) : 0.0f);
    }
    if (vt.hasProperty("edW")) editorW.store((int) vt.getProperty("edW"));
    if (vt.hasProperty("edH")) editorH.store((int) vt.getProperty("edH"));
}

//------------------------------------------------------------------------------
#ifndef VIS_STANDALONE_APP
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioVisualizerProcessor();
}
#endif
