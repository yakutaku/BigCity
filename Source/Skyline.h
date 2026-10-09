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

    // The normal look's lit windows with their glow, in one colour, rebuilt only when the lit count changes.
    struct LitLayer
    {
        juce::Colour windowColour;
        juce::Image glowSprite;
        juce::Image image;
    };

    Look lookFor (float citySizePercent, bool bypassed) const;
    LitLayer createLitLayer (juce::Colour windowColour) const;
    void rebuildLitLayers();
    void rebuild (LitLayer& layer) const;
    void drawNormal (juce::Graphics& g) const;
    void drawBypassed (juce::Graphics& g) const;
    juce::Rectangle<float> windowBounds (juce::Point<int> window) const;

    const juce::Image colour;
    const juce::Image grey;
    const juce::Image overlay;
    const std::vector<juce::Point<int>> windows;
    const int pixelsPerPoint;

    // In Figma the saturation layer sits over the windows and glow too, so they're desaturated with the
    // skyline: the grey copy goes over the yellow at the grey layer's alpha, as skyline_grey does.
    LitLayer litLayer;
    LitLayer greyLitLayer;
    Look look;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Skyline)
};
