#pragma once

#include <juce_graphics/juce_graphics.h>

#include <vector>

// How City Size maps onto the skyline (docs/ui-spec.md "Skyline" morph). Pure, so it's tested without a window.
namespace SkylineMorph
{
    // floor(windowCount × 0.6 × s/100): 0, 52, 105, 158 and 211 of 353 at 0, 25, 50, 75 and 100.
    int litWindowCount (float citySizePercent, int windowCount);

    // The two time-of-day keyframes either side of a City Size (D-27), and how much of the upper one shows
    // over the lower: a linear crossfade.
    struct KeyframeBlend
    {
        int lowerIndex = 0;  // the upper keyframe is lowerIndex + 1
        float upperAlpha = 0.0f;

        bool operator== (const KeyframeBlend& other) const
        {
            return lowerIndex == other.lowerIndex && juce::exactlyEqual (upperAlpha, other.upperAlpha);
        }
    };

    // Keyframes sit evenly from 0 to 100, e.g. 12.5 is halfway from the first to the second. 100 is the last
    // keyframe shown fully, so the upper index never runs past the end.
    KeyframeBlend keyframeBlendFor (float citySizePercent);

    // The windows in lighting order, in grid pixels, from assets/skyline-windows.json. A file that doesn't
    // parse, or that puts any window off the grid, gives no windows: the skyline then just never lights up.
    std::vector<juce::Point<int>> parseWindows (const juce::String& json);
}
