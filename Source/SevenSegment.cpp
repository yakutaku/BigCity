#include "SevenSegment.h"
#include "SoundSpec.h"
#include "UiSpec.h"

#include <array>

namespace
{
    using namespace UiSpec::Readout;

    enum Segment { a, b, c, d, e, f, g, segmentCount };

    using SegmentSet = std::array<bool, segmentCount>;

    // docs/ui-spec.md "Segment map"
    SegmentSet segmentsFor (int digit)
    {
        static constexpr std::array<SegmentSet, 10> segmentMap {{
            //  a      b      c      d      e      f      g
            { true,  true,  true,  true,  true,  true,  false },  // 0
            { false, true,  true,  false, false, false, false },  // 1
            { true,  true,  false, true,  true,  false, true  },  // 2
            { true,  true,  true,  true,  false, false, true  },  // 3
            { false, true,  true,  false, false, true,  true  },  // 4
            { true,  false, true,  true,  false, true,  true  },  // 5
            { true,  false, true,  true,  true,  true,  true  },  // 6
            { true,  true,  true,  false, false, false, false },  // 7
            { true,  true,  true,  true,  true,  true,  true  },  // 8
            { true,  true,  true,  true,  false, true,  true  },  // 9
        }};

        return segmentMap[(size_t) digit];
    }

    using Hexagon = std::array<juce::Point<float>, 6>;

    Hexagon horizontalSegment (float y)
    {
        constexpr auto ht = segmentHalfThickness, gap = segmentGap, left = segmentLeftX, right = segmentRightX;
        return {{ { left + gap, y }, { left + gap + ht, y - ht }, { right - gap - ht, y - ht },
                  { right - gap, y }, { right - gap - ht, y + ht }, { left + gap + ht, y + ht } }};
    }

    Hexagon verticalSegment (float x, float topY, float bottomY)
    {
        constexpr auto ht = segmentHalfThickness, gap = segmentGap;
        return {{ { x, topY + gap }, { x + ht, topY + gap + ht }, { x + ht, bottomY - gap - ht },
                  { x, bottomY - gap }, { x - ht, bottomY - gap - ht }, { x - ht, topY + gap + ht } }};
    }

    // Cell-local outline, as in the Figma script.
    Hexagon outlineOf (Segment segment)
    {
        switch (segment)
        {
            case a: return horizontalSegment (segmentTopY);
            case g: return horizontalSegment (segmentMiddleY);
            case d: return horizontalSegment (segmentBottomY);
            case b: return verticalSegment (segmentRightX, segmentTopY, segmentMiddleY);
            case c: return verticalSegment (segmentRightX, segmentMiddleY, segmentBottomY);
            case f: return verticalSegment (segmentLeftX, segmentTopY, segmentMiddleY);
            case e: return verticalSegment (segmentLeftX, segmentMiddleY, segmentBottomY);
            case segmentCount: break;
        }

        jassertfalse;
        return {};
    }

    // Skews a cell-local point into the LCD, pivoting on the cell's bottom edge.
    juce::Point<float> toLcd (juce::Point<float> cellPoint, float cellX)
    {
        static const auto skew = std::tan (juce::degreesToRadians (skewDegrees));
        return { cellX + cellPoint.x + (cellHeight - cellPoint.y) * skew, cellTopY + cellPoint.y };
    }

    void addSegment (juce::Path& path, Segment segment, float cellX)
    {
        const auto outline = outlineOf (segment);
        path.startNewSubPath (toLcd (outline.front(), cellX));

        for (size_t i = 1; i < outline.size(); ++i)
            path.lineTo (toLcd (outline[i], cellX));

        path.closeSubPath();
    }

    void addDigit (juce::Path& path, int digit, float cellX)
    {
        if (digit == SevenSegment::blank)
            return;

        const auto lit = segmentsFor (digit);

        for (int segment = a; segment < segmentCount; ++segment)
            if (lit[(size_t) segment])
                addSegment (path, (Segment) segment, cellX);
    }
}

SevenSegment::Digits SevenSegment::digitsFor (float citySizePercent)
{
    const auto clamped = juce::jlimit (SoundSpec::citySizeMinPercent, SoundSpec::citySizeMaxPercent, citySizePercent);
    constexpr int tenthsPerOne = 10;
    constexpr int tenthsPerTen = 100;
    constexpr int tenthsPerHundred = 1000;
    constexpr int base = 10;

    const auto totalTenths = juce::roundToInt (clamped * (float) tenthsPerOne);

    return { totalTenths >= tenthsPerHundred,
             totalTenths >= tenthsPerTen ? totalTenths / tenthsPerTen % base : blank,
             totalTenths / tenthsPerOne % base,
             totalTenths % tenthsPerOne };
}

juce::Path SevenSegment::litSegmentsFor (const Digits& digits)
{
    juce::Path path;

    if (digits.halfDigitLit)
        addDigit (path, 1, halfDigitCellX);

    addDigit (path, digits.tens, tensCellX);
    addDigit (path, digits.ones, onesCellX);
    addDigit (path, digits.tenths, tenthsCellX);
    path.addRectangle (decimalPoint);

    return path;
}
