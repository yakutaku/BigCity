#include "SkylineBypassFade.h"
#include "UiSpec.h"

class SkylineBypassFadeTests final : public juce::UnitTest
{
public:
    SkylineBypassFadeTests() : juce::UnitTest ("SkylineBypassFade", "BigCity") {}

    void runTest() override
    {
        constexpr double fadeSeconds = UiSpec::Skyline::bypassFadeMs / 1000.0;
        constexpr float tolerance = 1.0e-4f;

        beginTest ("Opens in its state with no fade");
        {
            SkylineBypassFade normal { false };
            SkylineBypassFade bypassed { true };
            expectEquals (normal.advance (false, 10.0), 0.0f);
            expectEquals (bypassed.advance (true, 10.0), 1.0f);
        }

        beginTest ("Bypass fades in over 200 ms, eased in and out, then holds");
        {
            SkylineBypassFade fade { false };
            const auto start = 10.0;

            expectEquals (fade.advance (true, start), 0.0f);
            expectWithinAbsoluteError (fade.advance (true, start + fadeSeconds * 0.25), 0.0625f, tolerance);  // 4 × 0.25³
            expectWithinAbsoluteError (fade.advance (true, start + fadeSeconds * 0.5), 0.5f, tolerance);
            expectWithinAbsoluteError (fade.advance (true, start + fadeSeconds * 0.75), 0.9375f, tolerance);
            expectEquals (fade.advance (true, start + fadeSeconds), 1.0f);
            expectEquals (fade.advance (true, start + 5.0), 1.0f);
        }

        beginTest ("Un-bypassing fades back out the same way");
        {
            SkylineBypassFade fade { true };
            fade.advance (false, 0.0);
            expectWithinAbsoluteError (fade.advance (false, fadeSeconds * 0.5), 0.5f, tolerance);
            expectEquals (fade.advance (false, fadeSeconds), 0.0f);
        }

        beginTest ("A long idle before a toggle doesn't skip the fade");
        {
            SkylineBypassFade fade { false };
            fade.advance (false, 0.0);
            expectEquals (fade.advance (true, 60.0), 0.0f);
            expectWithinAbsoluteError (fade.advance (true, 60.0 + fadeSeconds * 0.5), 0.5f, tolerance);
        }

        beginTest ("Toggling back mid-fade turns around from where it is, without a jump");
        {
            SkylineBypassFade fade { false };
            fade.advance (true, 0.0);
            const auto atTurn = fade.advance (true, fadeSeconds * 0.5);

            expectWithinAbsoluteError (fade.advance (false, fadeSeconds * 0.5), atTurn, tolerance);
            expectWithinAbsoluteError (fade.advance (false, fadeSeconds * 0.75), 0.0625f, tolerance);  // back at a quarter
            expectEquals (fade.advance (false, fadeSeconds), 0.0f, "half the way back takes half the time");
        }
    }
};

static SkylineBypassFadeTests skylineBypassFadeTests;
