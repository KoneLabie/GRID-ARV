#pragma once
#include "VisualModes.h"

// Shaders are written in legacy GLSL (attribute/varying/texture2D/gl_FragColor);
// JUCE translates them to GLSL 1.50 core automatically (OpenGLHelpers::translate*ToV3).
// Avoid the words "varying", "texture2D" and "gl_FragColor" inside identifiers or comments.
// Every string literal is kept well below MSVC's 16 KB literal limit.

namespace Shaders
{
// ---------------------------------------------------------------- shared vertex shader
inline const char* const kQuadVertex = R"GLSL(
attribute vec2 aPos;
varying vec2 vUV;
void main()
{
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)GLSL";

// ---------------------------------------------------------------- common header for mode shaders
inline const char* const kSceneHeader = R"GLSL(
uniform vec2 iRes;
uniform float iTime, iBeat, iBass, iMid, iTreb, iLevel;
uniform vec4 uP, uQ;
uniform vec3 uC0, uC1, uC2;
uniform float uHue, uBright;
uniform sampler2D uAudio, uHist, uImage;
uniform float uHistRow, uHasImage, uImgAspect;
uniform float uAges[8];
uniform float uSeeds[8];

#define PI 3.14159265359
#define TAU 6.28318530718
#define ASPECT (iRes.x / iRes.y)

float hash11(float p){ p = fract(p * 0.1031); p *= p + 33.33; p *= p + p; return fract(p); }
float hash21(vec2 p){ vec3 p3 = fract(vec3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.x + p3.y) * p3.z); }
vec2 hash22(vec2 p){ vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973)); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.xx + p3.yz) * p3.zy); }
float vnoise(vec2 p){
    vec2 i = floor(p); vec2 f = fract(p); f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash21(i), hash21(i + vec2(1.0, 0.0)), f.x),
               mix(hash21(i + vec2(0.0, 1.0)), hash21(i + vec2(1.0, 1.0)), f.x), f.y);
}
float fbm(vec2 p){
    float v = 0.0; float a = 0.5;
    for (int i = 0; i < 5; i++){ v += a * vnoise(p); p = p * 2.03 + vec2(17.1, 9.2); a *= 0.5; }
    return v;
}
mat2 rot2(float a){ float c = cos(a); float s = sin(a); return mat2(c, -s, s, c); }

float unpack16(vec4 c){ return (floor(c.r * 255.0 + 0.5) * 256.0 + floor(c.g * 255.0 + 0.5)) / 65535.0; }
float fetchA(float row, float x){
    float px = clamp(x, 0.0, 1.0) * 511.0;
    float i0 = floor(px); float f = px - i0; float i1 = min(i0 + 1.0, 511.0);
    float v0 = unpack16(texture2D(uAudio, vec2((i0 + 0.5) / 512.0, (row + 0.5) / 4.0)));
    float v1 = unpack16(texture2D(uAudio, vec2((i1 + 0.5) / 512.0, (row + 0.5) / 4.0)));
    return mix(v0, v1, f);
}
float spec(float x){ return fetchA(0.0, x); }
float waveL(float x){ return fetchA(1.0, x) * 2.0 - 1.0; }
float waveR(float x){ return fetchA(2.0, x) * 2.0 - 1.0; }
float hist(float x, float age){
    float row = mod(uHistRow - age, 256.0);
    return texture2D(uHist, vec2(clamp(x, 0.0, 1.0), (row + 0.5) / 256.0)).r;
}

vec3 hueRot(vec3 c, float a){
    float s = sin(a); float co = cos(a); vec3 k = vec3(0.57735027);
    return c * co + cross(k, c) * s + k * dot(k, c) * (1.0 - co);
}
vec3 pal(float t){
    t = fract(t); float x = t * 3.0; vec3 c;
    if (x < 1.0) c = mix(uC0, uC1, smoothstep(0.0, 1.0, x));
    else if (x < 2.0) c = mix(uC1, uC2, smoothstep(0.0, 1.0, x - 1.0));
    else c = mix(uC2, uC0, smoothstep(0.0, 1.0, x - 2.0));
    return hueRot(c, uHue);
}
float polyD(vec2 p, float n){ float a = atan(p.y, p.x); float k = TAU / n; return length(p) * cos(floor(0.5 + a / k) * k - a); }
float gl1(float v){ return 0.5 - abs(fract(v) - 0.5); }
)GLSL";

// main() for every normal mode: render(uv, st) is supplied by the mode
inline const char* const kSceneMain = R"GLSL(
void main()
{
    vec2 st = gl_FragCoord.xy / iRes;
    vec2 uv = (gl_FragCoord.xy - 0.5 * iRes) / iRes.y;
    vec3 c = render(uv, st);
    gl_FragColor = vec4(clamp(c * uBright, 0.0, 4.0), 1.0);
}
)GLSL";

// ---------------------------------------------------------------- XY / phase scope (line geometry)
inline const char* const kXYVertex = R"GLSL(
attribute vec3 aXY;
varying float vT;
uniform vec2 uOff;
void main()
{
    vT = aXY.z;
    gl_Position = vec4(aXY.xy + uOff, 0.0, 1.0);
}
)GLSL";

inline const char* const kXYMain = R"GLSL(
varying float vT;
void main()
{
    vec3 c = pal(vT * 0.8 + iTime * 0.05) * (0.22 + 0.3 * iLevel + 0.3 * iBeat);
    gl_FragColor = vec4(c * uBright, 1.0);
}
)GLSL";

// ---------------------------------------------------------------- feedback / trails pass
inline const char* const kTrailFrag = R"GLSL(
varying vec2 vUV;
uniform sampler2D uScene;
uniform sampler2D uPrev;
uniform float uDecay, uZoom, uRotA, uHueD;
vec3 hueRot(vec3 c, float a){
    float s = sin(a); float co = cos(a); vec3 k = vec3(0.57735027);
    return c * co + cross(k, c) * s + k * dot(k, c) * (1.0 - co);
}
void main()
{
    vec3 s = texture2D(uScene, vUV).rgb;
    vec2 p = vUV - 0.5;
    float c = cos(uRotA); float sn = sin(uRotA);
    p = vec2(c * p.x - sn * p.y, sn * p.x + c * p.y) / (1.0 + uZoom) + 0.5;
    vec3 prev = texture2D(uPrev, p).rgb;
    prev = hueRot(prev, uHueD);
    prev = max(prev * uDecay - 1.5 / 255.0, 0.0);
    gl_FragColor = vec4(max(s, prev), 1.0);
}
)GLSL";

// ---------------------------------------------------------------- final pass: glow, aberration, bg image, vignette
inline const char* const kFinalFrag = R"GLSL(
varying vec2 vUV;
uniform sampler2D uTex;
uniform sampler2D uImage;
uniform vec2 uTexel;
uniform float uGlow, uAberr, uBeat, uVig, uImgMix, uHasImage, uImgAspect, uAspect;
void main()
{
    vec2 uv = vUV;
    vec3 c;
    if (uAberr > 0.001) {
        float o = uAberr * (0.002 + 0.012 * uBeat);
        vec2 d = (uv - 0.5) * o * 2.0;
        c = vec3(texture2D(uTex, uv + d).r, texture2D(uTex, uv).g, texture2D(uTex, uv - d).b);
    } else {
        c = texture2D(uTex, uv).rgb;
    }
    if (uGlow > 0.001) {
        vec3 g = vec3(0.0);
        for (int i = 0; i < 8; i++) {
            float a = float(i) * 0.785398;
            vec2 o = vec2(cos(a), sin(a));
            g += texture2D(uTex, uv + o * uTexel * 6.0).rgb;
            g += texture2D(uTex, uv + o * uTexel * 16.0).rgb * 0.7;
            g += texture2D(uTex, uv + o * uTexel * 34.0).rgb * 0.5;
        }
        c += g * uGlow * 0.075;
    }
    if (uImgMix > 0.001 && uHasImage > 0.5) {
        vec2 s = vUV - 0.5;
        if (uAspect > uImgAspect) s.y *= uImgAspect / uAspect; else s.x *= uAspect / uImgAspect;
        s /= 1.0 + 0.04 * uBeat;
        vec3 im = texture2D(uImage, clamp(vec2(s.x + 0.5, 0.5 - s.y), 0.0, 1.0)).rgb;
        float l = dot(c, vec3(0.299, 0.587, 0.114));
        c += im * uImgMix * (1.0 - clamp(l * 1.2, 0.0, 1.0));
    }
    float vg = 1.0 - uVig * smoothstep(0.35, 0.95, length((vUV - 0.5) * vec2(uAspect, 1.0)));
    gl_FragColor = vec4(c * vg, 1.0);
}
)GLSL";

// ---------------------------------------------------------------- text overlay
inline const char* const kTextVertex = R"GLSL(
attribute vec2 aPos;
varying vec2 vUV;
uniform vec2 uCenter;
uniform vec2 uHalf;
uniform float uRot;
uniform float uAspect;
void main()
{
    vUV = aPos * 0.5 + 0.5;
    vec2 p = aPos * uHalf;
    float c = cos(uRot); float s = sin(uRot);
    p = vec2(c * p.x - s * p.y, s * p.x + c * p.y);
    p.x /= uAspect;
    gl_Position = vec4(p + uCenter, 0.0, 1.0);
}
)GLSL";

inline const char* const kTextFrag = R"GLSL(
varying vec2 vUV;
uniform sampler2D uText;
uniform vec2 uTexel;
uniform float uAlpha, uReveal, uGlitch, uTime, uColorMode, uHue;
uniform vec3 uC0, uC1, uC2;
vec3 hueRot(vec3 c, float a){
    float s = sin(a); float co = cos(a); vec3 k = vec3(0.57735027);
    return c * co + cross(k, c) * s + k * dot(k, c) * (1.0 - co);
}
vec3 pal(float t){
    t = fract(t); float x = t * 3.0; vec3 c;
    if (x < 1.0) c = mix(uC0, uC1, smoothstep(0.0, 1.0, x));
    else if (x < 2.0) c = mix(uC1, uC2, smoothstep(0.0, 1.0, x - 1.0));
    else c = mix(uC2, uC0, smoothstep(0.0, 1.0, x - 2.0));
    return hueRot(c, uHue);
}
float mask(vec2 uv){ return texture2D(uText, vec2(uv.x, 1.0 - uv.y)).a; }
void main()
{
    vec2 uv = vUV;
    float split = 0.0;
    if (uGlitch > 0.001) {
        float band = floor(uv.y * 14.0);
        float h = fract(sin(band * 91.7 + floor(uTime * 18.0) * 13.1) * 43758.5);
        if (h > 0.7) { uv.x += (h - 0.85) * 0.12 * uGlitch; split = 0.01 * uGlitch; }
    }
    float rv = step(uv.x, uReveal);
    float a = mask(uv) * rv;
    float sh = mask(uv + vec2(-3.0, 3.0) * uTexel) * rv;
    float g = 0.0;
    for (int i = 0; i < 8; i++) {
        float an = float(i) * 0.785398;
        vec2 o = vec2(cos(an), sin(an));
        g += mask(uv + o * uTexel * 10.0) + mask(uv + o * uTexel * 22.0) * 0.6;
    }
    g = g / 12.8 * rv;

    vec3 tc = vec3(1.0);
    if (uColorMode > 0.5 && uColorMode < 1.5) tc = pal(uv.x * 0.8 + uTime * 0.1) * 1.3;
    else if (uColorMode > 1.5 && uColorMode < 2.5) tc = pal(0.1 + uTime * 0.05) * 1.3;
    else if (uColorMode > 2.5) tc = vec3(0.0);
    vec3 gc = (uColorMode > 2.5) ? vec3(1.0) : tc;

    vec3 rgb = tc * a + gc * g * 0.55 * (1.0 - a);
    float al = clamp(a + g * 0.5 * (1.0 - a) + sh * 0.45 * (1.0 - a), 0.0, 1.0);
    if (split > 0.0) {
        float r2 = mask(uv + vec2(split, 0.0)); float b2 = mask(uv - vec2(split, 0.0));
        rgb = vec3(max(rgb.r, r2 * 0.9), rgb.g, max(rgb.b, b2 * 0.9));
    }
    gl_FragColor = vec4(rgb * uAlpha, al * uAlpha);
}
)GLSL";

// ================================================================ MODE SHADERS (indexed by enum Base)
// Each defines:  vec3 render(vec2 uv, vec2 st)
//   uv : centred, aspect-correct (y in -0.5..0.5)     st : 0..1 screen coordinates
//   uP / uQ : the preset parameters from VisualModes.h

inline const char* const kBars = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float n = uP.x; float mir = uP.y; float gap = uP.z; float hs = uP.w;
    float led = uQ.x; float segs = max(uQ.y, 1.0); float refl = uQ.z;
    float x = st.x; if (mir > 0.5) x = abs(st.x * 2.0 - 1.0);
    float bi = floor(x * n); float bx = fract(x * n);
    float v = clamp(spec((bi + 0.5) / n) * hs, 0.0, 1.0);
    float baseY = (refl > 0.5) ? 0.32 : 0.04;
    float maxH = 0.95 - baseY;
    float yy = st.y; float fade = 1.0;
    if (yy < baseY) {
        if (refl > 0.5) { yy = 2.0 * baseY - yy; fade = 0.28 * (st.y / baseY); }
        else return vec3(0.0);
    }
    float h = (yy - baseY) / maxH;
    float inBar = step(gap * 0.5, bx) * step(bx, 1.0 - gap * 0.5);
    float on = step(h, v) * step(0.0, h);
    if (led > 0.5) on *= step(0.14, fract(h * segs));
    vec3 col = pal(x * 0.8 + iTime * 0.04) * (0.55 + 0.7 * h);
    float cap = smoothstep(0.03, 0.0, abs(h - v)) * step(0.02, v);
    vec3 c = col * on * inBar * fade;
    c += vec3(1.0) * cap * inBar * 0.5 * fade;
    return c;
}
)GLSL";

inline const char* const kRadial = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float n = uP.x; float inner = uP.y * 0.6 * (1.0 + 0.12 * iBass); float ls = uP.z; float mir = uP.w;
    float rs = uQ.x; float style = uQ.y;
    float r = length(uv); float a = atan(uv.y, uv.x) / TAU + 0.5;
    a = fract(a + iTime * rs * 0.04);
    float x = (mir > 0.5) ? abs(a * 2.0 - 1.0) : a;
    float bi = floor(x * n); float bx = fract(x * n);
    float v = (style > 1.5) ? spec(x) : spec((bi + 0.5) / n);
    float len = v * ls * 0.55; float d = r - inner;
    float col = 0.0;
    if (style < 0.5) col = step(0.12, bx) * step(bx, 0.88) * step(0.0, d) * step(d, len);
    else if (style < 1.5) col = step(0.0, d) * step(d, len);
    else col = smoothstep(0.006, 0.0, abs(d - len)) * step(0.002, len);
    vec3 c = pal(x + iTime * 0.05) * col * (0.6 + 0.8 * clamp(d / (len + 0.001), 0.0, 1.0));
    c += pal(0.5 + iTime * 0.03) * smoothstep(0.008, 0.0, abs(r - inner)) * (0.35 + 0.8 * iBass);
    return c;
}
)GLSL";

inline const char* const kMeter = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    vec3 col = vec3(0.0);
    for (int i = 0; i < 3; i++) {
        float v = (i == 0) ? iBass : ((i == 1) ? iMid : iTreb);
        v = clamp(v * 1.2, 0.0, 1.0);
        float fi = float(i);
        if (uP.x < 0.5) {
            float r = length(uv); float a = atan(uv.x, uv.y);
            float rad = 0.16 + 0.09 * fi; float sweep = 2.4;
            float on = step(abs(a), sweep); float f = (a + sweep) / (2.0 * sweep);
            float ring = smoothstep(0.012, 0.0, abs(r - rad));
            float lit = step(f, v);
            col += pal(fi * 0.33 + 0.1) * ring * on * (0.15 + 0.85 * lit);
        } else {
            float y0 = (fi - 1.0) * 0.14;
            float inb = step(abs(uv.y - y0), 0.045);
            float f = (uv.x + 0.4) / 0.8;
            float seg = floor(f * 24.0);
            float lit = step((seg + 0.5) / 24.0, v) * step(0.1, fract(f * 24.0));
            col += pal(fi * 0.33 + f * 0.3) * inb * step(0.0, f) * step(f, 1.0) * (0.12 + 0.9 * lit);
        }
    }
    return col;
}
)GLSL";

inline const char* const kScope = R"GLSL(
float lineG(float d, float w){ d = abs(d); return smoothstep(w, 0.0, d) + 0.12 * w / (d + w); }
vec3 render(vec2 uv, vec2 st)
{
    float th = uP.x; float lay = uP.y; float amp = uP.z; int L = int(uQ.x + 0.5);
    vec3 col = vec3(0.0);
    for (int k = 0; k < 4; k++) {
        if (k >= L) break;
        float fk = float(k) / max(float(L - 1), 1.0);
        float sh = float(k) * 0.02;
        float d = 1.0; float cx = st.x;
        if (lay < 0.5) { float w = waveL(st.x + sh); d = uv.y - w * amp * 0.4; }
        else if (lay < 1.5) { float w = waveL(st.x + sh) * amp * 0.4; d = min(abs(uv.y - w), abs(uv.y + w)); }
        else if (lay < 2.5) {
            float wl = waveL(st.x + sh) * amp * 0.22; float wr = waveR(st.x + sh) * amp * 0.22;
            d = min(abs(uv.y - 0.22 - wl), abs(uv.y + 0.22 - wr));
        } else {
            float a = atan(uv.y, uv.x); float x = abs(a) / PI; float w = waveL(x + sh) * amp * 0.2;
            d = length(uv) - (0.26 + w); cx = x;
        }
        col += pal(fk * 0.5 + cx * 0.3 + iTime * 0.05) * lineG(d, th) * (1.0 - fk * 0.55);
    }
    return col * (0.8 + 0.6 * iLevel);
}
)GLSL";

inline const char* const kTunnel = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float sides = uP.x; float tw = uP.y; float dens = uP.z; float sp = uP.w; float chk = uQ.x;
    float a = atan(uv.y, uv.x);
    float d = (sides > 0.5) ? polyD(uv, sides) : length(uv);
    d = max(d, 0.003);
    float depth = 0.15 / d;
    float v = depth * dens * 0.18 - iTime * sp * 0.7 - iBeat * 0.15;
    float ang = a / TAU + tw * depth * 0.06 + iTime * 0.02 * tw;
    float pat;
    if (chk > 0.5) pat = mod(floor(v * 2.0) + floor(ang * 12.0), 2.0);
    else {
        float f = fract(v);
        pat = smoothstep(0.0, 0.06, f) * (1.0 - smoothstep(0.14, 0.32, f))
            + 0.25 * smoothstep(0.46, 0.5, abs(fract(ang * 12.0) - 0.5));
    }
    float fade = smoothstep(0.0, 0.3, d);
    float s = spec(clamp(d * 1.8, 0.0, 1.0));
    vec3 c = pal(v * 0.04 + d + iTime * 0.03) * pat * fade * (0.45 + 1.6 * s + 0.6 * iBeat);
    c += pal(0.2) * 0.03 / (d + 0.03) * (0.2 + iBass);
    return c;
}
)GLSL";

inline const char* const kPoly = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float n = uP.x; float N = uP.y; float sp = uP.z; float th = uP.w; float alt = uQ.x;
    vec3 col = vec3(0.0);
    for (int i = 0; i < 16; i++) {
        if (float(i) >= N) break;
        float fi = float(i) / N;
        float dir = (alt > 0.5 && mod(float(i), 2.0) > 0.5) ? -1.0 : 1.0;
        vec2 q = rot2(iTime * sp * 0.35 * dir + fi * 2.0 + iBass * 0.3) * uv;
        float v = spec(fi * 0.85 + 0.02);
        float rad = (0.04 + fi * 0.5) * (1.0 + v * 0.25) + 0.01 * iBeat;
        float d = abs(polyD(q, n) - rad);
        float w = th * (1.0 + v * 2.0);
        col += pal(fi + iTime * 0.05) * (smoothstep(w, 0.0, d) + 0.05 * w / (d + w)) * (0.5 + 1.4 * v);
    }
    return col;
}
)GLSL";

inline const char* const kSpiral = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float arms = uP.x; float tw = uP.y; float sp = uP.z; float ct = uP.w; float chk = uQ.x;
    float r = length(uv); float a = atan(uv.y, uv.x);
    float lr = log(r + 0.03);
    float v = a * arms / TAU + lr * tw * 2.0 - iTime * sp * 0.5 - iBass * 0.4;
    float f = fract(v);
    float pat;
    if (chk > 0.5) pat = mod(floor(v * 2.0) + floor(lr * tw * 5.0), 2.0);
    else pat = smoothstep(0.0, 0.12, f) * (1.0 - smoothstep(0.4, 0.6, f));
    float s = spec(clamp(r * 1.7, 0.0, 1.0));
    float fade = smoothstep(0.0, 0.08, r) * (1.0 - smoothstep(0.55, 0.9, r) * 0.5);
    return pal(v * 0.1 + r * 0.6 + iTime * 0.03) * pat * fade * (0.4 + 1.4 * s * ct + 0.5 * iBeat);
}
)GLSL";

inline const char* const kHex = R"GLSL(
vec4 hexCoords(vec2 uv){
    vec2 r = vec2(1.0, 1.73205); vec2 h = r * 0.5;
    vec2 a = mod(uv, r) - h; vec2 b = mod(uv - h, r) - h;
    vec2 gv = (dot(a, a) < dot(b, b)) ? a : b;
    return vec4(gv, uv - gv);
}
float hexDist(vec2 p){ p = abs(p); float c = dot(p, normalize(vec2(1.0, 1.73205))); return max(c, p.x); }
vec3 render(vec2 uv, vec2 st)
{
    vec2 p = uv * uP.x; vec4 h = hexCoords(p);
    float d = 0.5 - hexDist(h.xy);
    vec2 id = floor(h.zw * 100.0 + 0.5) * 0.01;
    float rid = hash21(id);
    float dc = clamp(length(id) / uP.x, 0.0, 1.0);
    float v = spec(clamp(dc * 0.85 + rid * 0.12, 0.0, 1.0));
    float edge = smoothstep(uP.z, 0.0, d);
    float fill = smoothstep(0.0, 0.06, d) * v * uP.y * (0.55 + 0.45 * sin(iTime * 1.5 + rid * TAU));
    return pal(rid * 0.5 + dc * 0.5 + iTime * 0.03) * (fill + edge * (0.25 + v)) * (0.8 + 0.6 * iBeat);
}
)GLSL";

inline const char* const kRays = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float n = uP.x; float spin = uP.y; float sharp = uP.z; float swirl = uP.w;
    float r = length(uv); float a = atan(uv.y, uv.x);
    float x = a / TAU * n + iTime * spin * 0.3 + r * swirl * 2.0;
    float ray = pow(0.5 + 0.5 * cos(x * TAU), sharp);
    float a01 = abs(a) / PI;
    float v = spec(a01);
    vec3 c = pal(a01 + iTime * 0.05) * ray * (0.25 + 1.6 * v) * exp(-r * 1.8);
    c += pal(0.1) * 0.02 / (r + 0.02) * (0.3 + iBass + iBeat);
    return c;
}
)GLSL";

inline const char* const kPulse = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float th = uP.x; float spd = uP.y; float sides = uP.z; float amp = uP.w;
    float d = (sides > 0.5) ? polyD(uv, sides) : length(uv);
    vec3 c = vec3(0.0);
    for (int i = 0; i < 8; i++) {
        float age = uAges[i];
        float life = 1.0 - age / 2.2;
        if (life <= 0.0) continue;
        float rad = 0.03 + age * spd * 0.45;
        float w = th * (1.0 + age * 1.5);
        float dd = abs(d - rad);
        float ring = smoothstep(w, 0.0, dd) + 0.05 * w / (dd + w);
        c += pal(float(i) * 0.13 + iTime * 0.05) * ring * life * life * amp;
    }
    float core = 0.05 + iBass * 0.08;
    c += pal(iTime * 0.04) * smoothstep(core, 0.0, d) * (0.6 + iBeat);
    return c;
}
)GLSL";

inline const char* const kKali = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float iters = uP.x; float fp = uP.y; float zoom = uP.z; float gain = uP.w;
    vec2 p2 = rot2(iTime * 0.04) * uv * zoom * 2.0;
    vec3 p = vec3(p2, 0.3 + 0.15 * sin(iTime * 0.11) + iBass * 0.05);
    vec3 prm = vec3(fp * 0.62 + 0.04 * sin(iTime * 0.21) + iBass * 0.03,
                    fp * 0.58 + 0.03 * cos(iTime * 0.17) + iMid * 0.02,
                    fp * 0.60 + 0.02 * sin(iTime * 0.13));
    float a = 0.0; float pa = 0.0; float m = 10.0;
    for (int i = 0; i < 16; i++) {
        if (float(i) >= iters) break;
        p = abs(p) / max(dot(p, p), 0.0001) - prm;
        float lp = length(p);
        a += abs(lp - pa); pa = lp; m = min(m, lp);
    }
    float v = a * a * a * 0.0004 * gain * (0.8 + iLevel + iBeat * 0.5);
    v = 1.0 - exp(-v);
    return pal(m * 0.6 + iTime * 0.03) * v * (0.7 + 0.9 * spec(clamp(length(uv) * 1.4, 0.0, 1.0)));
}
)GLSL";

inline const char* const kJulia = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float maxIt = uP.x; float cr = uP.y; float zoom = uP.z; float type = uP.w;
    float ang = iTime * 0.17 + iBass * 0.6 + iMid * 0.3;
    vec2 c = cr * vec2(cos(ang), sin(ang)) * (0.97 + 0.03 * sin(iTime));
    if (type > 1.5) c *= 0.9;
    vec2 z = uv * 3.0 / zoom;
    float it = 0.0; float esc = 0.0;
    for (int i = 0; i < 64; i++) {
        if (float(i) >= maxIt) break;
        if (type > 0.5 && type < 1.5) z = abs(z);
        if (type > 1.5) z = vec2(z.x * z.x * z.x - 3.0 * z.x * z.y * z.y, 3.0 * z.x * z.x * z.y - z.y * z.y * z.y) + c;
        else z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        if (dot(z, z) > 16.0) { esc = 1.0; break; }
        it += 1.0;
    }
    if (esc < 0.5) return pal(0.55 + iTime * 0.03) * 0.08 * (0.3 + iBass * 2.0);
    float sm = it - log2(max(log2(dot(z, z)), 0.0001)) + 4.0;
    float t = sm / maxIt;
    return pal(t * 2.0 + iTime * 0.05) * (0.35 + 1.2 * sqrt(max(t, 0.0))) * (0.65 + 0.9 * spec(clamp(t, 0.0, 1.0)) + 0.4 * iBeat);
}
)GLSL";

inline const char* const kPlasma = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float fr = uP.x; float warp = uP.y; float spd = uP.z; float ct = uP.w; float vr = uQ.x;
    vec2 p = uv * fr; float t = iTime * spd;
    if (vr > 0.5 && vr < 1.5) p += warp * vec2(fbm(p * 0.7 + t * 0.3), fbm(p * 0.7 - t * 0.3 + 5.0)) * 2.0;
    float v = sin(p.x + t) + sin(p.y * 1.3 + t * 1.1) + sin((p.x + p.y) * 0.7 + t * 0.7)
            + sin(length(p + vec2(sin(t * 0.3), cos(t * 0.4)) * 2.0) * 1.5 - t);
    v *= 0.25;
    float k = v * 0.5 + 0.5;
    if (vr > 1.5) k = abs(sin(v * 3.0 + iBass));
    return pal(k * ct + iTime * 0.03 + iBass * 0.2) * (0.35 + 0.9 * k * k + 0.5 * spec(k)) * (0.8 + 0.5 * iBeat);
}
)GLSL";

inline const char* const kMeta = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float N = uP.x; float spd = uP.y; float sz = uP.z; float soft = uP.w;
    float f = 0.0; vec3 acc = vec3(0.0);
    for (int i = 0; i < 12; i++) {
        if (float(i) >= N) break;
        float fi = float(i);
        vec2 c = 0.36 * vec2(sin(iTime * spd * (0.3 + 0.07 * fi) + fi * 2.1) * 1.4,
                             cos(iTime * spd * (0.27 + 0.05 * fi) + fi * 1.3));
        float v = spec(fi / N * 0.8 + 0.03);
        float rad = sz * 0.075 * (0.55 + 1.5 * v);
        vec2 d = uv - c;
        float infl = rad * rad / (dot(d, d) + 0.0001);
        f += infl; acc += pal(fi / N + iTime * 0.03) * infl;
    }
    vec3 base = acc / max(f, 0.001);
    float m = smoothstep(0.85, 1.0 + soft * 0.4, f);
    vec3 c = base * m * (0.55 + 0.5 * smoothstep(1.0, 3.0, f));
    c += pal(0.7 + iTime * 0.02) * 0.5 * exp(-abs(f - 1.0) * 5.0);
    return c;
}
)GLSL";

inline const char* const kVoronoi = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float sc = uP.x; float mode = uP.y; float gn = uP.z;
    vec2 p = uv * sc; vec2 ip = floor(p); vec2 fp = fract(p);
    float d1 = 8.0; float d2 = 8.0; vec2 id = vec2(0.0);
    for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {
            vec2 g = vec2(float(i), float(j));
            vec2 o = hash22(ip + g);
            o = 0.5 + 0.45 * sin(iTime * 0.6 + TAU * o);
            vec2 r = g + o - fp;
            float d = dot(r, r);
            if (d < d1) { d2 = d1; d1 = d; id = ip + g; }
            else if (d < d2) { d2 = d; }
        }
    }
    d1 = sqrt(d1); d2 = sqrt(d2);
    float edge = d2 - d1;
    float rid = hash21(id);
    float cv = spec(clamp(rid * 0.7 + 0.02, 0.0, 1.0));
    if (mode < 0.5) return pal(rid + iTime * 0.02) * smoothstep(0.08, 0.0, edge) * (0.4 + 1.6 * cv);
    if (mode < 1.5) return pal(rid + iTime * 0.02) * (0.15 + cv * 1.4 * gn) * (0.6 + 0.4 * smoothstep(0.0, 0.2, edge));
    return pal(rid + d1) * (0.3 + cv) * (0.25 + 0.75 * smoothstep(0.0, 0.35, edge)) + vec3(1.0) * smoothstep(0.05, 0.0, edge) * 0.3;
}
)GLSL";

inline const char* const kNebula = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float sc = uP.x; float spd = uP.y; float vr = uP.z; float base = uP.w;
    vec2 p = uv * sc; if (vr > 0.5) p.x *= 0.6;
    float t = iTime * spd * 0.2;
    vec2 q = vec2(fbm(p + t), fbm(p + vec2(5.2, 1.3) - t));
    vec2 r = vec2(fbm(p + 3.0 * q + vec2(1.7, 9.2) + t * 0.7), fbm(p + 3.0 * q + vec2(8.3, 2.8) - t * 0.5));
    float f = fbm(p + 3.5 * r + iBass * 0.5);
    float inten = f * f * 2.2 + base;
    vec3 c = pal(f * 1.1 + r.x * 0.6 + iTime * 0.02) * inten * (0.55 + 0.9 * iLevel + 0.4 * iBeat);
    c += pal(0.3) * pow(f, 4.0) * iBass * 1.5;
    return c;
}
)GLSL";

inline const char* const kFire = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float sc = uP.x; float spd = uP.y; float hs = uP.z; float inten = uP.w; float vr = uQ.x;
    vec2 p = vec2(st.x * ASPECT * sc, st.y * sc * 0.8);
    float t = iTime * spd;
    float n = fbm(vec2(p.x, p.y - t * 2.2)) * 0.65 + fbm(vec2(p.x * 2.1 + 3.0, p.y * 1.7 - t * 3.1)) * 0.35;
    float h = 0.12 + clamp(spec(st.x) * hs, 0.0, 1.0) * 0.8 + iBeat * 0.05;
    float y = st.y / h;
    float heat = clamp(n * 1.45 + (1.0 - y) * 0.95 - 0.5, 0.0, 1.0);
    heat *= 1.0 - smoothstep(1.0, 1.6, y);
    heat = pow(heat, 0.9) * inten;
    vec3 fire = vec3(heat * 1.6, heat * heat * 1.3, heat * heat * heat * 0.8);
    vec3 pc = pal(heat * 0.8 + iTime * 0.03) * heat * 1.6;
    return (vr > 0.5) ? pc : fire;
}
)GLSL";

inline const char* const kRipple = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float N = uP.x; float fr = uP.y; float spd = uP.z; float am = uP.w;
    float v = 0.0;
    for (int i = 0; i < 6; i++) {
        if (float(i) >= N) break;
        float fi = float(i);
        vec2 c = 0.3 * vec2(cos(iTime * 0.2 * (fi + 1.0) + fi * 1.7) * 1.5, sin(iTime * 0.23 * (fi + 1.0) + fi * 2.3));
        float d = length(uv - c);
        float s = spec(fi / N * 0.7 + 0.03);
        v += sin(d * fr * 1.5 - iTime * spd * 3.0) * exp(-d * 1.6) * (0.35 + s * am * 1.6);
    }
    v /= max(N * 0.55, 1.0);
    float k = clamp(v * 0.5 + 0.5, 0.0, 1.0);
    return pal(k + iTime * 0.02) * (0.12 + 1.2 * k * k) * (0.8 + 0.5 * iBeat);
}
)GLSL";

inline const char* const kStars = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float L = uP.x; float spd = uP.y; float boost = uP.z; float dens = clamp(uP.w / 100.0, 0.05, 1.0);
    vec3 col = vec3(0.0);
    vec2 dir = normalize(uv + vec2(0.0001));
    float stretch = 1.0 + boost * (iBeat * 1.6 + iLevel * 1.4);
    for (int l = 0; l < 8; l++) {
        if (float(l) >= L * 2.0) break;
        float fl = float(l) / (L * 2.0);
        float z = fract(fl - iTime * 0.06 * spd);
        float scale = mix(26.0, 1.5, z);
        float fade = smoothstep(0.0, 0.12, z) * (1.0 - smoothstep(0.8, 1.0, z));
        vec2 p = uv * scale + vec2(float(l) * 37.7, float(l) * 11.3);
        vec2 id = floor(p); vec2 gv = fract(p) - 0.5;
        float h = hash21(id);
        if (h > dens) continue;
        vec2 off = (hash22(id + 7.7) - 0.5) * 0.6;
        vec2 rel = gv - off;
        float a = dot(rel, dir); float b = dot(rel, vec2(-dir.y, dir.x));
        float d = length(vec2(a / stretch, b));
        float sz = 0.06 + 0.12 * spec(h / dens * 0.9);
        float star = smoothstep(sz, 0.0, d) * fade;
        col += mix(vec3(1.0), pal(h * 3.0), 0.55) * star * (0.7 + iBeat * 0.8);
    }
    return col;
}
)GLSL";

inline const char* const kOrbs = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float N = uP.x; float rr = uP.y; float tw = uP.z; float sz = uP.w;
    vec3 col = vec3(0.0);
    for (int i = 0; i < 32; i++) {
        if (float(i) >= N) break;
        float fi = float(i) / N;
        float v = spec(fi * 0.9 + 0.02);
        float ang = fi * TAU * (1.0 + tw * 2.0) + iTime * (0.25 + 0.3 * fi);
        float rad = rr * 0.5 * ((tw > 0.01) ? (0.12 + 0.88 * fi) : (0.65 + 0.35 * v));
        vec2 c = rad * vec2(cos(ang), sin(ang));
        vec2 d = uv - c;
        float s = sz * (0.4 + 1.2 * v);
        col += pal(fi + iTime * 0.05) * s * s * 0.45 / (dot(d, d) + s * s * 0.12);
    }
    col *= 0.55;
    col += pal(0.2) * 0.012 / (length(uv) + 0.02) * (0.3 + iBass + iBeat);
    return col;
}
)GLSL";

inline const char* const kRaymarch = R"GLSL(
float smin2(float a, float b, float k){ float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0); return mix(b, a, h) - k * h * (1.0 - h); }
float mapS(vec3 p)
{
    float t = iTime * uP.w * 0.5;
    p.xz = rot2(t) * p.xz; p.xy = rot2(t * 0.7) * p.xy;
    int sh = int(uP.x + 0.5);
    float d;
    if (sh == 0) d = length(p) - 1.0;
    else if (sh == 1) { vec2 q = vec2(length(p.xz) - 1.0, p.y); d = length(q) - 0.38; }
    else if (sh == 2) { vec3 q = abs(p) - vec3(0.72); d = length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - 0.06; }
    else { float o = 0.55 * sin(iTime * 0.7); d = smin2(length(p - vec3(o, 0.0, 0.0)) - 0.65, length(p + vec3(o, 0.0, 0.0)) - 0.65, 0.5); }
    float w = sin(p.x * 4.0 + iTime * 1.5) * sin(p.y * 4.0 - iTime) * sin(p.z * 4.0 + iTime * 1.2);
    return d - uP.y * (0.05 + 0.2 * iBass + 0.1 * iMid + 0.05 * iTreb) * w;
}
vec3 render(vec2 uv, vec2 st)
{
    vec3 ro = vec3(0.0, 0.0, -uP.z); vec3 rd = normalize(vec3(uv, 1.3));
    float t = 0.0; float md = 10.0; bool hit = false;
    for (int i = 0; i < 56; i++) {
        float d = mapS(ro + rd * t);
        md = min(md, d);
        if (d < 0.002) { hit = true; break; }
        t += d * 0.75;
        if (t > 12.0) break;
    }
    vec3 col = pal(0.05) * 0.06 * (1.0 - clamp(length(uv) * 1.5, 0.0, 1.0));
    col += pal(0.6 + iTime * 0.03) * 0.012 / (md + 0.03) * (0.4 + iBass);
    if (hit) {
        vec3 p = ro + rd * t; vec2 e = vec2(0.003, 0.0);
        vec3 n = normalize(vec3(mapS(p + e.xyy) - mapS(p - e.xyy), mapS(p + e.yxy) - mapS(p - e.yxy), mapS(p + e.yyx) - mapS(p - e.yyx)));
        vec3 l = normalize(vec3(0.6, 0.7, -0.5));
        float dif = max(dot(n, l), 0.0); float fr = pow(1.0 - max(dot(n, -rd), 0.0), 3.0);
        vec3 base = pal(0.3 + 0.25 * n.y + 0.2 * n.x + iTime * 0.03);
        col = base * (0.18 + 0.9 * dif) + pal(0.75) * fr * (0.5 + iBass * 1.5)
            + vec3(1.0) * pow(max(dot(reflect(rd, n), l), 0.0), 24.0) * 0.4;
    }
    return col;
}
)GLSL";

inline const char* const kTerrain = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float gd = uP.x; float hzp = uP.y; float spd = uP.z; float mh = uP.w;
    float hz = 0.30 + hzp * 0.5;
    vec3 col = vec3(0.0);
    if (st.y < hz) {
        float dep = max(hz - st.y, 0.002);
        float z = 0.14 / dep; float x = uv.x / dep * 0.14;
        float w = 0.012 + 0.004 * z;
        float lz = gl1(z * 0.5 - iTime * spd * 0.35);
        float lx = gl1(x * 0.05 * gd);
        float ln = max(smoothstep(w, 0.0, lz), smoothstep(w, 0.0, lx));
        float fadeD = 1.0 - exp(-dep * 9.0);
        col = pal(0.15 + iTime * 0.02) * ln * fadeD * (0.7 + 1.2 * iBass + 0.6 * iBeat);
        col += pal(0.8) * 0.06 * (1.0 - fadeD);
    } else {
        vec3 sky = mix(pal(0.0) * 0.35, pal(0.6) * 0.9, smoothstep(0.0, 0.6, st.y - hz));
        sky *= 0.25 + 0.35 * (1.0 - (st.y - hz));
        col = sky;
        float sunY = hz + 0.17 - 0.5; float rad = 0.17 + 0.015 * iBass;
        float ds = length(uv - vec2(0.0, sunY)) - rad;
        float sun = smoothstep(0.004, 0.0, ds);
        float stripe = 1.0;
        if (uv.y < sunY) stripe = step(0.35 * (sunY - uv.y) / rad, fract(uv.y * 28.0));
        col = mix(col, pal(0.1) * 1.4 + vec3(0.2, 0.1, 0.0), sun * stripe);
        float xs = abs(st.x * 2.0 - 1.0);
        float mt = spec(xs * 0.85) * mh * 0.28 + 0.02 * vnoise(vec2(st.x * 40.0, 0.0));
        float top = hz + mt;
        if (st.y < top) col = pal(0.7) * 0.025;
        col += pal(0.7) * smoothstep(0.004, 0.0, abs(st.y - top)) * (0.8 + iBeat);
    }
    return col;
}
)GLSL";

inline const char* const kMatrix = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float cols = uP.x; float spd = uP.y; float fl = uP.z; float tl = uP.w;
    float rows = cols / ASPECT;
    vec2 cell = vec2(st.x * cols, st.y * rows);
    vec2 id = floor(cell); vec2 f = fract(cell);
    float h1 = hash11(id.x * 7.13 + 1.0);
    float sp = (0.35 + 0.65 * hash11(id.x * 3.7 + 2.0)) * spd * 0.35;
    float head = fract(h1 - iTime * sp * 0.1) * (rows + tl);
    float d = id.y - head;
    float inTrail = step(0.0, d) * step(d, tl);
    float tr = inTrail * exp(-d * 6.0 / tl);
    float isHead = step(abs(d), 0.5);
    vec2 gp = floor(f * vec2(3.0, 5.0));
    float g = step(0.42, hash21(gp + id * 1.37 + floor(iTime * fl) * 7.0));
    float mask = step(0.12, f.x) * step(f.x, 0.88) * step(0.1, f.y) * step(f.y, 0.9);
    float sv = spec(st.x);
    vec3 c = pal(0.3 + sv * 0.4) * tr * g * mask * (0.5 + 2.0 * sv);
    c += vec3(1.0) * isHead * g * mask * (0.6 + sv);
    return c;
}
)GLSL";

inline const char* const kRidge = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float N = uP.x; float ageStep = uP.y; float th = uP.z; float amp = uP.w;
    vec3 col = vec3(0.0);
    for (int j = 0; j < 64; j++) {
        float fi = N - 1.0 - float(j);
        if (fi < 0.0) break;
        float depth = fi / N;
        float by = 0.07 + 0.66 * depth;
        float xs = (st.x - 0.5) * (1.0 - 0.3 * depth) + 0.5;
        float env = pow(sin(clamp(xs, 0.0, 1.0) * PI), 0.7);
        float h = hist(xs, fi * ageStep * 4.0) * 0.34 * amp * env;
        float y = by + h;
        if (st.y < y) col = vec3(0.0);
        float e = smoothstep(th * (1.0 + depth), 0.0, abs(st.y - y));
        col = mix(col, pal(depth * 0.7 + iTime * 0.03) * (1.0 - depth * 0.55) * (0.7 + iBeat * (1.0 - depth)), e);
    }
    return col;
}
)GLSL";

inline const char* const kDots = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float n = uP.x; float ty = uP.y; float ds = uP.z;
    float x01; float y01; vec2 f;
    if (ty < 1.5) {
        vec2 g = vec2(st.x * n * ASPECT, st.y * n);
        if (ty > 0.5) g.x += 0.5 * mod(floor(g.y), 2.0);
        vec2 id = floor(g); f = fract(g) - 0.5;
        x01 = clamp((id.x + 0.5) / (n * ASPECT), 0.0, 1.0);
        y01 = clamp((id.y + 0.5) / n, 0.0, 1.0);
    } else {
        float r = length(uv) * 2.0; float a = atan(uv.y, uv.x) / TAU + 0.5;
        vec2 g = vec2(a * n * 2.0, r * n * 0.6);
        vec2 id = floor(g); f = fract(g) - 0.5;
        x01 = abs(((id.x + 0.5) / (n * 2.0)) * 2.0 - 1.0);
        y01 = clamp((id.y + 0.5) / (n * 0.6), 0.0, 1.0);
    }
    float v = spec(x01);
    float on = clamp((v * 1.1 - y01) * 7.0 + 0.5, 0.0, 1.0);
    float rad = (0.1 + 0.38 * on) * ds + 0.05 * iBeat;
    float dt = smoothstep(rad, rad - 0.1, length(f));
    return pal(x01 + y01 * 0.3 + iTime * 0.03) * dt * (0.25 + 0.9 * on);
}
)GLSL";

inline const char* const kBurst = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float N = uP.x; float life = uP.y; float spd = uP.z; float psz = uP.w;
    vec3 col = vec3(0.0);
    for (int b = 0; b < 8; b++) {
        float age = uAges[b];
        float lf = 1.0 - age / (life * 1.6);
        if (lf <= 0.0) continue;
        float seed = uSeeds[b];
        vec2 o = vec2((hash11(seed * 1.7 + 3.0) - 0.5) * ASPECT * 0.8, (hash11(seed * 3.1 + 9.0) - 0.5) * 0.55);
        vec2 q = uv - o + vec2(0.0, 0.22 * age * age);
        float a = atan(q.y, q.x) / TAU + 0.5;
        float idx = floor(a * N);
        for (int k = -1; k <= 1; k++) {
            float i = mod(idx + float(k), N);
            float hh = hash11(i * 1.31 + seed * 7.7);
            float ang = (i + hh * 0.8) / N * TAU - PI;
            float sp = (0.35 + 0.65 * hash11(i * 2.71 + seed * 1.3)) * 0.5 * spd;
            vec2 pp = sp * age * vec2(cos(ang), sin(ang));
            vec2 dd = q - pp;
            float s = psz * (0.6 + 0.8 * hh) * (0.5 + lf);
            col += pal(hash11(seed) + hh * 0.2 + iTime * 0.02) * s * s * 0.6 / (dot(dd, dd) + s * s * 0.08) * lf * lf;
        }
    }
    return col * 0.7;
}
)GLSL";

inline const char* const kSpectrogram = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float ori = uP.x; float mir = uP.y; float gain = uP.z; float shift = uP.w;
    float f; float age;
    if (ori < 0.5) { age = (1.0 - st.x) * 255.0; f = st.y; }
    else if (ori < 1.5) { age = (1.0 - st.y) * 255.0; f = st.x; }
    else { age = clamp(length(uv) * 2.0, 0.0, 1.0) * 255.0; f = abs(atan(uv.y, uv.x)) / PI; }
    if (mir > 0.5 && ori < 2.0) f = abs(f * 2.0 - 1.0);
    float v = clamp(hist(f, age) * gain, 0.0, 1.0);
    return pal(shift + v * 0.85 + iTime * 0.01) * pow(v, 0.85) * 1.5;
}
)GLSL";

inline const char* const kAddSynth = R"GLSL(
vec3 render(vec2 uv, vec2 st)
{
    float N = uP.x; float fq = uP.y; float spd = uP.z; float gn = uP.w; float radial = uQ.x; float fold = uQ.y;
    vec2 p = uv;
    if (fold > 0.5) { float a = atan(p.y, p.x); float k = TAU / fold; a = abs(mod(a, k) - k * 0.5); p = length(p) * vec2(cos(a), sin(a)); }
    float s = 0.0;
    for (int i = 0; i < 16; i++) {
        if (float(i) >= N) break;
        float fi = float(i);
        float amp = 0.15 + spec((fi + 0.5) / N * 0.8) * 1.5;
        float k = fq * (1.0 + 0.4 * fi) * 3.0;
        vec2 dir = vec2(cos(fi * 2.399), sin(fi * 2.399));
        float ph = iTime * spd * (0.3 + 0.1 * fi) + fi;
        if (radial > 0.5) s += amp * sin(length(p) * k + ph + dot(p, dir) * 2.0);
        else s += amp * sin(dot(p, dir) * k + ph);
    }
    s = s / sqrt(N) * gn;
    float v = clamp(s * 0.5 + 0.5, 0.0, 1.0);
    return pal(v * 1.5 + iTime * 0.02) * (0.25 + 0.85 * smoothstep(0.15, 0.85, v)) * (0.8 + 0.5 * iBeat);
}
)GLSL";

inline const char* const kImageWarp = R"GLSL(
float lumaOf(vec3 c){ return dot(c, vec3(0.299, 0.587, 0.114)); }
vec3 procImg(vec2 q){
    vec2 g = floor(q * vec2(10.0, 6.0)); float ch = mod(g.x + g.y, 2.0);
    return mix(pal(q.x * 0.6 + q.y * 0.4) * 0.9, pal(q.y * 0.7 + 0.3) * 0.45, ch);
}
vec2 mirrorRep(vec2 v){ return 1.0 - abs(1.0 - mod(v, 2.0)); }
vec3 img(vec2 s)
{
    float A = ASPECT; vec2 q = s;
    if (uHasImage > 0.5) { if (A > uImgAspect) q.y *= uImgAspect / A; else q.x *= A / uImgAspect; }
    q = mirrorRep(vec2(q.x + 0.5, 0.5 - q.y));
    if (uHasImage > 0.5) return texture2D(uImage, q).rgb;
    return procImg(q);
}
vec2 toS(vec2 sa){ return vec2(sa.x / ASPECT, sa.y); }
vec3 render(vec2 uv, vec2 st)
{
    float m = uP.x;
    vec2 sa = vec2((st.x - 0.5) * ASPECT, st.y - 0.5);
    vec3 col;
    if (m < 0.5) { sa /= 1.0 + 0.1 * iBass + 0.1 * iBeat; col = img(toS(sa)) * (1.0 + 0.35 * iBeat + 0.25 * iBass); }
    else if (m < 1.5) { float r = length(sa); sa += normalize(sa + 0.00001) * sin(r * 40.0 - iTime * 4.0) * 0.012 * (0.3 + iBass * 2.0 + iMid); col = img(toS(sa)); }
    else if (m < 2.5) { float o = 0.004 + 0.03 * iBeat + 0.02 * iTreb; vec2 s0 = toS(sa); col = vec3(img(s0 + vec2(o, 0.0)).r, img(s0).g, img(s0 - vec2(o, 0.0)).b); }
    else if (m < 3.5) { float r = length(sa); sa = rot2((0.5 * sin(iTime * 0.5) + iBass * 2.0) * smoothstep(0.7, 0.0, r)) * sa; col = img(toS(sa)); }
    else if (m < 4.5) { float px = mix(200.0, 22.0, clamp(iBass * 1.3 + iBeat * 0.5, 0.0, 1.0)); vec2 s0 = toS(sa); s0 = (floor(s0 * px) + 0.5) / px; col = img(s0); }
    else if (m < 5.5) {
        float band = floor(st.y * 26.0); float h = hash21(vec2(band, floor(iTime * 14.0)));
        float trig = step(0.84 - 0.4 * iBeat, h) * (0.15 + iBeat);
        vec2 s0 = toS(sa); s0.x += (h - 0.5) * 0.25 * trig;
        col = vec3(img(s0 + vec2(0.012 * trig, 0.0)).r, img(s0).g, img(s0 - vec2(0.012 * trig, 0.0)).b);
    }
    else if (m < 6.5) { vec2 s0 = toS(sa); s0.x += (spec(st.y) - 0.15) * 0.22; s0.y += (spec(st.x) - 0.15) * 0.06; col = img(s0); }
    else if (m < 7.5) {
        float r = length(sa); float a = atan(sa.y, sa.x); float k = PI / 6.0;
        a = abs(mod(a + iTime * 0.1, 2.0 * k) - k);
        sa = r * vec2(cos(a), sin(a)) * (1.0 - 0.2 * iBass); col = img(toS(sa));
    }
    else if (m < 8.5) { vec2 s0 = toS(sa); float l = lumaOf(img(s0)); float v = spec(clamp(l * 0.9, 0.0, 1.0)); col = pal(l + iTime * 0.05) * (0.3 + l * 0.7 + v * 1.2); }
    else {
        vec2 s0 = toS(sa); float e = 0.0016;
        float gx = lumaOf(img(s0 + vec2(e, 0.0))) - lumaOf(img(s0 - vec2(e, 0.0)));
        float gy = lumaOf(img(s0 + vec2(0.0, e))) - lumaOf(img(s0 - vec2(0.0, e)));
        float edge = length(vec2(gx, gy)) * 8.0;
        col = img(s0) * 0.35 + pal(edge * 0.5 + iTime * 0.05) * edge * (0.5 + iLevel * 2.0 + iBeat);
    }
    return col;
}
)GLSL";

// Order MUST match enum Base in VisualModes.h. B_XY is drawn as line geometry, so it has no fragment mode.
inline const char* const kModeSource[B_COUNT] = {
    kBars, kRadial, kMeter,                                   // B_BARS, B_RADIAL, B_METER
    kScope, nullptr,                                          // B_SCOPE, B_XY
    kTunnel, kPoly, kSpiral, kHex, kRays, kPulse,             // B_TUNNEL .. B_PULSE
    kKali, kJulia,                                            // B_KALI, B_JULIA
    kPlasma, kMeta, kVoronoi, kNebula, kFire, kRipple,        // B_PLASMA .. B_RIPPLE
    kStars, kOrbs, kRaymarch,                                 // B_STARS, B_ORBS, B_RAYMARCH
    kTerrain, kMatrix, kRidge,                                // B_TERRAIN, B_MATRIX, B_RIDGE
    kDots, kBurst,                                            // B_DOTS, B_BURST
    kSpectrogram, kAddSynth,                                  // B_SPECTROGRAM, B_ADDSYNTH
    kImageWarp                                                // B_IMAGEWARP
};
} // namespace Shaders
