#pragma once

#include <array>

// Every audio constant BigCity uses. Each comment names its row in docs/PRD.md §6 "Sound spec"
// (or, where stated, the CLAUDE.md parameter table). Change a value here only after changing the PRD.
namespace SoundSpec
{
    // CLAUDE.md "Parameters": citySize range
    constexpr float citySizeMinPercent = 0.0f;
    constexpr float citySizeMaxPercent = 100.0f;
    constexpr float citySizeStepPercent = 0.1f;

    // PRD §6 "Default"
    constexpr float citySizeDefaultPercent = 50.0f;

    // PRD §6 "Micro-shift: detune" and "Micro-shift: wet level" give values at 0, 50 and 100
    constexpr float curveMidpointPercent = 50.0f;

    // PRD §6 "Air: corner frequency"
    constexpr double airCornerHz = 10000.0;

    // PRD §6 "Air: Q"
    constexpr double airShelfQ = 0.707;

    // PRD §6 "Air: Q" status: the Q that lost the A/B listen, used only by the shelf check
    constexpr double airShelfAlternativeQ = 0.5;

    // PRD §6 "Air: gain curve"
    constexpr float airMaxGainDb = 7.8f;

    // PRD §6 "Micro-shift: detune" and "Micro-shift: detune curve"
    constexpr float detuneCentsAtMin = 5.5f;
    constexpr float detuneCentsAtMidpoint = 7.7f;
    constexpr float detuneCentsAtMax = 11.0f;

    // PRD §6 "Micro-shift: pre-delay"
    constexpr double preDelayLeftMs = 11.0;
    constexpr double preDelayRightMs = 17.0;

    // PRD §6 "Micro-shift: wet level" and "Micro-shift: wet level curve"
    constexpr float wetLevelDbAtMidpoint = -9.0f;
    constexpr float wetLevelDbAtMax = -3.0f;

    // PRD §6 "Micro-shift: window"
    constexpr double microShiftWindowMs = 20.0;

    // PRD §6 "Micro-shift: modulation"
    constexpr double modulationRateHz = 0.72;
    constexpr double modulationDepthMs = 1.65;

    // PRD §6 "Wet: high-pass" and "Wet: low-pass"
    constexpr double wetHighPassHz = 250.0;
    constexpr double wetLowPassHz = 7000.0;

    // PRD §6 "Wet: drive"
    constexpr float wetDriveDb = 3.0f;

    // PRD §6 "Auto gain": output gain in dB at City Size 0, 10, 20 … 100, from Tests/AutoGainMeasure.
    // The first value must stay exactly 0 so City Size 0 nulls against bypass.
    constexpr float autoGainStepPercent = 10.0f;
    constexpr std::array<float, 11> autoGainDb { 0.00f, -0.02f, -0.07f, -0.15f, -0.24f, -0.35f, -0.47f, -0.60f, -0.77f, -0.97f, -1.22f };

    // PRD §6 "Smoothing"
    constexpr double citySizeSmoothingSeconds = 0.030;

    // PRD §6 "Bypass crossfade"
    constexpr double bypassCrossfadeSeconds = 0.020;

    // Sound v1 task: shelf coefficients, detune and the wet-level ramp update once per this many samples.
    // Not a sound value; it trades CPU against how finely the 30 ms smoothing is followed.
    constexpr int controlIntervalSamples = 32;
}
