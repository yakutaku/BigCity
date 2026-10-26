#pragma once

#include "SoundTuning.h"
#include "Voicing.h"

// What one City Size position means for each sound element.
struct CitySettings
{
    float airGainDb;
    double airCornerHz;
    double airShelfQ;
    float detuneCents;  // magnitude; the left voice goes down by this much and the right voice up
    float wetLevelDb;   // minus infinity means off
};

namespace CityMacro
{
    // Maps citySize (0–100, clamped) through the PRD §6 knob response to the curves, each scaled
    // to the tuning's maximum. Voicing 2 plays those curves at twice the position up to 50, then
    // carries each element on to its voicing 2 value at 100.
    CitySettings map (float citySizePercent, Voicing voicing, const SoundTuning& tuning = {});
}
