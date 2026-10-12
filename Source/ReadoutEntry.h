#pragma once

#include "SevenSegment.h"

#include <juce_core/juce_core.h>

#include <optional>

// Typing a value into the readout (docs/ui-spec.md "Readout" → "Editing"). No component or host here,
// so the rules can be tested on their own.
namespace ReadoutEntry
{
    // The citySize percent that typed text commits, clamped to 0–100; nothing if the text isn't a number,
    // so the old value stays. Snapping to the 0.1 step is left to the parameter.
    std::optional<float> parseCitySize (const juce::String& text);

    // What has been typed so far. It opens with the current value selected, so the first key replaces it.
    // It accepts only what the 3½-digit LCD can show, so every typed state can be drawn as segments.
    class TypedValue
    {
    public:
        explicit TypedValue (float citySizePercent);

        // Ignores a character the LCD couldn't show; returns whether it was taken.
        bool type (juce::juce_wchar character);
        void deleteBackwards();

        bool isSelected() const { return selected; }
        const juce::String& getText() const { return text; }

        // Right-aligned to the ones cell, with blanks where nothing has been typed.
        SevenSegment::Digits digits() const;

    private:
        juce::String text;
        bool selected = true;
    };
}
