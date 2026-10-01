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

    // Ranges from the tuning-mode task; defaults are the shipping values from SoundSpec.h.
    constexpr TuningRange airMaxGain    { "tuningAirMaxGainDb",     "Tuning: air max gain",  4.0f,     10.0f,    0.1f,  SoundSpec::airMaxGainDb,             "dB" };
    constexpr TuningRange airCorner     { "tuningAirCornerHz",      "Tuning: air corner",    8000.0f,  14000.0f, 10.0f, (float) SoundSpec::airCornerHz,  "Hz" };
    constexpr TuningRange airQ          { "tuningAirShelfQ",        "Tuning: air Q",         0.5f,     0.9f,     0.01f, (float) SoundSpec::airShelfQ,    "" };
    constexpr TuningRange maxWetLevel   { "tuningWetLevelDbAtMax",  "Tuning: max wet level", -12.0f,   -2.0f,    0.1f,  SoundSpec::wetLevelDbAtMax,          "dB" };
    constexpr TuningRange maxDetune     { "tuningDetuneCentsAtMax", "Tuning: max detune",    6.0f,     14.0f,    0.1f,  SoundSpec::detuneCentsAtMax,         "cents" };

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
    for (const auto* range : { &airMaxGain, &airCorner, &airQ, &maxWetLevel, &maxDetune })
        add (layout, *range);
}

TuningParameters::Reader::Reader (juce::AudioProcessorValueTreeState& parameters)
    : airMaxGainDb (parameters.getRawParameterValue (airMaxGain.id)),
      airCornerHz (parameters.getRawParameterValue (airCorner.id)),
      airShelfQ (parameters.getRawParameterValue (airQ.id)),
      wetLevelDbAtMax (parameters.getRawParameterValue (maxWetLevel.id)),
      detuneCentsAtMax (parameters.getRawParameterValue (maxDetune.id))
{
    jassert (airMaxGainDb != nullptr && airCornerHz != nullptr && airShelfQ != nullptr
             && wetLevelDbAtMax != nullptr && detuneCentsAtMax != nullptr);
}

SoundTuning TuningParameters::Reader::read() const
{
    return { airMaxGainDb->load(), (double) airCornerHz->load(), (double) airShelfQ->load(),
             wetLevelDbAtMax->load(), detuneCentsAtMax->load() };
}
