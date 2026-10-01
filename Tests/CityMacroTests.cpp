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

        beginTest ("Out-of-range input is clamped");
        {
            expectEquals (CityMacro::map (-5.0f).airGainDb, 0.0f);
            expectWithinAbsoluteError (CityMacro::map (150.0f).airGainDb, 6.0f, tolerance);
        }
    }
};

static CityMacroTests cityMacroTests;
