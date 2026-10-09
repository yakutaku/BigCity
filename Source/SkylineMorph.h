#pragma once

#include <juce_graphics/juce_graphics.h>

#include <vector>

// How City Size maps onto the skyline (docs/ui-spec.md "Skyline" morph). Pure, so it's tested without a window.
namespace SkylineMorph
{
    // floor(windowCount × 0.6 × s/100): 0, 52, 105, 158 and 211 of 353 at 0, 25, 50, 75 and 100.
    int litWindowCount (float citySizePercent, int windowCount);

    // skyline_grey's alpha over skyline_colour: 0.3 at 0, so the skyline is 70% saturated, down to 0 at 100.
    float greyLayerAlpha (float citySizePercent);

    // The windows in lighting order, in grid pixels, from assets/skyline-windows.json. A file that doesn't
    // parse, or that puts any window off the grid, gives no windows: the skyline then just never lights up.
    std::vector<juce::Point<int>> parseWindows (const juce::String& json);
}
