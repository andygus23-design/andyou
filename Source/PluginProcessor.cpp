#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Early reflection tap delays (ms) for stereo width; scaled by Character knob.
    constexpr float tapMsL[8] { 7.f, 13.f, 23.f, 31.f, 43.f, 57.f, 71.f, 89.f };
    constexpr float tapMsR[8] { 11.f, 17.f, 29.f, 37.f, 47.f, 61.f, 79.f, 97.f };
    constexpr float maxCharacterScale = 3.2f;
}

juce::AudioProcessorValueTreeState::ParameterLayout RevTremProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto knob = [&] (const char* id, const char* name, float def)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                        NormalisableRange<float> (0.f, 1.f, 0.001f), def));
    };

    knob ("reverb",    "Reverb",    0.5f);
    knob ("character", "Character", 0.5f);
    knob ("length",    "Length",    0.5f);
    knob ("air",       "Air",       0.5f);
    knob ("noise",     "Noise",     0.0f);
    knob ("tremolo",   "Tremolo",   0.0f);

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "tempo", 1 }, "Tempo",
                    NormalisableRange<float> (0.2f, 40.f, 0.01f, 0.4f), 4.f,
                    AudioParameterFloatAttributes().withLabel ("Hz")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "filter", 1 }, "Filter",
                    NormalisableRange<float> (20.f, 20000.f, 1.f, 0.3f), 20.f,
                    AudioParameterFloatAttributes().withLabel ("Hz")));

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "wetmode", 1 },   "Mix/Wet",   false)); // false = Mix, true = Wet
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "on", 1 },        "On",        true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "momentary", 1 }, "Momentary", false));

    return { std::make_move_iterator (p.begin()), std::make_move_iterator (p.end()) };
}

RevTremProcessor::RevTremProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
    , currentLfoPhase (0.f)
    , isEffectCurrentlyActive (true)
{
    pReverb    = apvts.getRawParameterValue ("reverb");
    pCharacter = apvts.getRawParameterValue ("character");
    pLength    = apvts.getRawParameterValue ("length");
    pAir       = apvts.getRawParameterValue ("air");
    pNoise     = apvts.getRawParameterValue ("noise");
    pTremolo   = apvts.getRawParameterValue ("tremolo");
    pTempo     = apvts.getRawParameterValue ("tempo");
    pFilter    = apvts.getRawParameterValue ("filter");
    pWetMode   = apvts.getRawParameterValue ("wetmode");
    pOn        = apvts.getRawParameterValue ("on");
    pMomentary = apvts.getRawParameterValue ("momentary");
}

bool RevTremProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void RevTremProcessor::updateReverbParams()
{
    juce::Reverb::Parameters rp;
    rp.roomSize   = 0.30f + 0.69f * pLength->load();   // Length -> decay tail length
    rp.damping    = 1.0f - pAir->load();               // Air -> high frequency sizzle/brightness
    rp.wetLevel   = 0.33f;
    rp.dryLevel   = 0.0f;                              // Dry handled directly in mixer stage
    rp.width      = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
}

void RevTremProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;

    reverb.reset();
    updateReverbParams();
    reverb.setSampleRate (sr);

    erSize = (int) (0.12 * maxCharacterScale * sr) + 8;
    for (auto& b : erBuffer) b.assign ((size_t) erSize, 0.f);
    erWrite = 0;

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, 2 };
    for (auto& f : lowCut)
    {
        f.prepare (spec);
        f.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        f.setResonance (0.707f);
        f.reset();
    }
    lastCutoff = -1.f;

    work.setSize (4, samplesPerBlock, false, true, false);
    random.setSeedRandomly();
    lfoPhase = 0.f;

    sLevel.reset (sr, 0.03);
    sFeed.reset (sr, 0.02);
    sNoise.reset (sr, 0.05);
    sDepth.reset (sr, 0.03);
    sCharacter.reset (sr, 0.2);
    sCutoff.reset (sr, 0.05);

    sLevel.setCurrentAndTargetValue (pReverb->load());
    sFeed.setCurrentAndTargetValue (pOn->load() > 0.5f ? 1.f : 0.f);
    sNoise.setCurrentAndTargetValue (pNoise->load());
    sDepth.setCurrentAndTargetValue (pTremolo->load());
    sCharacter.setCurrentAndTargetValue (pCharacter->load());
    sCutoff.setCurrentAndTargetValue (pFilter->load());
}

void RevTremProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0)
        return;

    if (work.getNumSamples() < n)
        work.setSize (4, n, false, true, true);

    float* inL = buffer.getWritePointer (0);
    float* inR = buffer.getWritePointer (1);
    float* revL = work.getWritePointer (0);
    float* revR = work.getWritePointer (1);
    float* erL  = work.getWritePointer (2);
    float* erR  = work.getWritePointer (3);

    // Target parameters for this block
    sLevel.setTargetValue (pReverb->load());
    const bool onState = pOn->load() > 0.5f;
    sFeed.setTargetValue (onState ? 1.f : 0.f);
    sNoise.setTargetValue (pNoise->load());
    sDepth.setTargetValue (pTremolo->load());
    sCharacter.setTargetValue (pCharacter->load());
    sCutoff.setTargetValue (juce::jlimit (20.f, (float) (0.45 * sampleRate), pFilter->load()));

    isEffectCurrentlyActive.store (onState, std::memory_order_relaxed);

    updateReverbParams();

    const float lfoInc = juce::MathConstants<float>::twoPi * pTempo->load() / (float) sampleRate;
    const bool wetOnly = pWetMode->load() > 0.5f;

    // --- 1) Reverb input: audio signal (with trails preserved on bypass) + white noise + early reflections ---
    for (int i = 0; i < n; ++i)
    {
        const float feed  = sFeed.getNextValue();
        const float noise = sNoise.getNextValue();
        const float chr   = sCharacter.getNextValue();
        const float scale = 0.4f + (maxCharacterScale - 0.4f) * chr;
        const float erLvl = 0.10f + 0.45f * chr;

        const float xl = inL[i] * feed;
        const float xr = inR[i] * feed;

        erBuffer[0][(size_t) erWrite] = xl;
        erBuffer[1][(size_t) erWrite] = xr;

        float sumL = 0.f, sumR = 0.f, g = 1.f;
        for (int k = 0; k < numTaps; ++k)
        {
            const int dL = juce::jmin (erSize - 1, (int) (tapMsL[k] * scale * 0.001f * (float) sampleRate));
            const int dR = juce::jmin (erSize - 1, (int) (tapMsR[k] * scale * 0.001f * (float) sampleRate));
            const float sign = (k & 1) ? -1.f : 1.f;
            sumL += sign * g * erBuffer[1][(size_t) ((erWrite - dL + erSize) % erSize)];
            sumR += sign * g * erBuffer[0][(size_t) ((erWrite - dR + erSize) % erSize)];
            g *= 0.80f;
        }
        erWrite = (erWrite + 1) % erSize;

        // Built-in white noise generator injected into reverb
        const float nAmt = noise * noise * 0.25f * feed;
        revL[i] = xl + nAmt * (random.nextFloat() * 2.f - 1.f);
        revR[i] = xr + nAmt * (random.nextFloat() * 2.f - 1.f);
        erL[i]  = sumL * erLvl * 0.5f;
        erR[i]  = sumR * erLvl * 0.5f;
    }

    // --- 2) Algorithmic Reverb Tank ---
    reverb.processStereo (revL, revR, n);

    // --- 3) High-pass Filter -> Tremolo -> Mix / Output ---
    for (int i = 0; i < n; ++i)
    {
        const float cutoff = sCutoff.getNextValue();
        if (cutoff != lastCutoff)
        {
            for (auto& f : lowCut) f.setCutoffFrequency (cutoff);
            lastCutoff = cutoff;
        }

        float wl = revL[i] + erL[i];
        float wr = revR[i] + erR[i];

        // 24 dB/oct low-cut filter
        wl = lowCut[1].processSample (0, lowCut[0].processSample (0, wl));
        wr = lowCut[1].processSample (1, lowCut[0].processSample (1, wr));

        // Tremolo modulation
        const float depth = sDepth.getNextValue();
        const float gain  = 1.f - depth * 0.5f * (1.f - std::cos (lfoPhase));
        lfoPhase += lfoInc;
        if (lfoPhase > juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;

        const float lvl = sLevel.getNextValue();
        const float dryL = wetOnly ? 0.f : inL[i];
        const float dryR = wetOnly ? 0.f : inR[i];

        inL[i] = dryL + wl * gain * lvl;
        inR[i] = dryR + wr * gain * lvl;
    }

    currentLfoPhase.store (lfoPhase, std::memory_order_relaxed);
}

void RevTremProcessor::setTempoFromTap (float hz)
{
    if (auto* p = apvts.getParameter ("tempo"))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (0.2f, 40.0f, hz)));
}

void RevTremProcessor::setEffectOn (bool shouldBeOn)
{
    if (auto* p = apvts.getParameter ("on"))
        p->setValueNotifyingHost (shouldBeOn ? 1.0f : 0.0f);
}

bool RevTremProcessor::getEffectOn() const noexcept
{
    return pOn != nullptr && pOn->load() > 0.5f;
}

bool RevTremProcessor::isMomentaryMode() const noexcept
{
    return pMomentary != nullptr && pMomentary->load() > 0.5f;
}

juce::AudioProcessorEditor* RevTremProcessor::createEditor()
{
    return new RevTremAudioProcessorEditor (*this);
}

void RevTremProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void RevTremProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RevTremProcessor();
}
