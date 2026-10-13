#pragma once

#include "SoundTuning.h"

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
    // Maps citySize (0–100, clamped) to the PRD §6 curves, each scaled to the tuning's maximum,
    // then shaped by the tuning's voicing.
    CitySettings map (float citySizePercent, const SoundTuning& tuning = {});
}
