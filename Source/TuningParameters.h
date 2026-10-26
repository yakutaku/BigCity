#pragma once

#include "SoundTuning.h"
#include "Voicing.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <optional>

// Developer-only, compiled only into BIGCITY_TUNING builds: extra parameters for tuning the sound
// by ear in the generic editor. They never ship, so their IDs are not frozen.
namespace TuningParameters
{
    void addTo (juce::AudioProcessorValueTreeState::ParameterLayout& layout);

    class Reader
    {
    public:
        explicit Reader (juce::AudioProcessorValueTreeState& parameters);

        SoundTuning read() const;

        // The voicing to play instead of the saved one, for A/B listening; none means play the saved one.
        std::optional<Voicing> voicingOverride() const;

    private:
        std::atomic<float>* airMaxGainDb;
        std::atomic<float>* airCornerHz;
        std::atomic<float>* airShelfQ;
        std::atomic<float>* wetLevelDbAtMax;
        std::atomic<float>* detuneCentsAtMax;
        std::atomic<float>* modulationRateHz;
        std::atomic<float>* modulationDepthMs;
        std::atomic<float>* wetHighPassHz;
        std::atomic<float>* wetLowPassHz;
        std::atomic<float>* wetDriveDb;
        std::atomic<float>* voicing;
        std::atomic<float>* voicing2Candidate;
    };
}
