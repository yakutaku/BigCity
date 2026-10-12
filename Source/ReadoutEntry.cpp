#include "ReadoutEntry.h"
#include "SoundSpec.h"

namespace
{
    const juce::String digitCharacters { "0123456789" };
    constexpr juce::juce_wchar decimalPoint = '.';

    // docs/ui-spec.md "Readout": 3½ digits, so the hundreds can only be the half digit's "1"
    constexpr int maxIntegerDigits = 3;
    constexpr juce::juce_wchar onlyHundredsDigit = '1';
    constexpr int maxFractionDigits = 1;

    juce::String integerPartOf (const juce::String& text)
    {
        return text.upToFirstOccurrenceOf (juce::String::charToString (decimalPoint), false, false);
    }

    juce::String fractionPartOf (const juce::String& text)
    {
        return text.fromFirstOccurrenceOf (juce::String::charToString (decimalPoint), false, false);
    }

    bool hasOnlyDigitsAndOnePoint (const juce::String& text)
    {
        return text.containsOnly (digitCharacters + decimalPoint)
            && text.indexOfChar (decimalPoint) == text.lastIndexOfChar (decimalPoint);
    }

    bool isUnsignedNumber (const juce::String& text)
    {
        return text.containsAnyOf (digitCharacters) && hasOnlyDigitsAndOnePoint (text);
    }

    bool fitsTheLcd (const juce::String& text)
    {
        if (! hasOnlyDigitsAndOnePoint (text))
            return false;

        const auto integerPart = integerPartOf (text);
        const auto integerFits = integerPart.length() < maxIntegerDigits
                              || (integerPart.length() == maxIntegerDigits && integerPart[0] == onlyHundredsDigit);

        return integerFits && fractionPartOf (text).length() <= maxFractionDigits;
    }

    int digitAt (const juce::String& part, int index)
    {
        return juce::isPositiveAndBelow (index, part.length()) ? (int) (part[index] - '0') : SevenSegment::blank;
    }

    // The text that shows the same digits, so an untouched entry looks exactly like the readout it replaced.
    juce::String textFor (const SevenSegment::Digits& digits)
    {
        juce::String text;

        if (digits.halfDigitLit)
            text << onlyHundredsDigit;

        if (digits.tens != SevenSegment::blank)
            text << digits.tens;

        return text << digits.ones << decimalPoint << digits.tenths;
    }
}

std::optional<float> ReadoutEntry::parseCitySize (const juce::String& text)
{
    const auto trimmed = text.trim();
    const auto hasSign = trimmed.startsWithChar ('-') || trimmed.startsWithChar ('+');

    if (! isUnsignedNumber (hasSign ? trimmed.substring (1) : trimmed))
        return std::nullopt;

    return juce::jlimit (SoundSpec::citySizeMinPercent, SoundSpec::citySizeMaxPercent, trimmed.getFloatValue());
}

ReadoutEntry::TypedValue::TypedValue (float citySizePercent)
    : text (textFor (SevenSegment::digitsFor (citySizePercent)))
{
}

bool ReadoutEntry::TypedValue::type (juce::juce_wchar character)
{
    const auto candidate = (selected ? juce::String() : text) + juce::String::charToString (character);

    if (! fitsTheLcd (candidate))
        return false;

    text = candidate;
    selected = false;
    return true;
}

void ReadoutEntry::TypedValue::deleteBackwards()
{
    text = selected ? juce::String() : text.dropLastCharacters (1);
    selected = false;
}

SevenSegment::Digits ReadoutEntry::TypedValue::digits() const
{
    const auto integerPart = integerPartOf (text);
    const auto integerDigitCount = integerPart.length();

    return { integerDigitCount == maxIntegerDigits,
             digitAt (integerPart, integerDigitCount - 2),
             digitAt (integerPart, integerDigitCount - 1),
             digitAt (fractionPartOf (text), 0) };
}
