#pragma once

#include "SoundSpec.h"

#include <array>

// Biquad coefficients normalised so that a0 = 1.
struct BiquadCoefficients
{
    double b0, b1, b2, a1, a2;
};

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
        // Transposed direct form II in double precision. At 0 dB the RBJ coefficients have
        // b == a bit for bit, so with a cleared state the output equals the input exactly.
        auto& state = channelStates[(size_t) channel];
        const auto x = (double) input;
        const auto y = coefficients.b0 * x + state.s1;
        state.s1 = coefficients.b1 * x - coefficients.a1 * y + state.s2;
        state.s2 = coefficients.b2 * x - coefficients.a2 * y;
        return (float) y;
    }

private:
    struct ChannelState
    {
        double s1 = 0.0, s2 = 0.0;
    };

    double sampleRate = 44100.0;
    BiquadCoefficients coefficients { 1.0, 0.0, 0.0, 0.0, 0.0 };
    std::array<ChannelState, maxChannels> channelStates {};
};
