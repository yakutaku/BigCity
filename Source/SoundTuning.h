#pragma once

#include "MicroShifter.h"
#include "SoundSpec.h"
#include "WetFilter.h"

#include <optional>

// The values the City Size curves scale to. Shipping builds always use these defaults, which are
// the PRD values; only a BIGCITY_TUNING developer build lets them change, for tuning by ear.
struct SoundTuning
{
    float airMaxGainDb = SoundSpec::airMaxGainDb;
    double airCornerHz = SoundSpec::airCornerHz;
    double airShelfQ = SoundSpec::airShelfQ;
    float wetLevelDbAtMax = SoundSpec::wetLevelDbAtMax;
    float detuneCentsAtMax = SoundSpec::detuneCentsAtMax;

    // Stages on the wet signal that only tuning builds try out. The PRD has neither, so shipping
    // builds leave them empty and the wet path stays exactly the PRD's.
    std::optional<MicroShifter::Modulation> wetModulation;
    std::optional<WetFilter::Cutoffs> wetFilterCutoffs;
};
