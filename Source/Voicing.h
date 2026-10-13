#pragma once

// Which City Size curve to use. Only a BIGCITY_TUNING developer build can pick anything but
// current, which is the PRD curve; the others are beta candidates (SoundSpec "Voicing candidates").
enum class Voicing
{
    current,
    a,   // bent so 50 sounds like today's 70
    b,   // a, plus more intensity towards 100
    c    // more intensity across the whole range
};
