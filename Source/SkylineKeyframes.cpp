#include "SkylineKeyframes.h"

#include <BinaryData.h>

#include <tuple>

namespace
{
    struct EmbeddedImage
    {
        const char* data;
        int size;
    };

    SkylineKeyframes::Frames decodeKeyframes()
    {
        const auto sources = std::to_array<EmbeddedImage> ({
            { BinaryData::skyline_tod_000_png, BinaryData::skyline_tod_000_pngSize },
            { BinaryData::skyline_tod_025_png, BinaryData::skyline_tod_025_pngSize },
            { BinaryData::skyline_tod_050_png, BinaryData::skyline_tod_050_pngSize },
            { BinaryData::skyline_tod_075_png, BinaryData::skyline_tod_075_pngSize },
            { BinaryData::skyline_tod_100_png, BinaryData::skyline_tod_100_pngSize },
        });

        static_assert (std::tuple_size_v<decltype (sources)> == UiSpec::Skyline::keyframeCount);

        // Not juce::ImageCache: the cache would keep its own reference, and these must go when the last editor closes.
        SkylineKeyframes::Frames frames;

        for (size_t i = 0; i < frames.size(); ++i)
            frames[i] = juce::ImageFileFormat::loadFrom (sources[i].data, (size_t) sources[i].size);

        return frames;
    }

    // The grey Figma's saturation blend leaves at full strength: the colour's luminosity with no hue
    // (Lum() in the W3C compositing spec, which Figma follows).
    juce::Colour withoutSaturation (juce::Colour colour)
    {
        constexpr float redWeight = 0.3f, greenWeight = 0.59f, blueWeight = 0.11f;
        const auto luminosity = redWeight * colour.getFloatRed() + greenWeight * colour.getFloatGreen()
                              + blueWeight * colour.getFloatBlue();
        return juce::Colour::fromFloatRGBA (luminosity, luminosity, luminosity, colour.getFloatAlpha());
    }

    juce::Image greyscaleCopyOf (const juce::Image& image)
    {
        auto grey = image.createCopy();
        juce::Image::BitmapData pixels (grey, juce::Image::BitmapData::readWrite);

        for (int y = 0; y < pixels.height; ++y)
            for (int x = 0; x < pixels.width; ++x)
                pixels.setPixelColour (x, y, withoutSaturation (pixels.getPixelColour (x, y)));

        return grey;
    }

    SkylineKeyframes::Frames greyscaleCopiesOf (const SkylineKeyframes::Frames& frames)
    {
        SkylineKeyframes::Frames greys;

        for (size_t i = 0; i < frames.size(); ++i)
            greys[i] = greyscaleCopyOf (frames[i]);

        return greys;
    }
}

SkylineKeyframes::SkylineKeyframes()
    : colour (decodeKeyframes()),
      grey (greyscaleCopiesOf (colour))
{
}
