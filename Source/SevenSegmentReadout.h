#pragma once

#include "ReadoutEntry.h"
#include "SevenSegment.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <optional>

// The value readout in the LCD under the knob (docs/ui-spec.md "Readout"). The LCD and its burned-in
// "188.8" are part of the panel; this draws only the lit segments. Double-click to type a value: Enter
// or a click anywhere else commits it to citySize in one automation gesture, and Esc cancels.
class SevenSegmentReadout final : public juce::Component
{
public:
    explicit SevenSegmentReadout (juce::AudioParameterFloat& citySizeParameter);

    // Called from the editor's repaint timer; repaints only when the shown digits change. Ignored while
    // editing, so the host can't overwrite what is being typed.
    void setDisplayedPercent (float citySizePercent);

    // Opens text entry with the current value selected. The knob's double-click calls this too.
    void beginEditing();
    bool isEditing() const { return typedValue.has_value(); }

    void paint (juce::Graphics& g) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    bool keyPressed (const juce::KeyPress& key) override;
    void focusLost (FocusChangeType cause) override;

private:
    void commitEditing();
    void endEditing();
    void showDigits (const SevenSegment::Digits& digits);
    void sendToHost (float citySizePercent);
    void drawEditingFrame (juce::Graphics& g) const;

    juce::AudioParameterFloat& citySize;
    SevenSegment::Digits displayedDigits;
    juce::Path litSegments;  // LCD-local, rebuilt only when the digits change

    // Set only while editing.
    std::optional<ReadoutEntry::TypedValue> typedValue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SevenSegmentReadout)
};
