#include "SevenSegment.h"
#include "UiSpec.h"

class SevenSegmentTests final : public juce::UnitTest
{
public:
    SevenSegmentTests() : juce::UnitTest ("SevenSegment", "BigCity") {}

    void runTest() override
    {
        using SevenSegment::blank;

        beginTest ("Digits: leading zeros are blank, the half digit lights only for 100.0");
        {
            expectDigits (5.0f, { false, blank, 5, 0 });
            expectDigits (0.0f, { false, blank, 0, 0 });
            expectDigits (50.0f, { false, 5, 0, 0 });
            expectDigits (99.9f, { false, 9, 9, 9 });
            expectDigits (100.0f, { true, 0, 0, 0 });
        }

        beginTest ("Digits: rounded to 0.1 and clamped to 0-100");
        {
            expectDigits (12.34f, { false, 1, 2, 3 });
            expectDigits (12.36f, { false, 1, 2, 4 });
            expectDigits (99.96f, { true, 0, 0, 0 });
            expectDigits (-3.0f, { false, blank, 0, 0 });
            expectDigits (140.0f, { true, 0, 0, 0 });
        }

        beginTest ("Geometry: a lit 8 fills its 28-tall cell at y 6, skewed right of the cell's x");
        {
            const auto bounds = SevenSegment::litSegmentsFor ({ false, blank, 8, blank }).getBounds();
            constexpr float tolerance = 1.0e-3f;

            expectWithinAbsoluteError (bounds.getY(), UiSpec::Readout::cellTopY, tolerance);
            expectWithinAbsoluteError (bounds.getBottom(), UiSpec::Readout::cellTopY + UiSpec::Readout::cellHeight, tolerance);

            // The leftmost point is e's lower-left corner; the skew moves it right by its height above the cell's bottom.
            using namespace UiSpec::Readout;
            const auto cornerY = segmentBottomY - segmentGap - segmentHalfThickness;
            const auto skew = std::tan (juce::degreesToRadians (skewDegrees));
            expectWithinAbsoluteError (bounds.getX(), onesCellX + (cellHeight - cornerY) * skew, tolerance);
        }
    }

private:
    void expectDigits (float citySizePercent, SevenSegment::Digits expected)
    {
        const auto actual = SevenSegment::digitsFor (citySizePercent);
        expect (actual == expected,
                "digits for " + juce::String (citySizePercent) + ": got "
                    + juce::String ((int) actual.halfDigitLit) + " " + juce::String (actual.tens) + " "
                    + juce::String (actual.ones) + " " + juce::String (actual.tenths));
    }
};

static SevenSegmentTests sevenSegmentTests;
