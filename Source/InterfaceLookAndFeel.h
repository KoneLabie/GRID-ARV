#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Flat, inexpensive vector controls: no blur, image effects, or animation timers.
class InterfaceLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    InterfaceLookAndFeel()
    {
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff24262b));
        setColour(juce::PopupMenu::textColourId, juce::Colour(0xfff2f3f5));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff3478f6));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xff555963));
        setColour(juce::ScrollBar::backgroundColourId, juce::Colours::transparentBlack);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int height) override
    {
        return juce::Font(juce::FontOptions(juce::jmin(13.0f, height * 0.48f)));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& colour, bool over, bool down) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto fill = down ? colour.brighter(0.15f) : over ? colour.brighter(0.08f) : colour;
        g.setColour(fill.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.45f));
        g.fillRoundedRectangle(bounds, 7.0f);
        g.setColour(button.hasKeyboardFocus(true) ? juce::Colour(0xff78a7ff)
                                                  : juce::Colour(0xff41444c));
        g.drawRoundedRectangle(bounds, 7.0f, button.hasKeyboardFocus(true) ? 1.5f : 0.6f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool over, bool down) override
    {
        if (!button.getProperties().contains("sectionHeader"))
        {
            juce::LookAndFeel_V4::drawButtonText(g, button, over, down);
            return;
        }
        g.setColour(button.findColour(juce::TextButton::textColourOffId));
        g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
        g.drawText(button.getButtonText(), button.getLocalBounds().reduced(12, 0).withTrimmedRight(18),
                   juce::Justification::centredLeft, true);
        const float x = button.getWidth() - 17.0f, y = button.getHeight() * 0.5f;
        juce::Path chevron;
        if (static_cast<bool>(button.getProperties()["sectionExpanded"]))
        { chevron.startNewSubPath(x - 3, y - 2); chevron.lineTo(x, y + 1); chevron.lineTo(x + 3, y - 2); }
        else
        { chevron.startNewSubPath(x - 2, y - 3); chevron.lineTo(x + 1, y); chevron.lineTo(x - 2, y + 3); }
        g.strokePath(chevron, juce::PathStrokeType(1.4f));
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int, int, int, int, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds, 7.0f);
        g.setColour(box.hasKeyboardFocus(true) ? juce::Colour(0xff78a7ff)
                                               : box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds, 7.0f, 1.0f);
        const float x = width - 14.0f, y = height * 0.5f;
        juce::Path arrow;
        arrow.startNewSubPath(x - 3, y - 1); arrow.lineTo(x, y + 2); arrow.lineTo(x + 3, y - 1);
        g.setColour(box.findColour(juce::ComboBox::arrowColourId).withMultipliedAlpha(box.isEnabled() ? 1.0f : 0.4f));
        g.strokePath(arrow, juce::PathStrokeType(1.4f));
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool over, bool) override
    {
        const float y = (button.getHeight() - 18.0f) * 0.5f;
        const auto track = juce::Rectangle<float>(2.0f, y, 32.0f, 18.0f);
        const float alpha = button.isEnabled() ? 1.0f : 0.45f;
        auto colour = button.getToggleState() ? juce::Colour(0xff3478f6) : juce::Colour(0xff555963);
        g.setColour((over ? colour.brighter(0.08f) : colour).withMultipliedAlpha(alpha));
        g.fillRoundedRectangle(track, 9.0f);
        g.setColour(juce::Colours::white.withMultipliedAlpha(alpha));
        g.fillEllipse(button.getToggleState() ? 18.0f : 4.0f, y + 2, 14.0f, 14.0f);
        g.setColour(button.findColour(juce::ToggleButton::textColourId).withMultipliedAlpha(alpha));
        g.setFont(juce::Font(juce::FontOptions(12.5f)));
        g.drawFittedText(button.getButtonText(), button.getLocalBounds().withTrimmedLeft(44),
                        juce::Justification::centredLeft, 2);
        if (button.hasKeyboardFocus(true))
        {
            g.setColour(juce::Colour(0xff78a7ff));
            g.drawRoundedRectangle(track.expanded(1.5f), 10.0f, 1.0f);
        }
    }
};
