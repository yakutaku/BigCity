#include "Skyline.h"
#include "UiSpec.h"
#include "WaterShimmer.h"

class SkylineTests final : public juce::UnitTest
{
public:
    SkylineTests() : juce::UnitTest ("Skyline", "BigCity") {}

    void runTest() override
    {
        beginTest ("A shimmer frame drawn from the cached water strip matches a full repaint pixel for pixel");

        const auto water = (WaterShimmer::waterCells().toFloat() * UiSpec::Skyline::pointsPerGridPixel).toNearestInt();

        // 25 shows one keyframe alone; 62.5 blends two. 1x and 2x cover both kinds of display.
        for (auto citySize : { 25.0f, 62.5f, 100.0f })
            for (auto scale : { 1.0f, 2.0f })
            {
                // Not on screen, so the skyline holds the still frame: its glint highlights are drawn.
                Skyline skyline (citySize, false);
                skyline.setState (citySize, false);

                const auto full = skyline.createComponentSnapshot (skyline.getLocalBounds(), true, scale)
                                      .getClippedImage ((water.toFloat() * scale).toNearestInt());
                const auto waterOnly = skyline.createComponentSnapshot (water, true, scale);

                expectEquals (differingPixels (full, waterOnly), 0,
                              "City size " + juce::String (citySize) + " at " + juce::String (scale) + "x");
            }
    }

private:
    static int differingPixels (const juce::Image& a, const juce::Image& b)
    {
        if (a.getBounds() != b.getBounds())
            return -1;

        int count = 0;

        for (int y = 0; y < a.getHeight(); ++y)
            for (int x = 0; x < a.getWidth(); ++x)
                if (a.getPixelAt (x, y) != b.getPixelAt (x, y))
                    ++count;

        return count;
    }
};

static SkylineTests skylineTests;
