#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "VisualModes.h"
#include "Palettes.h"

namespace ParamID
{
    inline constexpr const char* mode = "mode";
    inline constexpr const char* palette = "palette";
    inline constexpr const char* sens = "sens";
    inline constexpr const char* smooth = "smooth";
    inline constexpr const char* speed = "speed";
    inline constexpr const char* bright = "bright";
    inline constexpr const char* trails = "trails";
    inline constexpr const char* zoom = "zoom";
    inline constexpr const char* rot = "rot";
    inline constexpr const char* glow = "glow";
    inline constexpr const char* aberr = "aberr";
    inline constexpr const char* hue = "hue";
    inline constexpr const char* hueSpin = "hueSpin";
    inline constexpr const char* quality = "quality";
    inline constexpr const char* source = "source";
    inline constexpr const char* autoBeats = "autoBeats";
    inline constexpr const char* imgSlot = "imgSlot";
    inline constexpr const char* imgBeats = "imgBeats";
    inline constexpr const char* imgMix = "imgMix";
    inline constexpr const char* textOn = "textOn";
    inline constexpr const char* textLine = "textLine";
    inline constexpr const char* textBeats = "textBeats";
    inline constexpr const char* textSize = "textSize";
    inline constexpr const char* textReact = "textReact";
    inline constexpr const char* textAnim = "textAnim";
    inline constexpr const char* textColor = "textColor";
    inline constexpr const char* textPos = "textPos";
    inline constexpr const char* lyricOff = "lyricOff";
    inline constexpr const char* followHost = "followHost";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto F = [&](const char* id, const char* name, float lo, float hi, float def, float skew = 1.0f)
    {
        layout.add(std::make_unique<AudioParameterFloat>(ParameterID { id, 1 }, name,
                                                         NormalisableRange<float>(lo, hi, 0.001f, skew), def));
    };
    auto I = [&](const char* id, const char* name, int lo, int hi, int def)
    {
        layout.add(std::make_unique<AudioParameterInt>(ParameterID { id, 1 }, name, lo, hi, def));
    };
    auto C = [&](const char* id, const char* name, StringArray choices, int def)
    {
        layout.add(std::make_unique<AudioParameterChoice>(ParameterID { id, 1 }, name, choices, def));
    };
    auto B = [&](const char* id, const char* name, bool def)
    {
        layout.add(std::make_unique<AudioParameterBool>(ParameterID { id, 1 }, name, def));
    };

    // Mode: shows the mode NAME in the host's automation lane
    layout.add(std::make_unique<AudioParameterInt>(
        ParameterID { ParamID::mode, 1 }, "Visual Mode", 0, kNumModes - 1, 0,
        AudioParameterIntAttributes().withStringFromValueFunction(
            [](int v, int) { return String(kModes[jlimit(0, kNumModes - 1, v)].name); })));

    StringArray pals;
    for (int i = 0; i < kNumPalettes; ++i) pals.add(kPalettes[i].name);
    C(ParamID::palette, "Palette", pals, 0);

    F(ParamID::sens,    "Sensitivity",   0.1f, 4.0f, 1.0f, 0.6f);
    F(ParamID::smooth,  "Smoothing",     0.0f, 1.0f, 0.5f);
    F(ParamID::speed,   "Speed",         0.0f, 3.0f, 1.0f);
    F(ParamID::bright,  "Brightness",    0.2f, 2.0f, 1.0f);
    F(ParamID::trails,  "Trails",        0.0f, 0.97f, 0.35f);
    F(ParamID::zoom,    "Trail Zoom",   -0.03f, 0.06f, 0.0f);
    F(ParamID::rot,     "Trail Rotate", -0.03f, 0.03f, 0.0f);
    F(ParamID::glow,    "Glow",          0.0f, 2.0f, 0.5f);
    F(ParamID::aberr,   "Beat Aberration", 0.0f, 1.0f, 0.2f);
    F(ParamID::hue,     "Hue Shift",     0.0f, 1.0f, 0.0f);
    F(ParamID::hueSpin, "Hue Cycle",    -1.0f, 1.0f, 0.0f);

    C(ParamID::quality, "Quality", { "Performance (50%)", "Balanced (75%)", "Full (100%)" }, 1);
    C(ParamID::source,  "Input Source", { "Main Input", "Sidechain" }, 0);
    I(ParamID::autoBeats, "Auto-Cycle Modes (beats)", 0, 64, 0);

    I(ParamID::imgSlot,  "Image", 0, 64, 0);
    I(ParamID::imgBeats, "Image Cycle (beats)", 0, 64, 0);
    F(ParamID::imgMix,   "Image Background", 0.0f, 1.0f, 0.0f);

    B(ParamID::textOn, "Text On", true);
    I(ParamID::textLine,  "Text Line (0 = auto)", 0, 999, 0);
    I(ParamID::textBeats, "Text Advance (beats)", 0, 64, 0);
    F(ParamID::textSize,  "Text Size", 0.03f, 0.4f, 0.12f);
    F(ParamID::textReact, "Text Beat Reaction", 0.0f, 2.0f, 0.5f);
    C(ParamID::textAnim,  "Text Animation",
      { "Static", "Fade", "Pop", "Slide", "Typewriter", "Beat Shake", "Glitch", "Wobble" }, 2);
    C(ParamID::textColor, "Text Colour", { "White", "Palette Gradient", "Palette Solid", "Black" }, 0);
    C(ParamID::textPos,   "Text Position", { "Top", "Center", "Bottom" }, 1);
    F(ParamID::lyricOff,  "Lyric Offset (s)", -10.0f, 10.0f, 0.0f);
    B(ParamID::followHost, "Lyrics Follow Host", true);

    return layout;
}
