#include "CityMacro.h"

#include <juce_core/juce_core.h>

#include <cmath>

class CityMacroTests final : public juce::UnitTest
{
public:
    CityMacroTests() : juce::UnitTest ("CityMacro", "BigCity") {}

    void runTest() override
    {
        constexpr float tolerance = 1.0e-4f;

        beginTest ("At 0: no air, wet signal off");
        {
            const auto settings = CityMacro::map (0.0f);
            expectEquals (settings.airGainDb, 0.0f);
            expectWithinAbsoluteError (settings.detuneCents, 5.0f, tolerance);
            expect (std::isinf (settings.wetLevelDb) && settings.wetLevelDb < 0.0f, "wet level should be minus infinity");
        }

        beginTest ("At 50: +3 dB air, +/-7 cents, wet -12 dB");
        {
            const auto settings = CityMacro::map (50.0f);
            expectWithinAbsoluteError (settings.airGainDb, 3.0f, tolerance);
            expectWithinAbsoluteError (settings.detuneCents, 7.0f, tolerance);
            expectWithinAbsoluteError (settings.wetLevelDb, -12.0f, tolerance);
        }

        beginTest ("At 100: +6 dB air, +/-10 cents, wet -6 dB");
        {
            const auto settings = CityMacro::map (100.0f);
            expectWithinAbsoluteError (settings.airGainDb, 6.0f, tolerance);
            expectWithinAbsoluteError (settings.detuneCents, 10.0f, tolerance);
            expectWithinAbsoluteError (settings.wetLevelDb, -6.0f, tolerance);
        }

        beginTest ("Tuned maxima rescale each curve and keep its shape");
        {
            SoundTuning tuning;
            tuning.airMaxGainDb = 9.0f;
            tuning.airCornerHz = 10000.0;
            tuning.airShelfQ = 0.8;
            tuning.wetLevelDbAtMax = -3.0f;
            tuning.detuneCentsAtMax = 14.0f;

            const auto atMax = CityMacro::map (100.0f, tuning);
            expectWithinAbsoluteError (atMax.airGainDb, 9.0f, tolerance);
            expectWithinAbsoluteError (atMax.wetLevelDb, -3.0f, tolerance);
            expectWithinAbsoluteError (atMax.detuneCents, 14.0f, tolerance);
            expectEquals (atMax.airCornerHz, 10000.0);
            expectEquals (atMax.airShelfQ, 0.8);

            // The midpoint moves with the maximum: still 6 dB below it, still 7/10 of the detune.
            const auto atMidpoint = CityMacro::map (50.0f, tuning);
            expectWithinAbsoluteError (atMidpoint.wetLevelDb, -9.0f, tolerance);
            expectWithinAbsoluteError (atMidpoint.detuneCents, 9.8f, tolerance);

            const auto atZero = CityMacro::map (0.0f, tuning);
            expectEquals (atZero.airGainDb, 0.0f);
            expect (std::isinf (atZero.wetLevelDb) && atZero.wetLevelDb < 0.0f, "wet level should still be off at 0");
        }

        beginTest ("Default tuning hits the PRD points exactly, so shipping builds are unchanged");
        {
            const auto atMidpoint = CityMacro::map (50.0f);
            const auto atMax = CityMacro::map (100.0f);
            expect (juce::exactlyEqual (atMidpoint.detuneCents, SoundSpec::detuneCentsAtMidpoint));
            expect (juce::exactlyEqual (atMidpoint.wetLevelDb, SoundSpec::wetLevelDbAtMidpoint));
            expect (juce::exactlyEqual (atMax.airGainDb, SoundSpec::airMaxGainDb));
            expect (juce::exactlyEqual (atMax.detuneCents, SoundSpec::detuneCentsAtMax));
            expect (juce::exactlyEqual (atMax.wetLevelDb, SoundSpec::wetLevelDbAtMax));
            expect (juce::exactlyEqual (atMax.airCornerHz, SoundSpec::airCornerHz));
            expect (juce::exactlyEqual (atMax.airShelfQ, SoundSpec::airShelfQ));
        }

        beginTest ("Out-of-range input is clamped");
        {
            expectEquals (CityMacro::map (-5.0f).airGainDb, 0.0f);
            expectWithinAbsoluteError (CityMacro::map (150.0f).airGainDb, 6.0f, tolerance);
        }
    }
};

static CityMacroTests cityMacroTests;
