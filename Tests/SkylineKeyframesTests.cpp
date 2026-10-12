#include "SkylineKeyframes.h"

class SkylineKeyframesTests final : public juce::UnitTest
{
public:
    SkylineKeyframesTests() : juce::UnitTest ("SkylineKeyframes", "BigCity") {}

    void runTest() override
    {
        beginTest ("Every keyframe and its grey copy decodes at @2x screen size, opaque");
        {
            const juce::SharedResourcePointer<SkylineKeyframes> keyframes;
            const auto screen = UiSpec::Skyline::screenBounds * 2;

            for (size_t i = 0; i < keyframes->colour.size(); ++i)
            {
                for (const auto* image : { &keyframes->colour[i], &keyframes->grey[i] })
                {
                    expect (image->getBounds() == screen.withZeroOrigin(), "keyframe " + juce::String (i) + " size");
                    expect (isOpaque (*image), "keyframe " + juce::String (i) + " opaque");
                }

                expect (isGrey (keyframes->grey[i]), "keyframe " + juce::String (i) + " grey copy has no hue");
            }

            logMessage ("Shared keyframe memory: " + juce::String (bytesIn (*keyframes)) + " bytes");
        }

        beginTest ("Open editors share one copy; it's freed when the last one closes");
        {
            using Shared = juce::SharedResourcePointer<SkylineKeyframes>;
            expect (! Shared::getSharedObjectWithoutCreating().has_value(), "nothing loaded before an editor opens");

            {
                const Shared first;
                const Shared second;
                expect (&first.get() == &second.get());
            }

            expect (! Shared::getSharedObjectWithoutCreating().has_value(), "freed once the last editor closes");
        }
    }

private:
    template <typename PixelTest>
    static bool everyPixel (const juce::Image& image, PixelTest&& test)
    {
        const juce::Image::BitmapData pixels (image, juce::Image::BitmapData::readOnly);

        for (int y = 0; y < pixels.height; ++y)
            for (int x = 0; x < pixels.width; ++x)
                if (! test (pixels.getPixelColour (x, y)))
                    return false;

        return true;
    }

    static bool isOpaque (const juce::Image& image)
    {
        return everyPixel (image, [] (juce::Colour c) { return c.isOpaque(); });
    }

    static bool isGrey (const juce::Image& image)
    {
        return everyPixel (image, [] (juce::Colour c) { return c.getRed() == c.getGreen() && c.getGreen() == c.getBlue(); });
    }

    static juce::int64 bytesIn (const SkylineKeyframes& keyframes)
    {
        juce::int64 bytes = 0;

        for (const auto& frames : { &keyframes.colour, &keyframes.grey })
            for (const auto& image : *frames)
            {
                const juce::Image::BitmapData pixels (image, juce::Image::BitmapData::readOnly);
                bytes += (juce::int64) pixels.lineStride * pixels.height;
            }

        return bytes;
    }
};

static SkylineKeyframesTests skylineKeyframesTests;
