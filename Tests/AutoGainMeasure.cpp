#include "AutoGain.h"
#include "LoudnessMeasurement.h"
#include "SoundSpec.h"

#include <juce_events/juce_events.h>

#include <cstdio>

// Builds voicing 1's PRD §6 "Auto gain" curve. It measures with the current curve in place, so its
// suggestion is the current value corrected by what's still off; run it until nothing changes,
// then paste the line into SoundSpec::autoGainDb.
int main()
{
    // The processor's parameter tree runs timers, so the message manager must exist.
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    constexpr int stereo = 2;
    std::printf ("%10s %12s %12s %12s\n", "City size", "current dB", "vs bypass", "suggested");

    juce::String suggestedCurve;

    for (size_t point = 0; point < SoundSpec::autoGainDb.size(); ++point)
    {
        const auto citySize = (float) point * SoundSpec::autoGainStepPercent;
        const auto currentDb = AutoGain::gainDbFor (citySize, SoundSpec::autoGainDb);
        const auto differenceDb = LoudnessMeasurement::differenceFromBypassDb (citySize, stereo, Voicing::original);

        // City Size 0 must stay exactly 0 dB so it nulls against bypass.
        const auto suggestedDb = point == 0 ? 0.0 : currentDb - differenceDb;
        std::printf ("%10.0f %+12.2f %+12.2f %+12.2f\n", citySize, currentDb, differenceDb, suggestedDb);
        suggestedCurve << (point == 0 ? "" : ", ") << juce::String (suggestedDb, 2) << "f";
    }

    std::printf ("\nautoGainDb { %s }\n", suggestedCurve.toRawUTF8());
    return 0;
}
