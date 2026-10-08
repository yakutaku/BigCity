#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// How mouse and wheel input moves the City Size knob (docs/ui-spec.md "Knob", PRD §7).
// Values are citySize percent; every result is clamped to the parameter's range.
namespace KnobInput
{
    // The rotor angle, clockwise from 12 o'clock.
    float angleRadiansFor (float citySizePercent);

    // A drag of pixelsUp (negative for down) from the value at the last drag event.
    float valueAfterDrag (float citySizePercent, float pixelsUp, bool fine);

    // One wheel event: a mouse wheel moves a fixed step per notch, a trackpad as far as a drag of the same distance.
    float valueAfterWheel (float citySizePercent, const juce::MouseWheelDetails& wheel, bool fine);
}
