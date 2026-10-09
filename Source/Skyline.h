#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

// The pixel skyline in the screen, with screen_overlay's glass on top (docs/ui-spec.md "Skyline", draw
// order layers 2 and 3). It shows whatever City Size and bypass state the editor hands it.
class Skyline final : public juce::Component
{
public:
    Skyline (float initialCitySizePercent, bool initiallyBypassed);

    // Called from the editor's repaint timer; repaints only when the picture would change.
    void setState (float citySizePercent, bool bypassed);

    void paint (juce::Graphics& g) override;

private:
    struct Look
    {
        int litWindowCount = 0;
        float greyLayerAlpha = 0.0f;
        bool bypassed = false;

        bool operator== (const Look& other) const
        {
            return litWindowCount == other.litWindowCount
                && juce::exactlyEqual (greyLayerAlpha, other.greyLayerAlpha)
                && bypassed == other.bypassed;
        }
    };

    Look lookFor (float citySizePercent, bool bypassed) const;
    void rebuildLitLayer();
    void drawNormal (juce::Graphics& g) const;
    void drawBypassed (juce::Graphics& g) const;
    juce::Rectangle<float> windowBounds (juce::Point<int> window) const;

    const juce::Image colour;
    const juce::Image grey;
    const juce::Image overlay;
    const std::vector<juce::Point<int>> windows;
    const int pixelsPerPoint;
    const juce::Image glowSprite;

    // The normal look's lit windows with their glow, rebuilt only when the lit count changes.
    juce::Image litLayer;
    Look look;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Skyline)
};
