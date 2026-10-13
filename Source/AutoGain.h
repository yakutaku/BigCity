#pragma once

#include "Voicing.h"

// Keeps every City Size position as loud as bypass, using the fixed curve measured with programme
// simulation noise (PRD §6 "Auto gain").
namespace AutoGain
{
    // citySize (0–100, clamped) to output gain in dB: linear in dB between the measured points,
    // and exactly 0 dB at City Size 0. Each voicing has its own measured curve.
    float gainDbFor (float citySizePercent, Voicing voicing = Voicing::current);
}
