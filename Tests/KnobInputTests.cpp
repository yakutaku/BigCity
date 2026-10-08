#include "KnobInput.h"

class KnobInputTests final : public juce::UnitTest
{
public:
    KnobInputTests() : juce::UnitTest ("KnobInput", "BigCity") {}

    void runTest() override
    {
        constexpr float tolerance = 1.0e-4f;

        beginTest ("Angle: -135 at 0, 0 at 50, +135 at 100");
        {
            expectWithinAbsoluteError (KnobInput::angleRadiansFor (0.0f), juce::degreesToRadians (-135.0f), tolerance);
            expectWithinAbsoluteError (KnobInput::angleRadiansFor (50.0f), 0.0f, tolerance);
            expectWithinAbsoluteError (KnobInput::angleRadiansFor (100.0f), juce::degreesToRadians (135.0f), tolerance);
        }

        beginTest ("Drag: 200 px covers the full range; up increases");
        {
            expectWithinAbsoluteError (KnobInput::valueAfterDrag (0.0f, 200.0f, false), 100.0f, tolerance);
            expectWithinAbsoluteError (KnobInput::valueAfterDrag (50.0f, -20.0f, false), 40.0f, tolerance);
        }

        beginTest ("Shift drag: 0.1 per pixel");
        {
            expectWithinAbsoluteError (KnobInput::valueAfterDrag (50.0f, 1.0f, true), 50.1f, tolerance);
            expectWithinAbsoluteError (KnobInput::valueAfterDrag (50.0f, -30.0f, true), 47.0f, tolerance);
        }

        beginTest ("Drag clamps to 0-100");
        {
            expectEquals (KnobInput::valueAfterDrag (90.0f, 500.0f, false), 100.0f);
            expectEquals (KnobInput::valueAfterDrag (10.0f, -500.0f, false), 0.0f);
        }

        beginTest ("Mouse wheel: 1.0 per notch, 0.1 with Shift, whatever the notch size");
        {
            expectWithinAbsoluteError (KnobInput::valueAfterWheel (50.0f, mouseWheel (0.04f), false), 51.0f, tolerance);
            expectWithinAbsoluteError (KnobInput::valueAfterWheel (50.0f, mouseWheel (-0.4f), false), 49.0f, tolerance);
            expectWithinAbsoluteError (KnobInput::valueAfterWheel (50.0f, mouseWheel (0.04f), true), 50.1f, tolerance);
            expectEquals (KnobInput::valueAfterWheel (100.0f, mouseWheel (0.04f), false), 100.0f);
            expectEquals (KnobInput::valueAfterWheel (50.0f, mouseWheel (0.0f), false), 50.0f);
        }

        beginTest ("Mouse wheel with Shift scrolls horizontally on macOS and still moves the knob");
        {
            auto shifted = mouseWheel (0.0f);
            shifted.deltaX = -0.04f;
            expectWithinAbsoluteError (KnobInput::valueAfterWheel (50.0f, shifted, true), 50.1f, tolerance);
        }

        beginTest ("Trackpad: a swipe moves as far as a drag of the same distance");
        {
            const auto swipe100Pixels = trackpad (100.0f * 0.5f / 256.0f);
            expectWithinAbsoluteError (KnobInput::valueAfterWheel (0.0f, swipe100Pixels, false),
                                       KnobInput::valueAfterDrag (0.0f, 100.0f, false), tolerance);
        }
    }

private:
    static juce::MouseWheelDetails mouseWheel (float deltaY)
    {
        juce::MouseWheelDetails wheel {};
        wheel.deltaY = deltaY;
        return wheel;
    }

    static juce::MouseWheelDetails trackpad (float deltaY)
    {
        auto wheel = mouseWheel (deltaY);
        wheel.isSmooth = true;
        return wheel;
    }
};

static KnobInputTests knobInputTests;
