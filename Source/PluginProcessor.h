#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
//  Teil2 Revolo / AndYou RevTrem  -  reverb + tremolo effects processor
//
//  Signal path:
//     in --(ON/OFF)--> [early reflections] + [reverb] <-- [white noise]
//                                  |
//                         [low-cut filter] -> [tremolo] -> [reverb level]
//     out = (Mix: dry + wet) or (Wet: 100% wet for send/return)
//==============================================================================
class RevTremProcessor : public juce::AudioProcessor
{
public:
    RevTremProcessor();
    ~RevTremProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Teil2 Revolo"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // UI Feedback & Tap Tempo
    float getLfoPhase() const noexcept { return currentLfoPhase.load (std::memory_order_relaxed); }
    bool isEffectActive() const noexcept { return pOn != nullptr && pOn->load() > 0.5f; }
    void setTempoFromTap (float hz);
    void setEffectOn (bool shouldBeOn);
    bool getEffectOn() const noexcept;
    bool isMomentaryMode() const noexcept;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateReverbParams();

    // --- Reverb + early reflections ---
    juce::Reverb reverb;
    static constexpr int numTaps = 8;
    std::array<std::vector<float>, 2> erBuffer;
    int erSize = 0, erWrite = 0;

    // --- Low-cut filter (2-stage = 24 dB/oct), 2 channels ---
    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> lowCut;
    float lastCutoff = -1.f;

    // --- Noise / modulation ---
    juce::Random random;
    float lfoPhase = 0.f;

    std::atomic<float> currentLfoPhase { 0.f };
    std::atomic<bool> isEffectCurrentlyActive { true };

    // Work buffer: ch0/1 = reverb in/out, ch2/3 = early reflections
    juce::AudioBuffer<float> work;

    juce::SmoothedValue<float> sLevel, sFeed, sNoise, sDepth, sCharacter, sCutoff;
    double sampleRate = 44100.0;

    std::atomic<float> *pReverb{}, *pCharacter{}, *pLength{}, *pAir{}, *pNoise{},
                       *pTremolo{}, *pTempo{}, *pFilter{}, *pWetMode{}, *pOn{}, *pMomentary{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RevTremProcessor)
};
