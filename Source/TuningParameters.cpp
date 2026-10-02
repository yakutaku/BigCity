#include "TuningParameters.h"

namespace
{
    constexpr int parameterVersion = 1;

    struct TuningRange
    {
        const char* id;
        const char* name;
        float min, max, step, defaultValue;
        const char* label;
    };

    // Ranges from the tuning tasks; defaults are the shipping values from SoundSpec.h, so a tuning
    // build starts out sounding exactly like the shipping one.
    constexpr TuningRange airMaxGain      { "tuningAirMaxGainDb",      "Tuning: air max gain",     4.0f,    10.0f,    0.1f,  SoundSpec::airMaxGainDb,              "dB" };
    constexpr TuningRange airCorner       { "tuningAirCornerHz",       "Tuning: air corner",       8000.0f, 14000.0f, 10.0f, (float) SoundSpec::airCornerHz,      "Hz" };
    constexpr TuningRange airQ            { "tuningAirShelfQ",         "Tuning: air Q",            0.5f,    0.9f,     0.01f, (float) SoundSpec::airShelfQ,        "" };
    constexpr TuningRange maxWetLevel     { "tuningWetLevelDbAtMax",   "Tuning: max wet level",    -12.0f,  -2.0f,    0.1f,  SoundSpec::wetLevelDbAtMax,           "dB" };
    constexpr TuningRange maxDetune       { "tuningDetuneCentsAtMax",  "Tuning: max detune",       6.0f,    14.0f,    0.1f,  SoundSpec::detuneCentsAtMax,          "cents" };
    constexpr TuningRange modulationRate  { "tuningModulationRateHz",  "Tuning: modulation rate",  0.1f,    2.0f,     0.01f, (float) SoundSpec::modulationRateHz, "Hz" };
    constexpr TuningRange modulationDepth { "tuningModulationDepthMs", "Tuning: modulation depth", 0.0f,    (float) MicroShifter::maxModulationDepthMs, 0.01f, (float) SoundSpec::modulationDepthMs, "ms" };
    constexpr TuningRange wetHighPass     { "tuningWetHighPassHz",     "Tuning: wet high-pass",    20.0f,   1000.0f,  1.0f,  (float) SoundSpec::wetHighPassHz,    "Hz" };
    constexpr TuningRange wetLowPass      { "tuningWetLowPassHz",      "Tuning: wet low-pass",     4000.0f, 16000.0f, 10.0f, (float) SoundSpec::wetLowPassHz,     "Hz" };
    constexpr TuningRange wetDrive        { "tuningWetDriveDb",        "Tuning: wet drive",        0.0f,    12.0f,    0.1f,  SoundSpec::wetDriveDb,                "dB" };

    void add (juce::AudioProcessorValueTreeState::ParameterLayout& layout, const TuningRange& range)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { range.id, parameterVersion },
            range.name,
            juce::NormalisableRange<float> { range.min, range.max, range.step },
            range.defaultValue,
            juce::AudioParameterFloatAttributes().withLabel (range.label)));
    }
}

void TuningParameters::addTo (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    for (const auto* range : { &airMaxGain, &airCorner, &airQ, &maxWetLevel, &maxDetune,
                               &modulationRate, &modulationDepth, &wetHighPass, &wetLowPass, &wetDrive })
        add (layout, *range);
}

TuningParameters::Reader::Reader (juce::AudioProcessorValueTreeState& parameters)
    : airMaxGainDb (parameters.getRawParameterValue (airMaxGain.id)),
      airCornerHz (parameters.getRawParameterValue (airCorner.id)),
      airShelfQ (parameters.getRawParameterValue (airQ.id)),
      wetLevelDbAtMax (parameters.getRawParameterValue (maxWetLevel.id)),
      detuneCentsAtMax (parameters.getRawParameterValue (maxDetune.id)),
      modulationRateHz (parameters.getRawParameterValue (modulationRate.id)),
      modulationDepthMs (parameters.getRawParameterValue (modulationDepth.id)),
      wetHighPassHz (parameters.getRawParameterValue (wetHighPass.id)),
      wetLowPassHz (parameters.getRawParameterValue (wetLowPass.id)),
      wetDriveDb (parameters.getRawParameterValue (wetDrive.id))
{
    jassert (airMaxGainDb != nullptr && airCornerHz != nullptr && airShelfQ != nullptr
             && wetLevelDbAtMax != nullptr && detuneCentsAtMax != nullptr
             && modulationRateHz != nullptr && modulationDepthMs != nullptr
             && wetHighPassHz != nullptr && wetLowPassHz != nullptr && wetDriveDb != nullptr);
}

SoundTuning TuningParameters::Reader::read() const
{
    SoundTuning tuning;
    tuning.airMaxGainDb = airMaxGainDb->load();
    tuning.airCornerHz = airCornerHz->load();
    tuning.airShelfQ = airShelfQ->load();
    tuning.wetLevelDbAtMax = wetLevelDbAtMax->load();
    tuning.detuneCentsAtMax = detuneCentsAtMax->load();
    tuning.wetModulation = MicroShifter::Modulation { modulationRateHz->load(), modulationDepthMs->load() };
    tuning.wetFilterCutoffs = WetFilter::Cutoffs { wetHighPassHz->load(), wetLowPassHz->load() };
    tuning.wetDriveDb = wetDriveDb->load();
    return tuning;
}
