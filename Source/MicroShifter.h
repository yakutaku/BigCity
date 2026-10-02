#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <vector>

// One H910-style voice: a delay line read by two taps half a cycle apart, each sweeping a
// window centred on the pre-delay and fading out with sin² as it wraps (PRD §6 "Micro-shift" rows).
class ShiftVoice
{
public:
    // Allocates the delay line, so call only from prepareToPlay. maxSweepMs is the most the
    // window will ever be pushed past the centre delay.
    void prepare (double sampleRate, double centreDelayMs, double windowMs, double maxSweepMs);
    void reset();
    void setPitchCents (float cents);   // negative shifts down

    // sweepSamples pushes the whole window later; 0 leaves it centred on the pre-delay.
    float processSample (float input, double sweepSamples);

private:
    double delayForPhase (double tapPhase, double sweepSamples) const;
    float readDelayed (double delaySamples) const;

    std::vector<float> delayLine;
    int writeMask = 0;
    int writeIndex = 0;

    double centreDelaySamples = 0.0;
    double windowSamples = 0.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
};

// The stereo micro-shift: left voice detuned down, right voice up. An optional LFO sweeps both
// voices' delays in opposite phase; with no modulation set it is exactly the PRD's micro-shift.
class MicroShifter
{
public:
    struct StereoSample
    {
        float left, right;
    };

    struct Modulation
    {
        double rateHz;
        double depthMs;   // the total delay sweep
    };

    // How far each voice's window is pushed past its pre-delay.
    struct Sweep
    {
        double leftSamples, rightSamples;
    };

    // The delay lines are sized for this, so it caps the modulation depth.
    static constexpr double maxModulationDepthMs = 3.0;

    // Maps an LFO value (-1 to 1) to each voice's sweep, in opposite phase. Sweeps go upward from
    // the pre-delay only: the left voice's window already reaches down to 1 ms.
    static Sweep sweepFor (double lfoValue, double depthSamples);

    // Glides to this modulation. Set it before prepare to start there instead.
    void setModulation (Modulation target);

    void prepare (double newSampleRate);
    void reset();
    void setDetuneCents (float detuneCents);

    StereoSample process (float leftInput, float rightInput)
    {
        const auto sweep = nextSweep();
        return { downVoice.processSample (leftInput, sweep.leftSamples), upVoice.processSample (rightInput, sweep.rightSamples) };
    }

private:
    Sweep nextSweep();

    ShiftVoice downVoice, upVoice;
    double sampleRate = 44100.0;
    double lfoPhase = 0.0;
    juce::SmoothedValue<double> modulationRateHz, modulationDepthMs;
};
