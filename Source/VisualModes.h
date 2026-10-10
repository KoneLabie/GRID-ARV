#pragma once

// Every visual mode = one GLSL "base" algorithm + a preset of 8 parameters (p0..p3, q0..q3).
// The meaning of the parameters is documented next to each base in Shaders*.h.

enum Base
{
    B_BARS, B_RADIAL, B_METER,
    B_SCOPE, B_XY,
    B_TUNNEL, B_POLY, B_SPIRAL, B_HEX, B_RAYS, B_PULSE,
    B_KALI, B_JULIA,
    B_PLASMA, B_META, B_VORONOI, B_NEBULA, B_FIRE, B_RIPPLE,
    B_STARS, B_ORBS, B_RAYMARCH,
    B_TERRAIN, B_MATRIX, B_RIDGE,
    B_DOTS, B_BURST,
    B_SPECTROGRAM, B_ADDSYNTH,
    B_IMAGEWARP,
    B_COUNT
};

struct ModeDef
{
    const char* category;
    const char* name;
    int base;
    float p[4];
    float q[4];
};

inline const ModeDef kModes[] =
{
    // ------------------------------------------------------------ Spectrum
    { "Spectrum", "Classic Bars",      B_BARS,   { 48, 0, 0.25f, 0.9f },  { 0, 0, 1, 0 } },
    { "Spectrum", "Fine Bars",         B_BARS,   { 128, 0, 0.12f, 0.9f }, { 0, 0, 0, 0 } },
    { "Spectrum", "Mirror Bars",       B_BARS,   { 64, 1, 0.2f, 0.9f },   { 0, 0, 1, 0 } },
    { "Spectrum", "LED Stack",         B_BARS,   { 32, 0, 0.2f, 0.95f },  { 1, 24, 0, 0 } },
    { "Spectrum", "LED Mirror",        B_BARS,   { 40, 1, 0.2f, 0.95f },  { 1, 16, 0, 0 } },
    { "Spectrum", "Chunky Blocks",     B_BARS,   { 16, 0, 0.3f, 0.9f },   { 1, 10, 1, 0 } },
    { "Spectrum", "Wide Wall",         B_BARS,   { 256, 1, 0.0f, 0.8f },  { 0, 0, 0, 0 } },
    { "Spectrum", "Radial Burst",      B_RADIAL, { 48, 0.35f, 0.6f, 0 },  { 0.3f, 0, 0, 0 } },
    { "Spectrum", "Radial Dual",       B_RADIAL, { 40, 0.3f, 0.5f, 1 },   { 0.5f, 0, 1, 0 } },
    { "Spectrum", "Sunburst Fill",     B_RADIAL, { 64, 0.4f, 0.9f, 0 },   { 0.2f, 1, 0, 0 } },
    { "Spectrum", "Halo Line",         B_RADIAL, { 96, 0.5f, 0.5f, 1 },   { 0.1f, 2, 0, 0 } },
    { "Spectrum", "Spin Ring",         B_RADIAL, { 24, 0.45f, 0.35f, 1 }, { 1.5f, 0, 0, 0 } },
    { "Spectrum", "Triple Arc Meter",  B_METER,  { 0, 0, 0, 0 },          { 0, 0, 0, 0 } },
    { "Spectrum", "Level Bars",        B_METER,  { 1, 0, 0, 0 },          { 0, 0, 0, 0 } },

    // ------------------------------------------------------------ Waveform
    { "Waveform", "Scope Line",        B_SCOPE,  { 0.012f, 0, 0.6f, 0 },  { 1, 0, 0, 0 } },
    { "Waveform", "Scope Mirror",      B_SCOPE,  { 0.010f, 1, 0.5f, 0 },  { 2, 0, 0, 0 } },
    { "Waveform", "Scope Stereo",      B_SCOPE,  { 0.010f, 2, 0.6f, 0 },  { 1, 0, 0, 0 } },
    { "Waveform", "Scope Layers",      B_SCOPE,  { 0.008f, 0, 0.55f, 0 }, { 4, 0, 0, 0 } },
    { "Waveform", "Scope Circle",      B_SCOPE,  { 0.012f, 3, 1.0f, 0 },  { 3, 0, 0, 0 } },
    { "Waveform", "Phase Scope",       B_XY,     { 0.012f, 0, 1.0f, 0 },  { 0, 0, 0, 0 } },
    { "Waveform", "Phase Mid/Side",    B_XY,     { 0.012f, 1, 1.2f, 0 },  { 0, 0, 0, 0 } },
    { "Waveform", "Phase Thin",        B_XY,     { 0.006f, 1, 1.5f, 0 },  { 0, 0, 0, 0 } },

    // ------------------------------------------------------------ Geometric
    { "Geometric", "Warp Tunnel",      B_TUNNEL, { 0, 0.5f, 6, 1 },       { 0, 0, 0, 0 } },
    { "Geometric", "Square Tunnel",    B_TUNNEL, { 4, 0, 8, 1.2f },       { 0, 0, 0, 0 } },
    { "Geometric", "Hex Tunnel",       B_TUNNEL, { 6, 1, 6, 1 },          { 1, 0, 0, 0 } },
    { "Geometric", "Twist Tunnel",     B_TUNNEL, { 0, 3, 5, 0.8f },       { 0, 0, 0, 0 } },
    { "Geometric", "Checker Tunnel",   B_TUNNEL, { 0, 0, 8, 1.4f },       { 1, 0, 0, 0 } },
    { "Geometric", "Triangle Rings",   B_POLY,   { 3, 10, 0.6f, 0.008f }, { 0, 0, 0, 0 } },
    { "Geometric", "Square Rings",     B_POLY,   { 4, 12, 0.5f, 0.008f }, { 1, 0, 0, 0 } },
    { "Geometric", "Hexagon Rings",    B_POLY,   { 6, 14, 0.4f, 0.007f }, { 0, 0, 0, 0 } },
    { "Geometric", "Pentagon Rings",   B_POLY,   { 5, 9, 0.8f, 0.01f },   { 1, 0, 0, 0 } },
    { "Geometric", "Hypno Spiral",     B_SPIRAL, { 5, 1.2f, 1.5f, 1 },    { 0, 0, 0, 0 } },
    { "Geometric", "Vortex",           B_SPIRAL, { 9, 2.4f, 2.2f, 0.6f }, { 0, 0, 0, 0 } },
    { "Geometric", "Spiral Checker",   B_SPIRAL, { 8, 1.6f, 1.2f, 1 },    { 1, 0, 0, 0 } },
    { "Geometric", "Hex Grid",         B_HEX,    { 7, 1.0f, 0.04f, 0 },   { 0, 0, 0, 0 } },
    { "Geometric", "Honeycomb",        B_HEX,    { 12, 1.6f, 0.08f, 0 },  { 0, 0, 0, 0 } },
    { "Geometric", "Light Rays",       B_RAYS,   { 12, 0.8f, 4, 1 },      { 0, 0, 0, 0 } },
    { "Geometric", "Spiral Rays",      B_RAYS,   { 7, 3, 3, 1 },          { 0, 0, 0, 0 } },
    { "Geometric", "Laser Fan",        B_RAYS,   { 24, 0.2f, 12, 0.5f },  { 0, 0, 0, 0 } },
    { "Geometric", "Beat Rings",       B_PULSE,  { 0.02f, 0.9f, 0, 1 },   { 0, 0, 0, 0 } },
    { "Geometric", "Beat Squares",     B_PULSE,  { 0.02f, 0.8f, 4, 1 },   { 0, 0, 0, 0 } },
    { "Geometric", "Beat Hexagons",    B_PULSE,  { 0.025f, 0.7f, 6, 1 },  { 0, 0, 0, 0 } },

    // ------------------------------------------------------------ Fractal
    { "Fractal", "Kali Bloom",         B_KALI,   { 10, 0.8f, 1.2f, 0.4f },  { 0, 0, 0, 0 } },
    { "Fractal", "Kali Lace",          B_KALI,   { 14, 0.55f, 1.6f, 0.3f }, { 0, 0, 0, 0 } },
    { "Fractal", "Kali Deep",          B_KALI,   { 8, 1.0f, 0.9f, 0.6f },   { 0, 0, 0, 0 } },
    { "Fractal", "Julia Pulse",        B_JULIA,  { 48, 0.7885f, 1.0f, 0 },  { 0, 0, 0, 0 } },
    { "Fractal", "Ship Julia",         B_JULIA,  { 48, 0.45f, 1.0f, 1 },    { 0, 0, 0, 0 } },
    { "Fractal", "Cubic Julia",        B_JULIA,  { 40, 0.6f, 1.0f, 2 },     { 0, 0, 0, 0 } },

    // ------------------------------------------------------------ Organic
    { "Organic", "Plasma Classic",     B_PLASMA, { 2.5f, 0.3f, 0.5f, 0.6f }, { 0, 0, 0, 0 } },
    { "Organic", "Plasma Liquid",      B_PLASMA, { 1.6f, 1.2f, 1.0f, 0.5f }, { 1, 0, 0, 0 } },
    { "Organic", "Plasma Tight",       B_PLASMA, { 5, 0.6f, 0.8f, 0.9f },    { 2, 0, 0, 0 } },
    { "Organic", "Metaballs",          B_META,   { 6, 1.0f, 1.0f, 0.6f },    { 0, 0, 0, 0 } },
    { "Organic", "Lava Lamp",          B_META,   { 9, 0.8f, 1.3f, 0.3f },    { 0, 0, 0, 0 } },
    { "Organic", "Blob Swarm",         B_META,   { 12, 0.6f, 0.9f, 1.0f },   { 0, 0, 0, 0 } },
    { "Organic", "Cell Edges",         B_VORONOI,{ 5, 0, 0.8f, 0 },          { 0, 0, 0, 0 } },
    { "Organic", "Cell Fill",          B_VORONOI,{ 6, 1, 1.0f, 0 },          { 0, 0, 0, 0 } },
    { "Organic", "Crystals",           B_VORONOI,{ 4, 2, 0.6f, 0 },          { 0, 0, 0, 0 } },
    { "Organic", "Nebula",             B_NEBULA, { 1.6f, 1.0f, 0, 0.15f },   { 0, 0, 0, 0 } },
    { "Organic", "Smoke",              B_NEBULA, { 2.4f, 0.8f, 1, 0.25f },   { 0, 0, 0, 0 } },
    { "Organic", "Aurora Flow",        B_NEBULA, { 1.0f, 1.6f, 1, 0.12f },   { 0, 0, 0, 0 } },
    { "Organic", "Fire Wall",          B_FIRE,   { 3.2f, 0.7f, 1.6f, 1.2f }, { 0, 0, 0, 0 } },
    { "Organic", "Plasma Flames",      B_FIRE,   { 2.4f, 0.8f, 1.2f, 1.6f }, { 1, 0, 0, 0 } },
    { "Organic", "Ripples",            B_RIPPLE, { 3, 18, 1.0f, 1.5f },      { 0, 0, 0, 0 } },
    { "Organic", "Interference",       B_RIPPLE, { 5, 28, 0.8f, 2.2f },      { 0, 0, 0, 0 } },
    { "Organic", "Water Drops",        B_RIPPLE, { 2, 12, 1.4f, 1.0f },      { 0, 0, 0, 0 } },

    // ------------------------------------------------------------ Space
    { "Space", "Starfield",            B_STARS,  { 3, 0.5f, 1.0f, 60 },   { 0, 0, 0, 0 } },
    { "Space", "Hyperspace",           B_STARS,  { 4, 2.5f, 2.5f, 80 },   { 0, 0, 0, 0 } },
    { "Space", "Comet Rain",           B_STARS,  { 2, 4.0f, 1.4f, 40 },   { 0, 0, 0, 0 } },
    { "Space", "Orbit Orbs",           B_ORBS,   { 24, 0.7f, 0.0f, 0.07f }, { 0, 0, 0, 0 } },
    { "Space", "Spiral Galaxy",        B_ORBS,   { 32, 0.8f, 1.4f, 0.05f }, { 0, 0, 0, 0 } },
    { "Space", "Audio Sphere",         B_RAYMARCH,{ 0, 0.9f, 3.0f, 1.0f }, { 0, 0, 0, 0 } },
    { "Space", "Audio Torus",          B_RAYMARCH,{ 1, 0.7f, 4.0f, 1.2f }, { 0, 0, 0, 0 } },
    { "Space", "Audio Cube",           B_RAYMARCH,{ 2, 0.6f, 3.5f, 1.0f }, { 0, 0, 0, 0 } },
    { "Space", "Orb Fusion",           B_RAYMARCH,{ 3, 1.0f, 2.5f, 1.0f }, { 0, 0, 0, 0 } },

    // ------------------------------------------------------------ Retro
    { "Retro", "Synthwave",            B_TERRAIN,{ 10, 0.28f, 0.6f, 0.9f }, { 0, 0, 0, 0 } },
    { "Retro", "Neon Horizon",         B_TERRAIN,{ 16, 0.18f, 1.2f, 1.3f }, { 0, 0, 0, 0 } },
    { "Retro", "Digital Rain",         B_MATRIX, { 48, 1.0f, 5, 36 },       { 0, 0, 0, 0 } },
    { "Retro", "Data Stream",          B_MATRIX, { 96, 2.0f, 3, 72 },       { 0, 0, 0, 0 } },
    { "Retro", "Ridgelines",           B_RIDGE,  { 40, 0.5f, 0.006f, 1.0f }, { 0, 0, 0, 0 } },
    { "Retro", "Pulsar Waves",         B_RIDGE,  { 56, 0.7f, 0.005f, 1.5f }, { 0, 0, 0, 0 } },

    // ------------------------------------------------------------ Particles
    { "Particles", "Dot Matrix",       B_DOTS,   { 26, 0, 0.8f, 1 },      { 0, 0, 0, 0 } },
    { "Particles", "Halftone Hex",     B_DOTS,   { 18, 1, 0.9f, 1 },      { 0, 0, 0, 0 } },
    { "Particles", "Polar Dots",       B_DOTS,   { 16, 2, 0.8f, 1 },      { 0, 0, 0, 0 } },
    { "Particles", "Beat Fireworks",   B_BURST,  { 28, 1.4f, 1.0f, 0.012f }, { 0, 0, 0, 0 } },
    { "Particles", "Confetti Burst",   B_BURST,  { 20, 0.9f, 2.5f, 0.018f }, { 0, 0, 0, 0 } },

    // ------------------------------------------------------------ Image Synthesis (Photism-style)
    { "Image Synthesis", "Spectrogram",          B_SPECTROGRAM, { 0, 0, 1.2f, 0.0f }, { 0, 0, 0, 0 } },
    { "Image Synthesis", "Spectrogram Mirror",   B_SPECTROGRAM, { 0, 1, 1.2f, 0.3f }, { 0, 0, 0, 0 } },
    { "Image Synthesis", "Spectrogram Vertical", B_SPECTROGRAM, { 1, 0, 1.2f, 0.5f }, { 0, 0, 0, 0 } },
    { "Image Synthesis", "Spectrogram Radar",    B_SPECTROGRAM, { 2, 0, 1.2f, 0.1f }, { 0, 0, 0, 0 } },
    { "Image Synthesis", "Additive Waves",       B_ADDSYNTH, { 12, 3, 1.0f, 1.0f },   { 0, 0, 0, 0 } },
    { "Image Synthesis", "Moire Synth",          B_ADDSYNTH, { 16, 9, 0.5f, 0.8f },   { 0, 2, 0, 0 } },
    { "Image Synthesis", "Radial Synth",         B_ADDSYNTH, { 10, 6, 0.8f, 1.2f },   { 1, 0, 0, 0 } },
    { "Image Synthesis", "Crystal Synth",        B_ADDSYNTH, { 14, 5, 1.5f, 0.6f },   { 1, 3, 0, 0 } },

    // ------------------------------------------------------------ Image FX (use dropped images)
    { "Image FX", "Image Pulse",         B_IMAGEWARP, { 0, 1, 0, 0 }, { 0, 0, 0, 0 } },
    { "Image FX", "Image Ripple",        B_IMAGEWARP, { 1, 1, 0, 0 }, { 0, 0, 0, 0 } },
    { "Image FX", "Image RGB Split",     B_IMAGEWARP, { 2, 1, 0, 0 }, { 0, 0, 0, 0 } },
    { "Image FX", "Image Swirl",         B_IMAGEWARP, { 3, 1, 0, 0 }, { 0, 0, 0, 0 } },
    { "Image FX", "Image Mosaic",        B_IMAGEWARP, { 4, 1, 0, 0 }, { 0, 0, 0, 0 } },
    { "Image FX", "Image Glitch",        B_IMAGEWARP, { 5, 1, 0, 0 }, { 0, 0, 0, 0 } },
    { "Image FX", "Image Spectrum Wave", B_IMAGEWARP, { 6, 1, 0, 0 }, { 0, 0, 0, 0 } },
    { "Image FX", "Image Mirror Tiles",  B_IMAGEWARP, { 7, 1, 0, 0 }, { 0, 0, 0, 0 } },
    { "Image FX", "Image Luma Spectrum", B_IMAGEWARP, { 8, 1, 0, 0 }, { 0, 0, 0, 0 } },
    { "Image FX", "Image Edge Glow",     B_IMAGEWARP, { 9, 1, 0, 0 }, { 0, 0, 0, 0 } },
};

inline constexpr int kNumModes = (int) (sizeof(kModes) / sizeof(kModes[0]));

inline int firstModeWithBase(int base)
{
    for (int i = 0; i < kNumModes; ++i)
        if (kModes[i].base == base)
            return i;
    return 0;
}

inline bool isImageBase(int base) { return base == B_IMAGEWARP; }
