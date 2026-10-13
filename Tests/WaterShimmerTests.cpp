#include "WaterShimmer.h"
#include "UiSpec.h"

#include <set>

class WaterShimmerTests final : public juce::UnitTest
{
public:
    WaterShimmerTests() : juce::UnitTest ("WaterShimmer", "BigCity") {}

    void runTest() override
    {
        beginTest ("At City size 0 the water is still and nothing is drawn");
        {
            for (auto seconds : sampleTimes())
                expect (WaterShimmer::frameAt (seconds, 0.0f) == WaterShimmer::Frame {}, "a frame at 0 draws something");

            expect (WaterShimmer::stillFrame (0.0f) == WaterShimmer::Frame {}, "the Reduce motion frame at 0 draws something");
        }

        beginTest ("Glints drift by at most 1 grid pixel, moving both ways at any City size above 0");
        {
            for (auto citySize : { 0.1f, 25.0f, 50.0f, 100.0f })
            {
                std::set<int> offsetsSeen;

                for (auto seconds : sampleTimes())
                    for (auto offset : WaterShimmer::frameAt (seconds, citySize).glintOffsets)
                    {
                        expect (std::abs (offset) <= UiSpec::Skyline::glintDriftGridPixels, "offset " + juce::String (offset));
                        offsetsSeen.insert (offset);
                    }

                expect (offsetsSeen == std::set<int> { -1, 0, 1 }, "at " + juce::String (citySize) + " the glints don't drift both ways");
            }
        }

        beginTest ("Glint rows drift out of phase");
        {
            auto rowsDiffered = false;

            for (auto seconds : sampleTimes())
            {
                const auto offsets = WaterShimmer::frameAt (seconds, 100.0f).glintOffsets;
                rowsDiffered |= std::adjacent_find (offsets.begin(), offsets.end(), std::not_equal_to<>()) != offsets.end();
            }

            expect (rowsDiffered);
        }

        beginTest ("The highlight's alpha rises with City size to 60% at 100");
        {
            auto previousAlpha = 0.0f;

            for (int tenths = 1; tenths <= 1000; ++tenths)
            {
                const auto alpha = WaterShimmer::frameAt (0.0, (float) tenths / 10.0f).glintAlpha;
                expect (alpha > previousAlpha, "alpha doesn't rise at " + juce::String ((float) tenths / 10.0f));
                previousAlpha = alpha;
            }

            expectWithinAbsoluteError (WaterShimmer::frameAt (0.0, 100.0f).glintAlpha, 0.6f, 1.0e-6f);
        }

        beginTest ("Sparkle count is round (8 × City size / 100)");
        {
            expectEquals (WaterShimmer::frameAt (0.0, 6.0f).sparkleCount, 0);
            expectEquals (WaterShimmer::frameAt (0.0, 7.0f).sparkleCount, 1);
            expectEquals (WaterShimmer::frameAt (0.0, 50.0f).sparkleCount, 4);
            expectEquals (WaterShimmer::frameAt (0.0, 100.0f).sparkleCount, 8);
        }

        beginTest ("Sparkles stay on the water, never brighter than 60%");
        {
            const auto water = WaterShimmer::waterCells();
            expectEquals (water.getY(), 59);
            expectEquals (water.getBottom(), 81);
            expectEquals (water.getWidth(), UiSpec::Skyline::gridWidth);

            for (auto seconds : sampleTimes())
            {
                const auto frame = WaterShimmer::frameAt (seconds, 100.0f);

                for (int i = 0; i < frame.sparkleCount; ++i)
                {
                    const auto& sparkle = frame.sparkles[(size_t) i];
                    expect (water.contains (sparkle.cell), "sparkle off the water at " + sparkle.cell.toString());
                    expect (sparkle.alpha >= 0.0f && sparkle.alpha <= 0.6f, "sparkle alpha " + juce::String (sparkle.alpha));
                }
            }
        }

        beginTest ("Each sparkle moves to a new spot every cycle");
        {
            std::set<std::pair<int, int>> spots;
            constexpr int cycles = 20;

            for (int cycle = 0; cycle < cycles; ++cycle)
            {
                const auto cell = WaterShimmer::frameAt (cycle * UiSpec::Skyline::sparkleCycleSeconds, 100.0f).sparkles[0].cell;
                spots.insert ({ cell.x, cell.y });
            }

            expectGreaterThan ((int) spots.size(), cycles - 2);
        }

        beginTest ("Reduce motion: the frame at time 0 with no sparkles");
        {
            const auto still = WaterShimmer::stillFrame (75.0f);
            expectEquals (still.sparkleCount, 0);
            expect (still.glintOffsets == WaterShimmer::frameAt (0.0, 75.0f).glintOffsets);
            expectEquals (still.glintAlpha, WaterShimmer::frameAt (0.0, 75.0f).glintAlpha);
        }

        beginTest ("Glint highlights sit exactly on the keyframes' glints at rest");
        {
            const auto top = WaterShimmer::glintCells (0, 0);
            expectEquals (top.getX(), 41);
            expectEquals (top.getRight(), 73);
            expectEquals (top.getY(), 61);

            const auto bottom = WaterShimmer::glintCells (4, 1);
            expectEquals (bottom.getX(), 56);
            expectEquals (bottom.getRight(), 60);
            expectEquals (bottom.getY(), 73);
        }
    }

private:
    // Ten seconds at the 8 fps the water is drawn at, starting somewhere a real clock might be.
    static std::vector<double> sampleTimes()
    {
        std::vector<double> times;

        for (int frame = 0; frame < 80; ++frame)
            times.push_back (12345.0 + frame / (double) UiSpec::Skyline::shimmerRateHz);

        return times;
    }
};

static WaterShimmerTests waterShimmerTests;
