#pragma once

#include <vector>

// One H910-style voice: a delay line read by two taps half a cycle apart, each sweeping a
// window centred on the pre-delay and fading out with sin² as it wraps (PRD §6 "Micro-shift" rows).
class ShiftVoice
{
public:
    // Allocates the delay line, so call only from prepareToPlay.
    void prepare (double sampleRate, double centreDelayMs, double windowMs);
    void reset();
    void setPitchCents (float cents);   // negative shifts down

    float processSample (float input);

private:
    double delayForPhase (double tapPhase) const;
    float readDelayed (double delaySamples) const;

    std::vector<float> delayLine;
    int writeMask = 0;
    int writeIndex = 0;

    double centreDelaySamples = 0.0;
    double windowSamples = 0.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
};

// The stereo micro-shift: left voice detuned down, right voice up.
class MicroShifter
{
public:
    struct StereoSample
    {
        float left, right;
    };

    void prepare (double sampleRate);
    void reset();
    void setDetuneCents (float detuneCents);

    StereoSample process (float leftInput, float rightInput)
    {
        return { downVoice.processSample (leftInput), upVoice.processSample (rightInput) };
    }

private:
    ShiftVoice downVoice, upVoice;
};
