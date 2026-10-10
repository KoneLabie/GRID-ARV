#pragma once
#include <juce_opengl/juce_opengl.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>
#include "PluginProcessor.h"

// Renders everything on the OpenGL thread:
//   1. mode shader        -> scene FBO        (resolution scaled by the Quality setting)
//   2. trails/feedback    -> ping-pong FBO
//   3. final pass         -> screen           (glow, aberration, background image, vignette)
//   4. lyrics/text quad   -> screen
class VisualRenderer : public juce::OpenGLRenderer
{
public:
    VisualRenderer(AudioVisualizerProcessor& p, juce::OpenGLContext& c, juce::Component& targetComponent);
    ~VisualRenderer() override = default;

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    int getEffectiveMode() const noexcept { return effMode.load(); }
    float getFps() const noexcept { return fps.load(); }
    juce::String getShaderError() const { const juce::ScopedLock sl(errLock); return lastError; }

private:
    struct Prog
    {
        std::unique_ptr<juce::OpenGLShaderProgram> prog;
        std::vector<std::pair<const char*, int>> cache;   // uniform locations, keyed by literal pointer
        int attr = -1;
        bool failed = false;
    };

    bool compile(Prog& p, const char* vs, const juce::String& fs, const char* attrName);
    Prog* getModeProg(int base);
    int  loc(Prog& p, const char* name);
    void setCommonUniforms(Prog& p);
    void drawQuad(int attrib);
    void ensureFBOs(int w, int h);
    void updateImage();
    void advanceMode();
    void drawXY(const ModeDef& md);
    void drawText(int vw, int vh, float dt);
    void setError(const juce::String& e);

    AudioVisualizerProcessor& proc;
    juce::OpenGLContext& context;
    juce::Component& target;

    struct ParamPtrs
    {
        std::atomic<float> *mode, *palette, *sens, *smooth, *speed, *bright, *trails, *zoom, *rot, *glow, *aberr,
            *hue, *hueSpin, *quality, *autoBeats, *imgSlot, *imgBeats, *imgMix, *textOn, *textLine, *textBeats,
            *textSize, *textReact, *textAnim, *textColor, *textPos, *lyricOff, *followHost;
    } P {};

    std::unique_ptr<Prog> modeProgs[B_COUNT];
    Prog xyProg, trailProg, finalProg, textProg;

    unsigned int quadVBO = 0, xyVBO = 0, audioTex = 0, histTex = 0;
    juce::OpenGLFrameBuffer sceneFB, trailFB[2];
    int fbW = 0, fbH = 0, trailCur = 0;

    juce::OpenGLTexture imgTex;
    bool hasImage = false;
    float imgAspect = 1.0f;
    int loadedSlot = -1, loadedRev = -1;

    juce::OpenGLTexture textTex;
    int textW = 0, textH = 0, cachedFontRev = -1, cachedTextRev = -1;
    juce::String cachedText;

    double lastTime = 0.0;
    float sceneTime = 0.f, realTime = 0.f, hueAcc = 0.f, histAcc = 0.f, fpsAcc = 0.f;
    int fpsFrames = 0, histRow = 0;
    int curMode = 0, lastParamMode = -1, lastBeatCount = -1, beatsAccum = 0;
    int lastLine = -2;
    float lineStart = 0.f;
    int vpW = 1, vpH = 1;       // on-screen size in pixels
    float uBrightCur = 1.f;

    std::atomic<int> effMode { 0 };
    std::atomic<float> fps { 0.f };
    mutable juce::CriticalSection errLock;
    juce::String lastError;

    std::array<float, 512 * 3> xyVerts {};
};
