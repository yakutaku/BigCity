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
            expectWithinAbsoluteError (settings.detuneCents, 5.5f, tolerance);
            expect (std::isinf (settings.wetLevelDb) && settings.wetLevelDb < 0.0f, "wet level should be minus infinity");
        }

        beginTest ("At 50 the knob response reads the curves at 70: +5.46 dB air, +/-9.02 cents, wet -6.6 dB");
        {
            const auto settings = CityMacro::map (50.0f);
            expectWithinAbsoluteError (settings.airGainDb, 5.46f, tolerance);
            expectWithinAbsoluteError (settings.detuneCents, 9.02f, tolerance);
            expectWithinAbsoluteError (settings.wetLevelDb, -6.6f, tolerance);
        }

        beginTest ("The knob response bends 25 to 40 and 75 to 90, and keeps rising");
        {
            // The air curve is linear, so its gain shows the bent position directly.
            expectWithinAbsoluteError (CityMacro::map (25.0f).airGainDb, 7.8f * 0.40f, tolerance);
            expectWithinAbsoluteError (CityMacro::map (75.0f).airGainDb, 7.8f * 0.90f, tolerance);

            for (float citySize = 0.1f; citySize <= 100.0f; citySize += 0.1f)
                expectGreaterThan (CityMacro::map (citySize).airGainDb, CityMacro::map (citySize - 0.1f).airGainDb);
        }

        beginTest ("At 100: +7.8 dB air, +/-11 cents, wet -3 dB");
        {
            const auto settings = CityMacro::map (100.0f);
            expectWithinAbsoluteError (settings.airGainDb, 7.8f, tolerance);
            expectWithinAbsoluteError (settings.detuneCents, 11.0f, tolerance);
            expectWithinAbsoluteError (settings.wetLevelDb, -3.0f, tolerance);
        }

        beginTest ("Tuned maxima rescale each curve and keep its shape");
        {
            SoundTuning tuning;
            tuning.airMaxGainDb = 9.0f;
            tuning.airCornerHz = 10000.0;
            tuning.airShelfQ = 0.8;
            tuning.wetLevelDbAtMax = -5.0f;
            tuning.detuneCentsAtMax = 14.0f;

            const auto atMax = CityMacro::map (100.0f, tuning);
            expectWithinAbsoluteError (atMax.airGainDb, 9.0f, tolerance);
            expectWithinAbsoluteError (atMax.wetLevelDb, -5.0f, tolerance);
            expectWithinAbsoluteError (atMax.detuneCents, 14.0f, tolerance);
            expectEquals (atMax.airCornerHz, 10000.0);
            expectEquals (atMax.airShelfQ, 0.8);

            // Everywhere else moves with the maximum: the wet level by the same dB, the detune in proportion.
            const auto atMidpoint = CityMacro::map (50.0f, tuning);
            const auto defaultAtMidpoint = CityMacro::map (50.0f);
            expectWithinAbsoluteError (atMidpoint.wetLevelDb, defaultAtMidpoint.wetLevelDb - 2.0f, tolerance);
            expectWithinAbsoluteError (atMidpoint.detuneCents, defaultAtMidpoint.detuneCents * 14.0f / 11.0f, tolerance);

            const auto atZero = CityMacro::map (0.0f, tuning);
            expectEquals (atZero.airGainDb, 0.0f);
            expect (std::isinf (atZero.wetLevelDb) && atZero.wetLevelDb < 0.0f, "wet level should still be off at 0");
        }

        beginTest ("At the default tuning, 100 hits the PRD maxima exactly");
        {
            const auto atMax = CityMacro::map (100.0f);
            expect (juce::exactlyEqual (atMax.airGainDb, SoundSpec::airMaxGainDb));
            expect (juce::exactlyEqual (atMax.detuneCents, SoundSpec::detuneCentsAtMax));
            expect (juce::exactlyEqual (atMax.wetLevelDb, SoundSpec::wetLevelDbAtMax));
            expect (juce::exactlyEqual (atMax.airCornerHz, SoundSpec::airCornerHz));
            expect (juce::exactlyEqual (atMax.airShelfQ, SoundSpec::airShelfQ));
        }

        beginTest ("Out-of-range input is clamped");
        {
            expectEquals (CityMacro::map (-5.0f).airGainDb, 0.0f);
            expectWithinAbsoluteError (CityMacro::map (150.0f).airGainDb, 7.8f, tolerance);
        }
    }
};

static CityMacroTests cityMacroTests;
