#include "WaterShimmer.h"

#include <cmath>

namespace WaterShimmer
{
    namespace
    {
        using namespace UiSpec::Skyline;

        // Spreads the bits of a small integer, so neighbouring sparkles and cycles land far apart. Deterministic,
        // so the water needs no random state and a frame depends only on its inputs.
        juce::uint32 hash (juce::uint32 value)
        {
            value ^= value >> 16;
            value *= 0x7feb352dU;
            value ^= value >> 15;
            value *= 0x846ca68bU;
            value ^= value >> 16;
            return value;
        }

        double unitFromHash (juce::uint32 value)
        {
            return (double) hash (value) / 4294967296.0;
        }

        int glintOffset (double seconds, size_t row)
        {
            // Spread evenly around the cycle, so the rows never drift together.
            const auto rowPhase = juce::MathConstants<double>::twoPi * (double) row / (double) glintCount;
            const auto drift = std::sin (juce::MathConstants<double>::twoPi * seconds / glintDriftPeriodSeconds + rowPhase);
            return (int) std::lround (glintDriftGridPixels * drift);
        }

        Sparkle sparkleAt (double seconds, int index)
        {
            // Each sparkle starts its cycle at its own point, so they don't twinkle in step.
            const auto cyclePosition = seconds / sparkleCycleSeconds + unitFromHash ((juce::uint32) index);
            const auto cycle = (juce::uint32) std::floor (cyclePosition);
            const auto withinCycle = (float) (cyclePosition - std::floor (cyclePosition));

            // A new spot each cycle, anywhere on the water.
            const auto water = waterCells();
            const auto spot = hash (cycle * (juce::uint32) maxSparkles + (juce::uint32) index + 1U);
            const juce::Point<int> cell { water.getX() + (int) (spot % (juce::uint32) water.getWidth()),
                                          water.getY() + (int) ((spot / (juce::uint32) water.getWidth()) % (juce::uint32) water.getHeight()) };

            // Up to the peak at mid-cycle and back down to nothing.
            return { cell, sparklePeakAlpha * (1.0f - std::abs (2.0f * withinCycle - 1.0f)) };
        }

        float citySizeShare (float citySizePercent)
        {
            return juce::jlimit (0.0f, 1.0f, citySizePercent / 100.0f);
        }
    }

    Frame frameAt (double seconds, float citySizePercent)
    {
        Frame frame;

        if (citySizeShare (citySizePercent) <= 0.0f)
            return frame;

        frame.glintAlpha = glintHighlightMaxAlpha * citySizeShare (citySizePercent);

        for (size_t row = 0; row < glintCount; ++row)
            frame.glintOffsets[row] = glintOffset (seconds, row);

        frame.sparkleCount = (int) std::lround ((float) maxSparkles * citySizeShare (citySizePercent));

        for (int index = 0; index < frame.sparkleCount; ++index)
            frame.sparkles[(size_t) index] = sparkleAt (seconds, index);

        return frame;
    }

    Frame stillFrame (float citySizePercent)
    {
        auto frame = frameAt (0.0, citySizePercent);
        frame.sparkles = {};
        frame.sparkleCount = 0;
        return frame;
    }

    juce::Rectangle<int> glintCells (size_t index, int offset)
    {
        const auto halfWidth = glintHalfWidths[index];
        return { glintCentreX - halfWidth + offset, glintRows[index], 2 * halfWidth, 1 };
    }

    juce::Rectangle<int> waterCells()
    {
        return juce::Rectangle<int>::leftTopRightBottom (0, waterTopRow, gridWidth, waterBottomRow + 1);
    }
}
