#pragma once

#include <array>

// Biquad coefficients normalised so that a0 = 1.
struct BiquadCoefficients
{
    double b0, b1, b2, a1, a2;

    // From JUCE's ArrayCoefficients order: b0, b1, b2, a0, a1, a2.
    static BiquadCoefficients normalise (const std::array<double, 6>& raw)
    {
        const auto a0 = raw[3];
        return { raw[0] / a0, raw[1] / a0, raw[2] / a0, raw[4] / a0, raw[5] / a0 };
    }
};

// One channel of a transposed direct form II biquad, in double precision so low cutoffs stay accurate.
struct BiquadState
{
    double s1 = 0.0, s2 = 0.0;

    float process (const BiquadCoefficients& coefficients, float input)
    {
        const auto x = (double) input;
        const auto y = coefficients.b0 * x + s1;
        s1 = coefficients.b1 * x - coefficients.a1 * y + s2;
        s2 = coefficients.b2 * x - coefficients.a2 * y;
        return (float) y;
    }
};
