#include "ReadoutEntry.h"

class ReadoutEntryTests final : public juce::UnitTest
{
public:
    ReadoutEntryTests() : juce::UnitTest ("ReadoutEntry", "BigCity") {}

    void runTest() override
    {
        using ReadoutEntry::parseCitySize;
        using ReadoutEntry::TypedValue;
        using SevenSegment::blank;

        beginTest ("Parse: plain numbers commit as typed");
        {
            expectParses ("50", 50.0f);
            expectParses ("50.25", 50.25f);
            expectParses ("0", 0.0f);
            expectParses ("100", 100.0f);
            expectParses (".5", 0.5f);
            expectParses ("7.", 7.0f);
        }

        beginTest ("Parse: out-of-range numbers clamp to 0-100");
        {
            expectParses ("150", 100.0f);
            expectParses ("199.9", 100.0f);
            expectParses ("-3", 0.0f);
        }

        beginTest ("Parse: anything that isn't a number keeps the old value");
        {
            for (const auto* text : { "abc", "", " ", ".", "-", "5a", "1.2.3", "--3" })
                expect (! parseCitySize (text).has_value(), "\"" + juce::String (text) + "\" should not parse");
        }

        beginTest ("Typed value: opens with the current value selected, showing the readout's digits");
        {
            for (const auto percent : { 0.0f, 5.0f, 50.0f, 69.6f, 100.0f })
            {
                const TypedValue typed { percent };
                expect (typed.isSelected());
                expect (typed.digits() == SevenSegment::digitsFor (percent), "digits at " + juce::String (percent));
                expectParses (typed.getText(), percent);
            }
        }

        beginTest ("Typed value: the first key replaces the selection");
        {
            TypedValue typed { 50.0f };
            expect (typed.type ('7'));
            expect (! typed.isSelected());
            expectEquals (typed.getText(), juce::String ("7"));
            expect (typed.digits() == SevenSegment::Digits { false, blank, 7, blank });
        }

        beginTest ("Typed value: digits fill right to left, the decimal digit goes in the tenths");
        {
            const auto typed = typedFrom ("42.5");
            expect (typed.digits() == SevenSegment::Digits { false, 4, 2, 5 });
            expect (typedFrom ("100.0").digits() == SevenSegment::Digits { true, 0, 0, 0 });
            expect (typedFrom (".5").digits() == SevenSegment::Digits { false, blank, blank, 5 });
        }

        beginTest ("Typed value: Backspace clears the selection, then deletes the last character");
        {
            TypedValue typed { 50.0f };
            typed.deleteBackwards();
            expect (! typed.isSelected());
            expect (typed.getText().isEmpty());

            auto edited = typedFrom ("42.5");
            edited.deleteBackwards();
            expectEquals (edited.getText(), juce::String ("42."));
            typed.deleteBackwards();
            expect (typed.getText().isEmpty(), "Backspace on nothing stays empty");
        }

        beginTest ("Typed value: refuses what the LCD can't show");
        {
            auto typed = typedFrom ("42.5");
            expect (! typed.type ('1'), "a second decimal digit");
            expect (! typedFrom ("4.").type ('.'), "a second decimal point");
            expect (! typedFrom ("20").type ('0'), "a hundreds digit other than 1");
            expect (typedFrom ("19").type ('9'), "199 fits: the half digit shows the 1");
            expect (! typedFrom ("199").type ('9'), "a fourth integer digit");

            for (const auto character : { (juce::juce_wchar) '-', (juce::juce_wchar) 'a', (juce::juce_wchar) ' ', (juce::juce_wchar) ',' })
                expect (! TypedValue { 50.0f }.type (character), "'" + juce::String::charToString (character) + "' isn't a digit or a point");

            expectEquals (typed.getText(), juce::String ("42.5"), "refused keys change nothing");
        }

        beginTest ("Typed value: a refused first key keeps the selection");
        {
            TypedValue typed { 50.0f };
            typed.type ('x');
            expect (typed.isSelected());
            expectEquals (typed.getText(), juce::String ("50.0"));
        }
    }

private:
    void expectParses (const juce::String& text, float expectedPercent)
    {
        const auto parsed = ReadoutEntry::parseCitySize (text);
        expect (parsed.has_value(), "\"" + text + "\" should parse");

        if (parsed.has_value())
            expectWithinAbsoluteError (*parsed, expectedPercent, 1.0e-4f, "\"" + text + "\"");
    }

    // A fresh entry with these characters typed over the selection; every one must be accepted.
    ReadoutEntry::TypedValue typedFrom (const juce::String& characters)
    {
        ReadoutEntry::TypedValue typed { 50.0f };

        for (const auto character : characters)
            expect (typed.type (character), "typing \"" + characters + "\"");

        return typed;
    }
};

static ReadoutEntryTests readoutEntryTests;
