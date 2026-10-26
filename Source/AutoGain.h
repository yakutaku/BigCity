#pragma once

#include "SoundSpec.h"

// Keeps every City Size position as loud as bypass, using a fixed measured curve: voicing 1's was
// measured with programme simulation noise, voicing 2's on music (PRD §6 "Auto gain", "Voicing 2 auto gain").
namespace AutoGain
{
    // citySize (0–100, clamped) to output gain in dB: linear in dB between the curve's points, one per
    // 10 of City Size, and exactly 0 dB at City Size 0.
    float gainDbFor (float citySizePercent, const SoundSpec::AutoGainCurve& curveDb);
}
