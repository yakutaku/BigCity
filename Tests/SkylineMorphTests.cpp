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

        beginTest ("Grey layer: 0.3 at 0 (70% saturated), 0.15 at 50, none at 100");
        {
            expectWithinAbsoluteError (SkylineMorph::greyLayerAlpha (0.0f), 0.3f, 1.0e-6f);
            expectWithinAbsoluteError (SkylineMorph::greyLayerAlpha (50.0f), 0.15f, 1.0e-6f);
            expectEquals (SkylineMorph::greyLayerAlpha (100.0f), 0.0f);
        }

        beginTest ("Out-of-range City Size is clamped");
        {
            expectEquals (SkylineMorph::litWindowCount (-5.0f, windowCount), 0);
            expectEquals (SkylineMorph::litWindowCount (150.0f, windowCount), 211);
            expectEquals (SkylineMorph::greyLayerAlpha (-5.0f), UiSpec::Skyline::greyAlphaAtZero);
            expectEquals (SkylineMorph::greyLayerAlpha (150.0f), 0.0f);
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
};

static SkylineMorphTests skylineMorphTests;
