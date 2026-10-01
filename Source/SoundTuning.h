#pragma once

#include "SoundSpec.h"

// The values the City Size curves scale to. Shipping builds always use these defaults, which are
// the PRD values; only a BIGCITY_TUNING developer build lets them change, for tuning by ear.
struct SoundTuning
{
    float airMaxGainDb = SoundSpec::airMaxGainDb;
    double airCornerHz = SoundSpec::airCornerHz;
    double airShelfQ = SoundSpec::airShelfQ;
    float wetLevelDbAtMax = SoundSpec::wetLevelDbAtMax;
    float detuneCentsAtMax = SoundSpec::detuneCentsAtMax;
};
