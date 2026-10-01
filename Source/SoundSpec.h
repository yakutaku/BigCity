#pragma once

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
    constexpr double airCornerHz = 12000.0;

    // PRD §6 "Air: Q"
    constexpr double airShelfQ = 0.707;

    // PRD §6 "Air: Q" status: the Day 3 A/B alternative, used only by the shelf check
    constexpr double airShelfAlternativeQ = 0.707;

    // PRD §6 "Air: gain curve"
    constexpr float airMaxGainDb = 6.0f;

    // PRD §6 "Micro-shift: detune" and "Micro-shift: detune curve"
    constexpr float detuneCentsAtMin = 5.0f;
    constexpr float detuneCentsAtMidpoint = 7.0f;
    constexpr float detuneCentsAtMax = 10.0f;

    // PRD §6 "Micro-shift: pre-delay"
    constexpr double preDelayLeftMs = 11.0;
    constexpr double preDelayRightMs = 17.0;

    // PRD §6 "Micro-shift: wet level" and "Micro-shift: wet level curve"
    constexpr float wetLevelDbAtMidpoint = -12.0f;
    constexpr float wetLevelDbAtMax = -6.0f;

    // PRD §6 "Micro-shift: window"
    constexpr double microShiftWindowMs = 20.0;

    // PRD §6 "Smoothing"
    constexpr double citySizeSmoothingSeconds = 0.030;

    // PRD §6 "Bypass crossfade"
    constexpr double bypassCrossfadeSeconds = 0.020;

    // Sound v1 task: shelf coefficients, detune and the wet-level ramp update once per this many samples.
    // Not a sound value; it trades CPU against how finely the 30 ms smoothing is followed.
    constexpr int controlIntervalSamples = 32;
}
