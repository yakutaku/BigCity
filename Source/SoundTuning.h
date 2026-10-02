#pragma once

#include "MicroShifter.h"
#include "SoundSpec.h"
#include "WetFilter.h"

// The values the City Size curves scale to, and the wet path's fixed settings. Shipping builds
// always use these defaults, which are the PRD values; only a BIGCITY_TUNING developer build lets
// them change, for tuning by ear.
struct SoundTuning
{
    float airMaxGainDb = SoundSpec::airMaxGainDb;
    double airCornerHz = SoundSpec::airCornerHz;
    double airShelfQ = SoundSpec::airShelfQ;
    float wetLevelDbAtMax = SoundSpec::wetLevelDbAtMax;
    float detuneCentsAtMax = SoundSpec::detuneCentsAtMax;
    MicroShifter::Modulation wetModulation { SoundSpec::modulationRateHz, SoundSpec::modulationDepthMs };
    WetFilter::Cutoffs wetFilterCutoffs { SoundSpec::wetHighPassHz, SoundSpec::wetLowPassHz };
    float wetDriveDb = SoundSpec::wetDriveDb;
};
