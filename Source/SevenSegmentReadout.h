#pragma once

#include "SevenSegment.h"

#include <juce_gui_basics/juce_gui_basics.h>

// The value readout in the LCD under the knob (docs/ui-spec.md "Readout"). The LCD and its burned-in
// "188.8" are part of the panel; this draws only the lit segments. Text entry comes in a later task.
class SevenSegmentReadout final : public juce::Component
{
public:
    explicit SevenSegmentReadout (float initialCitySizePercent);

    // Called from the editor's repaint timer; repaints only when the shown digits change.
    void setDisplayedPercent (float citySizePercent);

    void paint (juce::Graphics& g) override;

private:
    SevenSegment::Digits displayedDigits;
    juce::Path litSegments;  // LCD-local, rebuilt only when the digits change

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SevenSegmentReadout)
};
