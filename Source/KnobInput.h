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

    // A mouse wheel moves a fixed step per notch, whatever size the notch reports.
    float valueAfterNotch (float citySizePercent, const juce::MouseWheelDetails& wheel, bool fine);

    // How far a trackpad scroll moved, in the same pixels as a drag, so a swipe moves the knob as far as a drag.
    float trackpadPixelsUp (const juce::MouseWheelDetails& wheel);

    // Ignores trackpad movement within one scroll burst until it adds up to a deliberate swipe, so a light
    // brush of the fingers doesn't move the knob. Beyond that, movement passes through unscaled.
    class TrackpadDeadZone
    {
    public:
        // The part of this movement that lies outside the dead zone, measured from where the burst started.
        float pixelsToApply (float pixelsUp);

        // Call when a scroll burst ends, so the next one starts inside the dead zone again.
        void reset();

    private:
        float pixelsInBurst = 0.0f;
    };
}
