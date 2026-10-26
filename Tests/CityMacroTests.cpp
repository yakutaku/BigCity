#include "CityMacro.h"
#include "SoundSpec.h"

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
            const auto settings = CityMacro::map (0.0f, Voicing::original);
            expectEquals (settings.airGainDb, 0.0f);
            expectWithinAbsoluteError (settings.detuneCents, 5.5f, tolerance);
            expect (std::isinf (settings.wetLevelDb) && settings.wetLevelDb < 0.0f, "wet level should be minus infinity");
        }

        beginTest ("At 50 the knob response reads the curves at 70: +5.46 dB air, +/-9.02 cents, wet -6.6 dB");
        {
            const auto settings = CityMacro::map (50.0f, Voicing::original);
            expectWithinAbsoluteError (settings.airGainDb, 5.46f, tolerance);
            expectWithinAbsoluteError (settings.detuneCents, 9.02f, tolerance);
            expectWithinAbsoluteError (settings.wetLevelDb, -6.6f, tolerance);
        }

        beginTest ("The knob response bends 25 to 40 and 75 to 90, and keeps rising");
        {
            // The air curve is linear, so its gain shows the bent position directly.
            expectWithinAbsoluteError (CityMacro::map (25.0f, Voicing::original).airGainDb, 7.8f * 0.40f, tolerance);
            expectWithinAbsoluteError (CityMacro::map (75.0f, Voicing::original).airGainDb, 7.8f * 0.90f, tolerance);

            for (float citySize = 0.1f; citySize <= 100.0f; citySize += 0.1f)
                expectGreaterThan (CityMacro::map (citySize, Voicing::original).airGainDb, CityMacro::map (citySize - 0.1f, Voicing::original).airGainDb);
        }

        beginTest ("At 100: +7.8 dB air, +/-11 cents, wet -3 dB");
        {
            const auto settings = CityMacro::map (100.0f, Voicing::original);
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

            const auto atMax = CityMacro::map (100.0f, Voicing::original, tuning);
            expectWithinAbsoluteError (atMax.airGainDb, 9.0f, tolerance);
            expectWithinAbsoluteError (atMax.wetLevelDb, -5.0f, tolerance);
            expectWithinAbsoluteError (atMax.detuneCents, 14.0f, tolerance);
            expectEquals (atMax.airCornerHz, 10000.0);
            expectEquals (atMax.airShelfQ, 0.8);

            // Everywhere else moves with the maximum: the wet level by the same dB, the detune in proportion.
            const auto atMidpoint = CityMacro::map (50.0f, Voicing::original, tuning);
            const auto defaultAtMidpoint = CityMacro::map (50.0f, Voicing::original);
            expectWithinAbsoluteError (atMidpoint.wetLevelDb, defaultAtMidpoint.wetLevelDb - 2.0f, tolerance);
            expectWithinAbsoluteError (atMidpoint.detuneCents, defaultAtMidpoint.detuneCents * 14.0f / 11.0f, tolerance);

            const auto atZero = CityMacro::map (0.0f, Voicing::original, tuning);
            expectEquals (atZero.airGainDb, 0.0f);
            expect (std::isinf (atZero.wetLevelDb) && atZero.wetLevelDb < 0.0f, "wet level should still be off at 0");
        }

        beginTest ("At the default tuning, 100 hits the PRD maxima exactly");
        {
            const auto atMax = CityMacro::map (100.0f, Voicing::original);
            expect (juce::exactlyEqual (atMax.airGainDb, SoundSpec::airMaxGainDb));
            expect (juce::exactlyEqual (atMax.detuneCents, SoundSpec::detuneCentsAtMax));
            expect (juce::exactlyEqual (atMax.wetLevelDb, SoundSpec::wetLevelDbAtMax));
            expect (juce::exactlyEqual (atMax.airCornerHz, SoundSpec::airCornerHz));
            expect (juce::exactlyEqual (atMax.airShelfQ, SoundSpec::airShelfQ));
        }

        beginTest ("Out-of-range input is clamped");
        {
            expectEquals (CityMacro::map (-5.0f, Voicing::original).airGainDb, 0.0f);
            expectWithinAbsoluteError (CityMacro::map (150.0f, Voicing::original).airGainDb, 7.8f, tolerance);
        }

        testVoicing2();
    }

private:
    static bool settingsMatchExactly (const CitySettings& a, const CitySettings& b)
    {
        return juce::exactlyEqual (a.airGainDb, b.airGainDb) && juce::exactlyEqual (a.airCornerHz, b.airCornerHz)
            && juce::exactlyEqual (a.airShelfQ, b.airShelfQ) && juce::exactlyEqual (a.detuneCents, b.detuneCents)
            && juce::exactlyEqual (a.wetLevelDb, b.wetLevelDb);
    }

    void testVoicing2()
    {
        using namespace SoundSpec;
        const auto voicing2At = [] (float citySize) { return CityMacro::map (citySize, Voicing::doubleRange); };

        beginTest ("Voicing 2: up to 50 it is voicing 1 at twice the position, bit for bit");
        {
            for (int tenths = 0; tenths <= 500; ++tenths)
            {
                const auto citySize = (float) tenths / 10.0f;
                expect (settingsMatchExactly (voicing2At (citySize), CityMacro::map (2.0f * citySize, Voicing::original)),
                        "differs at " + juce::String (citySize));
            }

            expect (settingsMatchExactly (voicing2At (50.0f), CityMacro::map (100.0f, Voicing::original)), "50 isn't voicing 1's 100");
        }

        beginTest ("Voicing 2 at 75: +8.74 dB air, +/-11.44 cents, wet -2.16 dB");
        {
            const auto settings = voicing2At (75.0f);
            expectWithinAbsoluteError (settings.airGainDb, 8.74f, 0.005f);
            expectWithinAbsoluteError (settings.detuneCents, 11.44f, 0.005f);
            expectWithinAbsoluteError (settings.wetLevelDb, -2.16f, 0.005f);
        }

        beginTest ("Voicing 2 at 100: +10 dB air, +/-11.5 cents, wet -2 dB");
        {
            const auto atMax = voicing2At (100.0f);
            expectWithinAbsoluteError (atMax.airGainDb, voicing2AirGainDbAtMax, 1.0e-4f);
            expectWithinAbsoluteError (atMax.detuneCents, voicing2DetuneCentsAtMax, 1.0e-4f);
            expectWithinAbsoluteError (atMax.wetLevelDb, voicing2WetLevelDbAtMax, 1.0e-4f);
            expect (settingsMatchExactly (voicing2At (150.0f), atMax), "above 100 isn't clamped");
        }

        beginTest ("Voicing 2: every element never turns down from 0 to 100");
        {
            for (int tenths = 1; tenths <= 1000; ++tenths)
            {
                const auto before = voicing2At ((float) (tenths - 1) / 10.0f);
                const auto after = voicing2At ((float) tenths / 10.0f);
                const auto where = " at " + juce::String ((float) tenths / 10.0f);
                expect (after.airGainDb >= before.airGainDb, "air turns down" + where);
                expect (after.detuneCents >= before.detuneCents, "detune turns down" + where);
                expect (after.wetLevelDb >= before.wetLevelDb, "wet level turns down" + where);
            }
        }

        beginTest ("Voicing 2: no corner at 50");
        {
            constexpr float step = 0.05f;
            const auto below = voicing2At (50.0f - step), at = voicing2At (50.0f), above = voicing2At (50.0f + step);

            const auto expectNoCorner = [&] (float before, float middle, float after, const juce::String& element)
            {
                const auto slopeBelow = (middle - before) / step, slopeAbove = (after - middle) / step;
                expectWithinAbsoluteError (slopeAbove, slopeBelow, 0.03f * slopeBelow, element + ": slope " + juce::String (slopeBelow) + " then " + juce::String (slopeAbove));
            };

            expectNoCorner (below.airGainDb, at.airGainDb, above.airGainDb, "air");
            expectNoCorner (below.detuneCents, at.detuneCents, above.detuneCents, "detune");
            expectNoCorner (below.wetLevelDb, at.wetLevelDb, above.wetLevelDb, "wet level");
        }
    }
};

static CityMacroTests cityMacroTests;
