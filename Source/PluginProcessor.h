#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
//  AndYou RevTrem  -  reverb + tremolo (inspirado en el concepto "reverb -> tremolo")
//
//  Cadena de señal:
//     in --(ON/OFF)--> [early reflections] + [reverb] <-- [ruido blanco]
//                                  |
//                         [filtro low-cut] -> [tremolo] -> [Reverb level]
//     salida = (Mix: dry + wet) o (Wet: solo wet)
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

    const juce::String getName() const override { return "AndYou RevTrem"; }
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

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateReverbParams();

    // --- Reverb + early reflections ---
    juce::Reverb reverb;
    static constexpr int numTaps = 8;
    std::array<std::vector<float>, 2> erBuffer;
    int erSize = 0, erWrite = 0;

    // --- Filtro low-cut (2 etapas = 24 dB/oct), 2 canales ---
    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> lowCut;
    float lastCutoff = -1.f;

    // --- Ruido / modulación ---
    juce::Random random;
    float lfoPhase = 0.f;

    // buffer de trabajo: ch0/1 = entrada y salida del reverb, ch2/3 = early reflections
    juce::AudioBuffer<float> work;

    juce::SmoothedValue<float> sLevel, sFeed, sNoise, sDepth, sCharacter, sCutoff;
    double sampleRate = 44100.0;

    std::atomic<float> *pReverb{}, *pCharacter{}, *pLength{}, *pAir{}, *pNoise{},
                       *pTremolo{}, *pTempo{}, *pFilter{}, *pWetMode{}, *pOn{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RevTremProcessor)
};
