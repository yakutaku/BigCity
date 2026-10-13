#include "AutoGain.h"
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

        beginTest ("At 50: +3.9 dB air, +/-7.7 cents, wet -9 dB");
        {
            const auto settings = CityMacro::map (50.0f);
            expectWithinAbsoluteError (settings.airGainDb, 3.9f, tolerance);
            expectWithinAbsoluteError (settings.detuneCents, 7.7f, tolerance);
            expectWithinAbsoluteError (settings.wetLevelDb, -9.0f, tolerance);
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

            // The midpoint moves with the maximum: still 6 dB below it, still 7/10 of the detune.
            const auto atMidpoint = CityMacro::map (50.0f, tuning);
            expectWithinAbsoluteError (atMidpoint.wetLevelDb, -11.0f, tolerance);
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

        beginTest ("Every voicing keeps City Size 0 off and holds 0 dB auto gain there");
        {
            for (const auto voicing : { Voicing::current, Voicing::a, Voicing::b, Voicing::c })
            {
                SoundTuning tuning;
                tuning.voicing = voicing;
                const auto atZero = CityMacro::map (0.0f, tuning);
                expectEquals (atZero.airGainDb, 0.0f);
                expect (std::isinf (atZero.wetLevelDb) && atZero.wetLevelDb < 0.0f, "wet level should be off at 0");
                expect (juce::exactlyEqual (AutoGain::gainDbFor (0.0f, voicing), 0.0f), "auto gain not exactly 0 dB at 0");
            }
        }

        beginTest ("Voicing A: 50 is today's 70, 0 and 100 unchanged");
        {
            SoundTuning a;
            a.voicing = Voicing::a;
            const auto today = CityMacro::map (70.0f);
            const auto atMidpoint = CityMacro::map (50.0f, a);
            expectWithinAbsoluteError (atMidpoint.airGainDb, today.airGainDb, tolerance);
            expectWithinAbsoluteError (atMidpoint.detuneCents, today.detuneCents, tolerance);
            expectWithinAbsoluteError (atMidpoint.wetLevelDb, today.wetLevelDb, tolerance);
            expectWithinAbsoluteError (CityMacro::map (100.0f, a).airGainDb, 7.8f, tolerance);

            // The bend keeps rising, so the knob never turns the effect down.
            for (float citySize = 1.0f; citySize <= 100.0f; citySize += 1.0f)
                expectGreaterThan (CityMacro::map (citySize, a).airGainDb, CityMacro::map (citySize - 1.0f, a).airGainDb);
        }

        beginTest ("Voicing B is A plus 15% at 100; voicing C is 25% everywhere");
        {
            SoundTuning b, c;
            b.voicing = Voicing::b;
            c.voicing = Voicing::c;
            const auto bAtMax = CityMacro::map (100.0f, b);
            expectWithinAbsoluteError (bAtMax.airGainDb, 7.8f * 1.15f, tolerance);
            expectWithinAbsoluteError (bAtMax.detuneCents, 11.0f * 1.15f, tolerance);
            expectWithinAbsoluteError (bAtMax.wetLevelDb, -3.0f + juce::Decibels::gainToDecibels (1.15f), tolerance);

            const auto cAtMidpoint = CityMacro::map (50.0f, c);
            expectWithinAbsoluteError (cAtMidpoint.airGainDb, 3.9f * 1.25f, tolerance);
            expectWithinAbsoluteError (cAtMidpoint.detuneCents, 7.7f * 1.25f, tolerance);
            expectWithinAbsoluteError (cAtMidpoint.wetLevelDb, -9.0f + juce::Decibels::gainToDecibels (1.25f), tolerance);
        }

        beginTest ("Out-of-range input is clamped");
        {
            expectEquals (CityMacro::map (-5.0f).airGainDb, 0.0f);
            expectWithinAbsoluteError (CityMacro::map (150.0f).airGainDb, 7.8f, tolerance);
        }
    }
};

static CityMacroTests cityMacroTests;
