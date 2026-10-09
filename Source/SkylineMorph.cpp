#include "SkylineMorph.h"
#include "SoundSpec.h"
#include "UiSpec.h"

namespace
{
    double fractionOfRange (float citySizePercent)
    {
        const auto clamped = juce::jlimit (SoundSpec::citySizeMinPercent, SoundSpec::citySizeMaxPercent, citySizePercent);
        return (double) (clamped - SoundSpec::citySizeMinPercent)
             / (double) (SoundSpec::citySizeMaxPercent - SoundSpec::citySizeMinPercent);
    }

    bool isOnGrid (juce::Point<int> window)
    {
        return juce::isPositiveAndBelow (window.x, UiSpec::Skyline::gridWidth)
            && juce::isPositiveAndBelow (window.y, UiSpec::Skyline::gridHeight);
    }
}

int SkylineMorph::litWindowCount (float citySizePercent, int windowCount)
{
    return (int) std::floor (windowCount * UiSpec::Skyline::maxLitFraction * fractionOfRange (citySizePercent));
}

float SkylineMorph::greyLayerAlpha (float citySizePercent)
{
    return UiSpec::Skyline::greyAlphaAtZero * (float) (1.0 - fractionOfRange (citySizePercent));
}

std::vector<juce::Point<int>> SkylineMorph::parseWindows (const juce::String& json)
{
    // Kept alive while its array is read: the array belongs to it.
    const auto parsed = juce::JSON::parse (json);
    const auto* entries = parsed["windows"].getArray();

    if (entries == nullptr)
        return {};

    std::vector<juce::Point<int>> windows;
    windows.reserve ((size_t) entries->size());

    for (const auto& entry : *entries)
    {
        const auto* coordinates = entry.getArray();

        if (coordinates == nullptr || coordinates->size() != 2 || ! (*coordinates)[0].isInt() || ! (*coordinates)[1].isInt())
            return {};

        const juce::Point<int> window { (*coordinates)[0], (*coordinates)[1] };

        if (! isOnGrid (window))
            return {};

        windows.push_back (window);
    }

    return windows;
}
