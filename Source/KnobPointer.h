#pragma once

#include <juce_graphics/juce_graphics.h>

// The knob's pointer, drawn as a vector shape so its edges stay crisp at every angle, where a rotated
// bitmap softens by a different amount at each angle and shimmers (docs/ui-spec.md "Knob", D-25).
namespace KnobPointer
{
    // The pointer's outline at this rotor angle, in knob-image-local points.
    juce::Path outlineAt (float angleRadians);

    void draw (juce::Graphics& g, float angleRadians);
}
