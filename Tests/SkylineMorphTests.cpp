#include "SkylineMorph.h"
#include "UiSpec.h"

#include <BinaryData.h>

class SkylineMorphTests final : public juce::UnitTest
{
public:
    SkylineMorphTests() : juce::UnitTest ("SkylineMorph", "BigCity") {}

    void runTest() override
    {
        constexpr int windowCount = 353;

        beginTest ("Lit windows match the spec's reference counts: 0, 52, 105, 158, 211");
        {
            expectEquals (SkylineMorph::litWindowCount (0.0f, windowCount), 0);
            expectEquals (SkylineMorph::litWindowCount (25.0f, windowCount), 52);
            expectEquals (SkylineMorph::litWindowCount (50.0f, windowCount), 105);
            expectEquals (SkylineMorph::litWindowCount (75.0f, windowCount), 158);
            expectEquals (SkylineMorph::litWindowCount (100.0f, windowCount), 211);
        }

        beginTest ("Keyframes: each one shows alone at 0, 25, 50, 75 and 100");
        {
            expectBlend (0.0f, 0, 0.0f);
            expectBlend (25.0f, 1, 0.0f);
            expectBlend (50.0f, 2, 0.0f);
            expectBlend (75.0f, 3, 0.0f);
            expectBlend (100.0f, 3, 1.0f);
        }

        beginTest ("Keyframes: linear between neighbours");
        {
            expectBlend (12.5f, 0, 0.5f);
            expectBlend (30.0f, 1, 0.2f);
            expectBlend (99.9f, 3, 0.996f);
        }

        beginTest ("Out-of-range City Size is clamped");
        {
            expectEquals (SkylineMorph::litWindowCount (-5.0f, windowCount), 0);
            expectEquals (SkylineMorph::litWindowCount (150.0f, windowCount), 211);
            expectBlend (-5.0f, 0, 0.0f);
            expectBlend (150.0f, 3, 1.0f);
        }

        beginTest ("The shipped window order: 353 windows on the grid, starting at (109, 53)");
        {
            const auto windows = SkylineMorph::parseWindows (juce::String::fromUTF8 (BinaryData::skylinewindows_json,
                                                                                     BinaryData::skylinewindows_jsonSize));
            expectEquals ((int) windows.size(), windowCount);
            expect (windows.front() == juce::Point<int> (109, 53));
        }

        beginTest ("A bad window file gives no windows rather than a crash or a half-lit city");
        {
            expect (SkylineMorph::parseWindows ("not json").empty(), "not JSON");
            expect (SkylineMorph::parseWindows ("{}").empty(), "no windows array");
            expect (SkylineMorph::parseWindows (R"({"windows":[[1,2],[3]]})").empty(), "a window without a y");
            expect (SkylineMorph::parseWindows (R"({"windows":[[1,2],["a",4]]})").empty(), "a non-number");
            expect (SkylineMorph::parseWindows (R"({"windows":[[1,2],[112,4]]})").empty(), "x off the 112-wide grid");
            expect (SkylineMorph::parseWindows (R"({"windows":[[1,2],[3,-1]]})").empty(), "y off the grid");
            expectEquals ((int) SkylineMorph::parseWindows (R"({"windows":[[0,0],[111,94]]})").size(), 2);
        }
    }

private:
    void expectBlend (float citySizePercent, int lowerIndex, float upperAlpha)
    {
        const auto blend = SkylineMorph::keyframeBlendFor (citySizePercent);
        const auto at = " at " + juce::String (citySizePercent);
        expectEquals (blend.lowerIndex, lowerIndex, "lower keyframe" + at);
        expectWithinAbsoluteError (blend.upperAlpha, upperAlpha, 1.0e-4f, "upper alpha" + at);
    }
};

static SkylineMorphTests skylineMorphTests;
