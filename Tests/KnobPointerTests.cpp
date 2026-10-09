#include "KnobPointer.h"
#include "UiSpec.h"

class KnobPointerTests final : public juce::UnitTest
{
public:
    KnobPointerTests() : juce::UnitTest ("KnobPointer", "BigCity") {}

    void runTest() override
    {
        const auto centre = UiSpec::Knob::centre;

        beginTest ("At 0 degrees: 4 x 20 at 12 o'clock, 30 to 50 above the centre");
        expectBounds (KnobPointer::outlineAt (0.0f).getBounds(),
                      { centre.x - 2.0f, centre.y - 50.0f, 4.0f, 20.0f });

        beginTest ("At 90 degrees: 20 x 4 at 3 o'clock, 30 to 50 right of the centre");
        expectBounds (KnobPointer::outlineAt (juce::MathConstants<float>::halfPi).getBounds(),
                      { centre.x + 30.0f, centre.y - 2.0f, 20.0f, 4.0f });
    }

private:
    void expectBounds (juce::Rectangle<float> actual, juce::Rectangle<float> expected)
    {
        constexpr float tolerance = 1.0e-3f;
        expectWithinAbsoluteError (actual.getX(), expected.getX(), tolerance);
        expectWithinAbsoluteError (actual.getY(), expected.getY(), tolerance);
        expectWithinAbsoluteError (actual.getWidth(), expected.getWidth(), tolerance);
        expectWithinAbsoluteError (actual.getHeight(), expected.getHeight(), tolerance);
    }
};

static KnobPointerTests knobPointerTests;
