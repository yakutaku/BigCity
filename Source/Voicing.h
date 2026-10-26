#pragma once

// Which version of the City Size curves an instance plays (PRD §6 "Voicing 2"). It lives in the saved
// state rather than as a host parameter, because parameter IDs are frozen, and so sessions saved by 1.0,
// which have no voicing, keep the sound they were made with. The numbers are what's saved: never renumber.
enum class Voicing
{
    original = 1,      // 1.0
    doubleRange = 2    // 1.1: 50 plays what 100 did in 1.0
};

constexpr Voicing newestVoicing = Voicing::doubleRange;
