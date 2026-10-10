#pragma once

// Each palette = 3 colours that the shaders blend cyclically (see pal() in Shaders.h).
struct PaletteDef { const char* name; float c[3][3]; };

inline const PaletteDef kPalettes[] =
{
    { "Neon Sunset", { { 1.0f, 0.10f, 0.55f }, { 1.0f, 0.60f, 0.10f }, { 0.40f, 0.10f, 1.0f } } },
    { "Cyber Ocean", { { 0.0f, 0.90f, 1.00f }, { 0.1f, 0.30f, 1.00f }, { 0.60f, 0.10f, 0.9f } } },
    { "Toxic",       { { 0.6f, 1.00f, 0.00f }, { 0.0f, 0.90f, 0.50f }, { 0.00f, 0.40f, 0.2f } } },
    { "Inferno",     { { 1.0f, 0.90f, 0.30f }, { 1.0f, 0.30f, 0.00f }, { 0.50f, 0.00f, 0.15f } } },
    { "Ice",         { { 0.9f, 1.00f, 1.00f }, { 0.4f, 0.80f, 1.00f }, { 0.20f, 0.30f, 0.9f } } },
    { "Candy",       { { 1.0f, 0.50f, 0.80f }, { 0.5f, 0.80f, 1.00f }, { 1.00f, 0.90f, 0.5f } } },
    { "Aurora",      { { 0.1f, 1.00f, 0.60f }, { 0.2f, 0.50f, 1.00f }, { 0.80f, 0.20f, 1.0f } } },
    { "Gold",        { { 1.0f, 0.85f, 0.40f }, { 0.8f, 0.50f, 0.10f }, { 0.40f, 0.20f, 0.05f } } },
    { "Mono White",  { { 1.0f, 1.00f, 1.00f }, { 0.7f, 0.70f, 0.70f }, { 0.35f, 0.35f, 0.35f } } },
    { "Blood",       { { 1.0f, 0.10f, 0.10f }, { 0.6f, 0.00f, 0.10f }, { 1.00f, 0.40f, 0.3f } } },
    { "Vaporwave",   { { 1.0f, 0.40f, 0.90f }, { 0.3f, 0.90f, 1.00f }, { 0.60f, 0.40f, 1.0f } } },
    { "Rainbow",     { { 1.0f, 0.20f, 0.20f }, { 0.2f, 1.00f, 0.30f }, { 0.20f, 0.30f, 1.0f } } },
};

inline constexpr int kNumPalettes = (int) (sizeof(kPalettes) / sizeof(kPalettes[0]));
