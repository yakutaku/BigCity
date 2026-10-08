#pragma once

#include <juce_graphics/juce_graphics.h>

// The readout's 3½-digit 7-segment display (docs/ui-spec.md "Readout", D-21): which digits a value shows
// and the outline of their lit segments.
namespace SevenSegment
{
    constexpr int blank = -1;

    struct Digits
    {
        bool halfDigitLit;  // the leading "1" of 100.0
        int tens;           // blank for a leading zero
        int ones;
        int tenths;

        bool operator== (const Digits&) const = default;
    };

    // citySize rounded to 0.1 and clamped to 0–100, e.g. 5 shows as " 5.0" and 100 as "100.0".
    Digits digitsFor (float citySizePercent);

    // Every lit segment and the decimal point, in LCD-local points.
    juce::Path litSegmentsFor (const Digits& digits);
}
