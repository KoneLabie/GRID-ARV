#include "VisualRenderer.h"
#include "Shaders.h"

using namespace juce::gl;

//==============================================================================
VisualRenderer::VisualRenderer(AudioVisualizerProcessor& p, juce::OpenGLContext& c, juce::Component& t)
    : proc(p), context(c), target(t)
{
    auto g = [&](const char* id) { return proc.apvts.getRawParameterValue(id); };
    P.mode = g(ParamID::mode);           P.palette = g(ParamID::palette);   P.sens = g(ParamID::sens);
    P.smooth = g(ParamID::smooth);       P.speed = g(ParamID::speed);       P.bright = g(ParamID::bright);
    P.trails = g(ParamID::trails);       P.zoom = g(ParamID::zoom);         P.rot = g(ParamID::rot);
    P.glow = g(ParamID::glow);           P.aberr = g(ParamID::aberr);       P.hue = g(ParamID::hue);
    P.hueSpin = g(ParamID::hueSpin);     P.quality = g(ParamID::quality);   P.autoBeats = g(ParamID::autoBeats);
    P.imgSlot = g(ParamID::imgSlot);     P.imgBeats = g(ParamID::imgBeats); P.imgMix = g(ParamID::imgMix);
    P.imgScale = g(ParamID::imgScale);   P.imgFit = g(ParamID::imgFit);       P.imgRotate = g(ParamID::imgRotate);
    P.imgFlipX = g(ParamID::imgFlipX);   P.imgFlipY = g(ParamID::imgFlipY);
    P.imgOffsetX = g(ParamID::imgOffsetX); P.imgOffsetY = g(ParamID::imgOffsetY);
    P.textOn = g(ParamID::textOn);       P.textLine = g(ParamID::textLine); P.textBeats = g(ParamID::textBeats);
    P.textSize = g(ParamID::textSize);   P.textReact = g(ParamID::textReact);
    P.textAnim = g(ParamID::textAnim);   P.textColor = g(ParamID::textColor);
    P.textPos = g(ParamID::textPos);     P.lyricOff = g(ParamID::lyricOff); P.followHost = g(ParamID::followHost);
}

void VisualRenderer::setError(const juce::String& e)
{
    DBG("Shader error: " << e);
    const juce::ScopedLock sl(errLock);
    lastError = e;
}

//==============================================================================
bool VisualRenderer::compile(Prog& p, const char* vs, const juce::String& fs, const char* attrName)
{
    auto program = std::make_unique<juce::OpenGLShaderProgram>(context);
    if (program->addVertexShader(juce::OpenGLHelpers::translateVertexShaderToV3(vs))
        && program->addFragmentShader(juce::OpenGLHelpers::translateFragmentShaderToV3(fs))
        && program->link())
    {
        p.attr = glGetAttribLocation(program->getProgramID(), attrName);
        p.cache.clear();
        p.prog = std::move(program);
        p.failed = false;
        return true;
    }
    setError(program->getLastError());
    p.failed = true;
    return false;
}

VisualRenderer::Prog* VisualRenderer::getModeProg(int base)
{
    auto& slot = modeProgs[base];
    if (slot == nullptr)
    {
        slot = std::make_unique<Prog>();
        const auto fs = juce::String(Shaders::kSceneHeader) + Shaders::kModeSource[base] + Shaders::kSceneMain;
        compile(*slot, Shaders::kQuadVertex, fs, "aPos");
    }
    return slot->failed ? nullptr : slot.get();
}

int VisualRenderer::loc(Prog& p, const char* name)
{
    for (auto& e : p.cache)
        if (e.first == name) return e.second;
    const int l = glGetUniformLocation(p.prog->getProgramID(), name);
    p.cache.emplace_back(name, l);
    return l;
}

//==============================================================================
void VisualRenderer::newOpenGLContextCreated()
{
    compile(trailProg, Shaders::kQuadVertex, Shaders::kTrailFrag, "aPos");
    compile(finalProg, Shaders::kQuadVertex, Shaders::kFinalFrag, "aPos");
    compile(textProg,  Shaders::kTextVertex, Shaders::kTextFrag, "aPos");
    compile(xyProg,    Shaders::kXYVertex, juce::String(Shaders::kSceneHeader) + Shaders::kXYMain, "aXY");

    const float quad[8] = { -1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f };
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glGenBuffers(1, &xyVBO);
    glBindBuffer(GL_ARRAY_BUFFER, xyVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (xyVerts.size() * sizeof(float)), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    std::vector<unsigned char> zeros(512 * 256 * 4, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &audioTex);
    glBindTexture(GL_TEXTURE_2D, audioTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint) GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint) GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint) GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint) GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint) GL_RGBA8, 512, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, zeros.data());

    glGenTextures(1, &histTex);
    glBindTexture(GL_TEXTURE_2D, histTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint) GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint) GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint) GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint) GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint) GL_RGBA8, 512, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, zeros.data());

    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    lastTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
    loadedSlot = loadedRev = -1;
    lastParamMode = -1;
}

void VisualRenderer::openGLContextClosing()
{
    for (auto& m : modeProgs) m.reset();
    xyProg.prog.reset(); trailProg.prog.reset(); finalProg.prog.reset(); textProg.prog.reset();
    sceneFB.release(); trailFB[0].release(); trailFB[1].release();
    imgTex.release(); textTex.release();
    fbW = fbH = 0;
    if (audioTex) { glDeleteTextures(1, &audioTex); audioTex = 0; }
    if (histTex)  { glDeleteTextures(1, &histTex);  histTex = 0; }
    if (quadVBO)  { glDeleteBuffers(1, &quadVBO);   quadVBO = 0; }
    if (xyVBO)    { glDeleteBuffers(1, &xyVBO);     xyVBO = 0; }
    hasImage = false; cachedText = {}; cachedFontRev = cachedTextRev = -1;
}

void VisualRenderer::ensureFBOs(int w, int h)
{
    if (w == fbW && h == fbH && sceneFB.isValid() && trailFB[0].isValid() && trailFB[1].isValid()) return;
    sceneFB.release(); trailFB[0].release(); trailFB[1].release();
    sceneFB.initialise(context, w, h);
    trailFB[0].initialise(context, w, h);
    trailFB[1].initialise(context, w, h);
    sceneFB.clear(juce::Colours::black);
    trailFB[0].clear(juce::Colours::black);
    trailFB[1].clear(juce::Colours::black);
    fbW = w; fbH = h; trailCur = 0;
}

void VisualRenderer::drawQuad(int attrib)
{
    if (attrib < 0) return;
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glEnableVertexAttribArray((GLuint) attrib);
    glVertexAttribPointer((GLuint) attrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray((GLuint) attrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//==============================================================================
void VisualRenderer::setCommonUniforms(Prog& p)
{
    const auto& an = proc.analyzer;
    const auto& pal = kPalettes[juce::jlimit(0, kNumPalettes - 1, (int) P.palette->load())];
    const ModeDef& md = kModes[curMode];

    glUniform2f(loc(p, "iRes"), (float) fbW, (float) fbH);
    glUniform1f(loc(p, "iTime"), sceneTime);
    glUniform1f(loc(p, "iBeat"), an.getBeat());
    glUniform1f(loc(p, "iBass"), an.getBass());
    glUniform1f(loc(p, "iMid"), an.getMid());
    glUniform1f(loc(p, "iTreb"), an.getTreble());
    glUniform1f(loc(p, "iLevel"), an.getLevel());
    glUniform4f(loc(p, "uP"), md.p[0], md.p[1], md.p[2], md.p[3]);
    glUniform4f(loc(p, "uQ"), md.q[0], md.q[1], md.q[2], md.q[3]);
    glUniform3f(loc(p, "uC0"), pal.c[0][0], pal.c[0][1], pal.c[0][2]);
    glUniform3f(loc(p, "uC1"), pal.c[1][0], pal.c[1][1], pal.c[1][2]);
    glUniform3f(loc(p, "uC2"), pal.c[2][0], pal.c[2][1], pal.c[2][2]);
    glUniform1f(loc(p, "uHue"), P.hue->load() * 6.2831853f + hueAcc);
    glUniform1f(loc(p, "uBright"), P.bright->load());
    glUniform1i(loc(p, "uAudio"), 0);
    glUniform1i(loc(p, "uHist"), 1);
    glUniform1i(loc(p, "uImage"), 2);
    glUniform1f(loc(p, "uHistRow"), (float) histRow);
    glUniform1f(loc(p, "uHasImage"), hasImage ? 1.f : 0.f);
    glUniform1f(loc(p, "uImgAspect"), imgAspect);
    glUniform1f(loc(p, "uImgScale"), P.imgScale->load());
    glUniform1f(loc(p, "uImgFit"), P.imgFit->load());
    const float imgAngle = P.imgRotate->load() * 0.01745329251994329577f;
    glUniform2f(loc(p, "uImgRotCS"), std::cos(imgAngle), std::sin(imgAngle));
    glUniform1f(loc(p, "uImgFlipX"), P.imgFlipX->load());
    glUniform1f(loc(p, "uImgFlipY"), P.imgFlipY->load());
    glUniform1f(loc(p, "uImgOffsetX"), P.imgOffsetX->load());
    glUniform1f(loc(p, "uImgOffsetY"), P.imgOffsetY->load());
    glUniform1fv(loc(p, "uAges"), 8, an.getAges().data());
    glUniform1fv(loc(p, "uSeeds"), 8, an.getSeeds().data());
}

void VisualRenderer::advanceMode()
{
    for (int t = 0; t < kNumModes; ++t)
    {
        curMode = (curMode + 1) % kNumModes;
        if (!isImageBase(kModes[curMode].base) || hasImage) break;
    }
}

void VisualRenderer::updateImage()
{
    const int n = proc.assets.getNumImages();
    int slot = (int) P.imgSlot->load();
    const int every = (int) P.imgBeats->load();
    if (every > 0 && n > 0) slot = (proc.analyzer.getBeatCount() / every) % n + 1;
    if (slot > n) slot = 0;

    const int rev = proc.assets.getImageRevision();
    if (slot == loadedSlot && rev == loadedRev) return;
    loadedSlot = slot; loadedRev = rev;

    hasImage = false;
    if (slot <= 0) return;

    auto img = juce::ImageFileFormat::loadFrom(proc.assets.getImageFile(slot - 1));
    if (!img.isValid()) return;

    // Bound GPU texture memory/upload cost; high-quality resampling only runs when the image changes.
    const int maxSide = 2048;
    if (juce::jmax(img.getWidth(), img.getHeight()) > maxSide)
    {
        const float k = (float) maxSide / (float) juce::jmax(img.getWidth(), img.getHeight());
        img = img.rescaled(juce::jmax(1, juce::roundToInt(img.getWidth() * k)),
                           juce::jmax(1, juce::roundToInt(img.getHeight() * k)), juce::Graphics::highResamplingQuality);
    }
    imgTex.loadImage(img);
    imgAspect = (float) img.getWidth() / (float) img.getHeight();
    hasImage = true;
}

void VisualRenderer::drawXY(const ModeDef& md)
{
    if (xyProg.failed || xyProg.prog == nullptr) return;
    const auto& an = proc.analyzer;
    const float aspect = (float) fbW / (float) fbH;
    const float scale = md.p[2] * 0.9f;
    const auto& wl = an.getWaveL();
    const auto& wr = an.getWaveR();
    const int n = AudioAnalyzer::kWave;
    for (int k = 0; k < n; ++k)
    {
        float x, y;
        if (md.p[1] < 0.5f) { x = wl[(size_t) k]; y = wr[(size_t) k]; }
        else { x = (wl[(size_t) k] - wr[(size_t) k]) * 0.7071f; y = (wl[(size_t) k] + wr[(size_t) k]) * 0.7071f; }
        xyVerts[(size_t) k * 3 + 0] = x * scale / aspect;
        xyVerts[(size_t) k * 3 + 1] = y * scale;
        xyVerts[(size_t) k * 3 + 2] = (float) k / (float) (n - 1);
    }

    xyProg.prog->use();
    setCommonUniforms(xyProg);
    glBindBuffer(GL_ARRAY_BUFFER, xyVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr) (xyVerts.size() * sizeof(float)), xyVerts.data());
    glEnableVertexAttribArray((GLuint) xyProg.attr);
    glVertexAttribPointer((GLuint) xyProg.attr, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    const float px = 1.3f;
    const float offs[5][2] = { { 0, 0 }, { px, 0 }, { -px, 0 }, { 0, px }, { 0, -px } };
    for (auto& o : offs)
    {
        glUniform2f(loc(xyProg, "uOff"), o[0] * 2.f / (float) fbW, o[1] * 2.f / (float) fbH);
        glDrawArrays(GL_LINE_STRIP, 0, n);
    }
    glDisable(GL_BLEND);
    glDisableVertexAttribArray((GLuint) xyProg.attr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//==============================================================================
void VisualRenderer::renderOpenGL()
{
    jassert(juce::OpenGLHelpers::isContextActive());

    // ---- timing ----
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    float dt = juce::jlimit(0.0005f, 0.1f, (float) (now - lastTime));
    lastTime = now;
    realTime += dt;
    fpsAcc += dt; ++fpsFrames;
    if (fpsAcc >= 0.5f) { fps.store((float) fpsFrames / fpsAcc); fpsAcc = 0.f; fpsFrames = 0; }

    // ---- viewport / FBO size ----
    const double scale = context.getRenderingScale();
    vpW = juce::jmax(2, juce::roundToInt(target.getWidth() * scale));
    vpH = juce::jmax(2, juce::roundToInt(target.getHeight() * scale));
    static const float kQ[3] = { 0.5f, 0.75f, 1.0f };
    const float q = kQ[juce::jlimit(0, 2, (int) P.quality->load())];
    ensureFBOs(juce::jmax(2, juce::roundToInt(vpW * q)), juce::jmax(2, juce::roundToInt(vpH * q)));

    // ---- audio analysis ----
    auto& an = proc.analyzer;
    an.update(dt, P.sens->load(), P.smooth->load());

    // advance the spectrogram history at a fixed 60 rows / second
    histAcc += dt;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    int guard = 0;
    while (histAcc >= (1.f / 60.f) && guard++ < 4)
    {
        histAcc -= 1.f / 60.f;
        histRow = (histRow + 1) & 255;
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, histTex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, histRow, 512, 1, GL_RGBA, GL_UNSIGNED_BYTE, an.histRowData());
    }
    if (histAcc > 0.1f) histAcc = 0.f;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, audioTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 512, 4, GL_RGBA, GL_UNSIGNED_BYTE, an.texData());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    // ---- time, hue ----
    sceneTime += dt * P.speed->load();
    hueAcc += dt * P.hueSpin->load() * 0.5f;

    // ---- mode selection (UI request / host automation / auto-cycle on beats) ----
    updateImage();
    const int req = proc.modeRequest.exchange(-1);
    if (req >= 0) { curMode = req; lastParamMode = (int) std::lround(P.mode->load()); }
    const int pm = (int) std::lround(P.mode->load());
    if (pm != lastParamMode) { curMode = pm; lastParamMode = pm; }
    curMode = juce::jlimit(0, kNumModes - 1, curMode);

    const int bc = an.getBeatCount();
    if (lastBeatCount < 0) lastBeatCount = bc;
    const int autoEvery = (int) P.autoBeats->load();
    if (bc != lastBeatCount)
    {
        if (autoEvery > 0)
        {
            beatsAccum += bc - lastBeatCount;
            if (beatsAccum >= autoEvery) { beatsAccum = 0; advanceMode(); }
        }
        lastBeatCount = bc;
    }
    if (autoEvery == 0) beatsAccum = 0;
    effMode.store(curMode);

    // ---- common GL state ----
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, audioTex);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, histTex);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, hasImage ? imgTex.getTextureID() : 0);

    // ---- pass 1: mode -> scene FBO ----
    sceneFB.makeCurrentRenderingTarget();
    glViewport(0, 0, fbW, fbH);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    const ModeDef& md = kModes[curMode];
    if (md.base == B_XY)
        drawXY(md);
    else if (auto* pr = getModeProg(md.base))
    {
        pr->prog->use();
        setCommonUniforms(*pr);
        drawQuad(pr->attr);
    }
    sceneFB.releaseAsRenderingTarget();

    // ---- pass 2: trails / feedback ----
    const int latest = trailCur;
    {
        auto& dst = trailFB[trailCur];
        auto& srcFb = trailFB[1 - trailCur];
        dst.makeCurrentRenderingTarget();
        glViewport(0, 0, fbW, fbH);
        if (trailProg.prog != nullptr)
        {
            trailProg.prog->use();
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, sceneFB.getTextureID());
            glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, srcFb.getTextureID());
            glUniform1i(loc(trailProg, "uScene"), 0);
            glUniform1i(loc(trailProg, "uPrev"), 1);
            // trail length is defined per frame at 60 fps, so correct it for the real frame time
            const float decay = std::pow(juce::jlimit(0.f, 0.97f, P.trails->load()), dt * 60.f);
            glUniform1f(loc(trailProg, "uDecay"), decay);
            glUniform1f(loc(trailProg, "uZoom"), P.zoom->load() * dt * 60.f);
            glUniform1f(loc(trailProg, "uRotA"), P.rot->load() * dt * 60.f);
            glUniform1f(loc(trailProg, "uHueD"), P.hueSpin->load() * 0.02f * dt * 60.f);
            drawQuad(trailProg.attr);
        }
        dst.releaseAsRenderingTarget();
        trailCur = 1 - trailCur;
    }

    // ---- pass 3: final -> screen ----
    glBindFramebuffer(GL_FRAMEBUFFER, context.getFrameBufferID());
    glViewport(0, 0, vpW, vpH);
    if (finalProg.prog != nullptr)
    {
        finalProg.prog->use();
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, trailFB[latest].getTextureID());
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, hasImage ? imgTex.getTextureID() : 0);
        glUniform1i(loc(finalProg, "uTex"), 0);
        glUniform1i(loc(finalProg, "uImage"), 1);
        glUniform2f(loc(finalProg, "uTexel"), 1.f / (float) fbW, 1.f / (float) fbH);
        glUniform1f(loc(finalProg, "uGlow"), P.glow->load());
        glUniform1f(loc(finalProg, "uAberr"), P.aberr->load());
        glUniform1f(loc(finalProg, "uBeat"), an.getBeat());
        glUniform1f(loc(finalProg, "uVig"), 0.35f);
        glUniform1f(loc(finalProg, "uImgMix"), isImageBase(md.base) ? 0.f : P.imgMix->load());
        glUniform1f(loc(finalProg, "uHasImage"), hasImage ? 1.f : 0.f);
        glUniform1f(loc(finalProg, "uImgAspect"), imgAspect);
        glUniform1f(loc(finalProg, "uAspect"), (float) vpW / (float) vpH);
        glUniform1f(loc(finalProg, "uImgScale"), P.imgScale->load());
        glUniform1f(loc(finalProg, "uImgFit"), P.imgFit->load());
        const float imgAngle = P.imgRotate->load() * 0.01745329251994329577f;
        glUniform2f(loc(finalProg, "uImgRotCS"), std::cos(imgAngle), std::sin(imgAngle));
        glUniform1f(loc(finalProg, "uImgFlipX"), P.imgFlipX->load());
        glUniform1f(loc(finalProg, "uImgFlipY"), P.imgFlipY->load());
        glUniform1f(loc(finalProg, "uImgOffsetX"), P.imgOffsetX->load());
        glUniform1f(loc(finalProg, "uImgOffsetY"), P.imgOffsetY->load());
        drawQuad(finalProg.attr);
    }

    // ---- pass 4: text ----
    drawText(vpW, vpH, dt);

    // ---- leave GL in a clean state for JUCE's own painting ----
    glDisable(GL_BLEND);
    for (int u = 2; u >= 0; --u) { glActiveTexture(GL_TEXTURE0 + (GLenum) u); glBindTexture(GL_TEXTURE_2D, 0); }
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(0);
}

//==============================================================================
static float easeOutBack(float t)
{
    const float c1 = 1.70158f, c3 = c1 + 1.f;
    const float u = t - 1.f;
    return 1.f + c3 * u * u * u + c1 * u * u;
}

void VisualRenderer::drawText(int vw, int vh, float dt)
{
    if (P.textOn->load() < 0.5f || textProg.prog == nullptr) return;
    const auto& an = proc.analyzer;

    // which line?
    double clock;
    if (P.followHost->load() > 0.5f && proc.hostHasTime.load())
        clock = proc.hostTime.load();
    else
    {
        if (proc.lyricRunning.load()) proc.lyricClock.store(proc.lyricClock.load() + (double) dt);
        clock = proc.lyricClock.load();
    }
    clock += (double) P.lyricOff->load();
    const double beatPos = proc.hostPlaying.load() ? proc.hostPpq.load() : (double) an.getBeatCount();

    const int idx = proc.text.pickLine((int) P.textLine->load(), clock, beatPos, (int) P.textBeats->load());
    if (idx != lastLine) { lastLine = idx; lineStart = realTime; }
    if (idx < 0) return;

    const auto s = proc.text.getLineText(idx);
    if (s.isEmpty()) return;

    // (re)build the text texture only when something changed
    const int fontRev = proc.text.getFontRevision();
    const int textRev = proc.text.getTextRevision();
    if (s != cachedText || fontRev != cachedFontRev || textRev != cachedTextRev)
    {
        auto img = proc.text.renderImage(s, 128.0f, 2000);
        textTex.loadImage(img);
        textW = img.getWidth(); textH = img.getHeight();
        cachedText = s; cachedFontRev = fontRev; cachedTextRev = textRev;
    }

    // animation
    const int anim = (int) P.textAnim->load();
    const float age = realTime - lineStart;
    const float react = P.textReact->load();
    const float beat = an.getBeat();
    float alpha = 1.f, scaleMul = 1.f + 0.10f * react * beat, rot = 0.f, offX = 0.f, offY = 0.f;
    float reveal = 2.f, glitch = 0.f;
    const float tIn = juce::jlimit(0.f, 1.f, age / 0.35f);

    switch (anim)
    {
        case 1: alpha = tIn; break;
        case 2: scaleMul *= juce::jmax(0.01f, easeOutBack(tIn)); alpha = juce::jlimit(0.f, 1.f, age / 0.1f); break;
        case 3: offY = (1.f - tIn * (2.f - tIn)) * -0.15f; alpha = tIn; break;
        case 4:
        {
            const float len = (float) juce::jmax(1, s.length());
            const float r = juce::jlimit(0.f, 1.f, age / (0.05f * len + 0.25f));
            reveal = (r >= 1.f) ? 2.f : (28.f + r * ((float) textW - 56.f)) / (float) textW;
            break;
        }
        case 5:
            offX = std::sin(realTime * 53.f) * std::cos(realTime * 37.f) * 0.03f * (0.3f + react) * beat;
            offY = std::cos(realTime * 61.f) * 0.02f * (0.3f + react) * beat;
            break;
        case 6: glitch = juce::jlimit(0.f, 1.f, beat * (0.4f + react) + (age < 0.25f ? 1.f - age / 0.25f : 0.f)); break;
        case 7: rot = 0.06f * std::sin(realTime * 2.f) * (0.5f + beat); scaleMul *= 1.f + 0.03f * std::sin(realTime * 3.f); break;
        default: break;
    }

    // geometry (pixels -> normalised device coordinates)
    float sc = P.textSize->load() * (float) vh / 128.0f * scaleMul;
    float qw = (float) textW * sc, qh = (float) textH * sc;
    const float maxW = (float) vw * 0.96f;
    if (qw > maxW) { const float k = maxW / qw; sc *= k; qw *= k; qh *= k; }
    const int pos = (int) P.textPos->load();
    const float cy = (pos == 0 ? 0.72f : (pos == 2 ? -0.72f : 0.0f)) + offY;
    const float aspect = (float) vw / (float) vh;

    textProg.prog->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textTex.getTextureID());
    glUniform1i(loc(textProg, "uText"), 0);
    glUniform2f(loc(textProg, "uCenter"), offX, cy);
    glUniform2f(loc(textProg, "uHalf"), qw / (float) vh, qh / (float) vh);
    glUniform1f(loc(textProg, "uRot"), rot);
    glUniform1f(loc(textProg, "uAspect"), aspect);
    glUniform2f(loc(textProg, "uTexel"), 1.f / (float) textW, 1.f / (float) textH);
    glUniform1f(loc(textProg, "uAlpha"), alpha);
    glUniform1f(loc(textProg, "uReveal"), reveal);
    glUniform1f(loc(textProg, "uGlitch"), glitch);
    glUniform1f(loc(textProg, "uTime"), realTime);
    glUniform1f(loc(textProg, "uColorMode"), P.textColor->load());

    const auto& pal = kPalettes[juce::jlimit(0, kNumPalettes - 1, (int) P.palette->load())];
    glUniform3f(loc(textProg, "uC0"), pal.c[0][0], pal.c[0][1], pal.c[0][2]);
    glUniform3f(loc(textProg, "uC1"), pal.c[1][0], pal.c[1][1], pal.c[1][2]);
    glUniform3f(loc(textProg, "uC2"), pal.c[2][0], pal.c[2][1], pal.c[2][2]);
    glUniform1f(loc(textProg, "uHue"), P.hue->load() * 6.2831853f + hueAcc);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);   // premultiplied alpha output
    drawQuad(textProg.attr);
    glDisable(GL_BLEND);
}
