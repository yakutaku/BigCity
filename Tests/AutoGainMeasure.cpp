#include "AutoGain.h"
#include "LoudnessMeasurement.h"
#include "SoundSpec.h"

#include <juce_events/juce_events.h>

#include <cstdio>

// Builds the PRD §6 "Auto gain" curve. It measures with the current curve in place, so its
// suggestion is the current value corrected by what's still off; run it until nothing changes,
// then paste the line into SoundSpec::autoGainDb.
//
//   AutoGainMeasure [--voicing=A|B|C]
//
// --voicing measures a voicing candidate instead, and needs a BIGCITY_TUNING build.
int main (int argc, char* argv[])
{
    // The processor's parameter tree runs timers, so the message manager must exist.
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    const juce::ArgumentList args (argc, argv);
    const auto voicingName = args.getValueForOption ("--voicing");
    const auto voicingIndex = juce::StringArray { "", "A", "B", "C" }.indexOf (voicingName);
    const auto voicing = (Voicing) juce::jmax (0, voicingIndex);

    if (voicingIndex < 0 || (voicing != Voicing::current && ! BIGCITY_TUNING))
    {
        std::fprintf (stderr, "--voicing takes A, B or C, and needs a BIGCITY_TUNING build\n");
        return 1;
    }

    constexpr int stereo = 2;
    std::printf ("%10s %12s %12s %12s\n", "City size", "current dB", "vs bypass", "suggested");

    juce::String suggestedCurve;

    for (size_t point = 0; point < SoundSpec::autoGainDb.size(); ++point)
    {
        const auto citySize = (float) point * SoundSpec::autoGainStepPercent;
        const auto currentDb = AutoGain::gainDbFor (citySize, voicing);
        const auto differenceDb = LoudnessMeasurement::differenceFromBypassDb (citySize, stereo, voicing);

        // City Size 0 must stay exactly 0 dB so it nulls against bypass.
        const auto suggestedDb = point == 0 ? 0.0 : currentDb - differenceDb;
        std::printf ("%10.0f %+12.2f %+12.2f %+12.2f\n", citySize, currentDb, differenceDb, suggestedDb);
        suggestedCurve << (point == 0 ? "" : ", ") << juce::String (suggestedDb, 2) << "f";
    }

    std::printf ("\nautoGainDb%s { %s }\n", voicing == Voicing::current ? "" : ("Voicing" + voicingName).toRawUTF8(), suggestedCurve.toRawUTF8());
    return 0;
}
