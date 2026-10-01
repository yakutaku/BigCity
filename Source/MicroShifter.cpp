#include "MicroShifter.h"
#include "SoundSpec.h"

#include <juce_core/juce_core.h>

#include <cmath>

namespace
{
    constexpr double centsPerOctave = 1200.0;
    constexpr double millisecondsPerSecond = 1000.0;
    constexpr double halfCycle = 0.5;

    // 4-point interpolation reads one sample before and two after the read position.
    constexpr int interpolationGuardSamples = 4;

    double wrapPhase (double phase)
    {
        return phase - std::floor (phase);
    }

    // 4-point, 3rd-order Hermite: keeps the top end steady as the read position slides between samples.
    float hermite (float before, float at, float after, float twoAfter, float fraction)
    {
        const auto c1 = 0.5f * (after - before);
        const auto c2 = before - 2.5f * at + 2.0f * after - 0.5f * twoAfter;
        const auto c3 = 0.5f * (twoAfter - before) + 1.5f * (at - after);
        return ((c3 * fraction + c2) * fraction + c1) * fraction + at;
    }
}

void ShiftVoice::prepare (double sampleRate, double centreDelayMs, double windowMs)
{
    const auto samplesPerMs = sampleRate / millisecondsPerSecond;
    centreDelaySamples = centreDelayMs * samplesPerMs;
    windowSamples = windowMs * samplesPerMs;

    const auto longestDelaySamples = (int) std::ceil (centreDelaySamples + windowSamples * halfCycle);
    const auto delayLineLength = juce::nextPowerOfTwo (longestDelaySamples + interpolationGuardSamples);
    delayLine.assign ((size_t) delayLineLength, 0.0f);
    writeMask = delayLineLength - 1;

    phaseIncrement = 0.0;
    reset();
}

void ShiftVoice::reset()
{
    std::fill (delayLine.begin(), delayLine.end(), 0.0f);
    writeIndex = 0;
    phase = 0.0;
}

void ShiftVoice::setPitchCents (float cents)
{
    // Reading at pitch ratio r means the delay changes by (1 - r) samples per sample.
    const auto pitchRatio = std::pow (2.0, (double) cents / centsPerOctave);
    phaseIncrement = (1.0 - pitchRatio) / windowSamples;
}

float ShiftVoice::processSample (float input)
{
    delayLine[(size_t) writeIndex] = input;

    // sin² and its half-cycle partner cos² always sum to 1, and each tap is silent as its delay wraps.
    const auto fadeA = (float) juce::square (std::sin (juce::MathConstants<double>::pi * phase));
    const auto tapA = readDelayed (delayForPhase (phase));
    const auto tapB = readDelayed (delayForPhase (wrapPhase (phase + halfCycle)));

    phase = wrapPhase (phase + phaseIncrement);
    writeIndex = (writeIndex + 1) & writeMask;

    return fadeA * tapA + (1.0f - fadeA) * tapB;
}

double ShiftVoice::delayForPhase (double tapPhase) const
{
    return centreDelaySamples + (tapPhase - halfCycle) * windowSamples;
}

float ShiftVoice::readDelayed (double delaySamples) const
{
    const auto readPosition = (double) writeIndex - delaySamples;
    const auto wholePosition = std::floor (readPosition);
    const auto fraction = (float) (readPosition - wholePosition);
    const auto index = (int) wholePosition;

    const auto sampleAt = [this, index] (int offset) { return delayLine[(size_t) ((index + offset) & writeMask)]; };
    return hermite (sampleAt (-1), sampleAt (0), sampleAt (1), sampleAt (2), fraction);
}

void MicroShifter::prepare (double sampleRate)
{
    downVoice.prepare (sampleRate, SoundSpec::preDelayLeftMs, SoundSpec::microShiftWindowMs);
    upVoice.prepare (sampleRate, SoundSpec::preDelayRightMs, SoundSpec::microShiftWindowMs);
}

void MicroShifter::reset()
{
    downVoice.reset();
    upVoice.reset();
}

void MicroShifter::setDetuneCents (float detuneCents)
{
    downVoice.setPitchCents (-detuneCents);
    upVoice.setPitchCents (detuneCents);
}
