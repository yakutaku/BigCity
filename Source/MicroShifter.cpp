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

void ShiftVoice::prepare (double sampleRate, double centreDelayMs, double windowMs, double maxSweepMs)
{
    const auto samplesPerMs = sampleRate / millisecondsPerSecond;
    centreDelaySamples = centreDelayMs * samplesPerMs;
    windowSamples = windowMs * samplesPerMs;

    const auto longestDelaySamples = (int) std::ceil (centreDelaySamples + windowSamples * halfCycle + maxSweepMs * samplesPerMs);
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

float ShiftVoice::processSample (float input, double sweepSamples)
{
    delayLine[(size_t) writeIndex] = input;

    // sin² and its half-cycle partner cos² always sum to 1, and each tap is silent as its delay wraps.
    const auto fadeA = (float) juce::square (std::sin (juce::MathConstants<double>::pi * phase));
    const auto tapA = readDelayed (delayForPhase (phase, sweepSamples));
    const auto tapB = readDelayed (delayForPhase (wrapPhase (phase + halfCycle), sweepSamples));

    phase = wrapPhase (phase + phaseIncrement);
    writeIndex = (writeIndex + 1) & writeMask;

    return fadeA * tapA + (1.0f - fadeA) * tapB;
}

double ShiftVoice::delayForPhase (double tapPhase, double sweepSamples) const
{
    return centreDelaySamples + (tapPhase - halfCycle) * windowSamples + sweepSamples;
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

MicroShifter::Sweep MicroShifter::sweepFor (double lfoValue, double depthSamples)
{
    const auto proportionUp = (1.0 + lfoValue) / 2.0;
    return { proportionUp * depthSamples, (1.0 - proportionUp) * depthSamples };
}

void MicroShifter::setModulation (Modulation target)
{
    modulationRateHz.setTargetValue (target.rateHz);
    modulationDepthMs.setTargetValue (juce::jlimit (0.0, maxModulationDepthMs, target.depthMs));
}

void MicroShifter::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    downVoice.prepare (sampleRate, SoundSpec::preDelayLeftMs, SoundSpec::microShiftWindowMs, maxModulationDepthMs);
    upVoice.prepare (sampleRate, SoundSpec::preDelayRightMs, SoundSpec::microShiftWindowMs, maxModulationDepthMs);

    // Same glide time as City Size, so every control responds alike.
    modulationRateHz.reset (sampleRate, SoundSpec::citySizeSmoothingSeconds);
    modulationDepthMs.reset (sampleRate, SoundSpec::citySizeSmoothingSeconds);
    lfoPhase = 0.0;
}

void MicroShifter::reset()
{
    downVoice.reset();
    upVoice.reset();
    lfoPhase = 0.0;
}

MicroShifter::Sweep MicroShifter::nextSweep()
{
    lfoPhase = wrapPhase (lfoPhase + modulationRateHz.getNextValue() / sampleRate);
    const auto depthMs = modulationDepthMs.getNextValue();

    // Without modulation (every shipping build) skip the sine; a zero sweep leaves each delay untouched.
    if (depthMs <= 0.0)
        return { 0.0, 0.0 };

    const auto lfoValue = std::sin (juce::MathConstants<double>::twoPi * lfoPhase);
    return sweepFor (lfoValue, depthMs * sampleRate / millisecondsPerSecond);
}

void MicroShifter::setDetuneCents (float detuneCents)
{
    downVoice.setPitchCents (-detuneCents);
    upVoice.setPitchCents (detuneCents);
}
