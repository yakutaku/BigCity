#pragma once

#include "SkylineBypassFade.h"
#include "SkylineKeyframes.h"
#include "SkylineMorph.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

// The pixel skyline in the screen, with screen_overlay's glass on top (docs/ui-spec.md "Skyline", draw
// order layers 2 and 3). It shows whatever City Size and bypass state the editor hands it: City Size is the
// time of day (D-27), a crossfade between the keyframes either side of it.
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
        SkylineMorph::KeyframeBlend blend;
        float bypassedShare = 0.0f;  // eased: 0 is the normal look, 1 the bypassed one

        bool operator== (const Look& other) const
        {
            return litWindowCount == other.litWindowCount && blend == other.blend
                && juce::exactlyEqual (bypassedShare, other.bypassedShare);
        }
    };

    Look lookFor (float citySizePercent, float bypassedShare) const;
    void rebuildLitLayer();
    void drawKeyframeBlend (juce::Graphics& g, const SkylineKeyframes::Frames& frames) const;
    void drawNormal (juce::Graphics& g, float opacity) const;
    void drawBypassed (juce::Graphics& g) const;
    juce::Rectangle<float> windowBounds (juce::Point<int> window) const;

    // First, so everything below can read the keyframes' size.
    const juce::SharedResourcePointer<SkylineKeyframes> keyframes;
    const juce::Image overlay;
    const std::vector<juce::Point<int>> windows;
    const int pixelsPerPoint;

    // The lit windows with their glow, rebuilt only when the lit count changes.
    const juce::Image glowSprite;
    juce::Image litLayer;
    SkylineBypassFade bypassFade;
    Look look;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Skyline)
};
