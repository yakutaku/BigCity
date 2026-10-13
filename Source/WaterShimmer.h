#pragma once

#include "UiSpec.h"

#include <juce_graphics/juce_graphics.h>

#include <array>

// What the water shows at a moment (docs/ui-spec.md "Water shimmer", D-28): the glints' highlight and the
// sparkles, in skyline grid pixels. Pure, so the motion can be tested without a window.
namespace WaterShimmer
{
    constexpr auto glintCount = UiSpec::Skyline::glintRows.size();

    struct Sparkle
    {
        juce::Point<int> cell;
        float alpha = 0.0f;

        bool operator== (const Sparkle& other) const
        {
            return cell == other.cell && juce::exactlyEqual (alpha, other.alpha);
        }
    };

    struct Frame
    {
        float glintAlpha = 0.0f;
        std::array<int, glintCount> glintOffsets {};   // grid pixels, −1 to +1
        std::array<Sparkle, UiSpec::Skyline::maxSparkles> sparkles {};
        int sparkleCount = 0;

        bool operator== (const Frame& other) const
        {
            return juce::exactlyEqual (glintAlpha, other.glintAlpha) && glintOffsets == other.glintOffsets
                && sparkles == other.sparkles && sparkleCount == other.sparkleCount;
        }
    };

    // The moving water at this time. At City size 0 it is the empty frame: nothing drawn.
    Frame frameAt (double seconds, float citySizePercent);

    // The frame for macOS "Reduce motion": the glints as at time 0, with no sparkles.
    Frame stillFrame (float citySizePercent);

    // Where glint `index`'s highlight sits, shifted by its offset.
    juce::Rectangle<int> glintCells (size_t index, int offset);

    // The water's rows in grid pixels: everything the shimmer can draw on.
    juce::Rectangle<int> waterCells();
}
