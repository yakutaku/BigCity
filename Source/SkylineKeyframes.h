#pragma once

#include "UiSpec.h"

#include <juce_graphics/juce_graphics.h>

#include <array>

// The time-of-day keyframes (D-27, docs/ui-spec.md "Skyline") and their greyscale copies for the bypassed
// look, decoded once and shared by every open editor. Hold it through juce::SharedResourcePointer: the first
// editor to open loads it and the last to close frees it, so closed plugins keep none of these images.
struct SkylineKeyframes
{
    using Frames = std::array<juce::Image, UiSpec::Skyline::keyframeCount>;

    SkylineKeyframes();

    const Frames colour;
    // Greyscale is linear, so crossfading these gives the greyscale of the colour crossfade.
    const Frames grey;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SkylineKeyframes)
};
