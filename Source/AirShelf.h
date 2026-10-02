#pragma once

#include "Biquad.h"
#include "SoundSpec.h"

#include <array>

// The "air": an RBJ high-shelf biquad per channel (PRD §6 "Air" rows).
class AirShelf
{
public:
    static constexpr int maxChannels = 2;

    // Exposed so the shelf check measures exactly the coefficients the plugin uses.
    static BiquadCoefficients makeCoefficients (double sampleRate, double cornerHz, double q, float gainDb);

    void prepare (double newSampleRate);
    void reset();
    void update (float gainDb, double cornerHz, double q);

    float processSample (int channel, float input)
    {
        // At 0 dB the RBJ coefficients have b == a bit for bit, so with a cleared state the
        // output equals the input exactly.
        return channelStates[(size_t) channel].process (coefficients, input);
    }

private:
    double sampleRate = 44100.0;
    BiquadCoefficients coefficients { 1.0, 0.0, 0.0, 0.0, 0.0 };
    std::array<BiquadState, maxChannels> channelStates {};
};
