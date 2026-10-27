#include "PluginEditor.h"

RevTremAudioProcessorEditor::RevTremAudioProcessorEditor (RevTremProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);

    if (BinaryData::pedal_bg_pngSize > 0)
    {
        backgroundImage = juce::ImageFileFormat::loadFrom (BinaryData::pedal_bg_png,
                                                            BinaryData::pedal_bg_pngSize);
    }

    // Initialize 8 Knobs
    setupKnob (reverbSlider,    "reverb");
    setupKnob (characterSlider, "character");
    setupKnob (lengthSlider,    "length");
    setupKnob (airSlider,       "air");
    setupKnob (noiseSlider,     "noise");
    setupKnob (tremoloSlider,   "tremolo");
    setupKnob (tempoSlider,     "tempo");
    setupKnob (filterSlider,    "filter");

    // Initialize Mix/Wet Switch (Horizontal) with native ButtonAttachment
    addAndMakeVisible (mixWetSwitch);
    mixWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, "wetmode", mixWetSwitch);

    // Initialize Mom/Onf Switch (Vertical) with native ButtonAttachment
    addAndMakeVisible (momOnfSwitch);
    momOnfAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, "momentary", momOnfSwitch);

    // Left Arcade Footswitch (Engage / Momentary)
    engageFootswitch.onStateChange = [this] (bool isPressed)
    {
        if (processor.isMomentaryMode())
        {
            processor.setEffectOn (isPressed);
            repaint (196 - 18, 449 - 18, 36, 36);
        }
    };
    engageFootswitch.onClick = [this]
    {
        if (! processor.isMomentaryMode())
        {
            processor.setEffectOn (! processor.getEffectOn());
            repaint (196 - 18, 449 - 18, 36, 36);
        }
    };
    addAndMakeVisible (engageFootswitch);

    // Right Arcade Footswitch (Tap Tempo)
    tapFootswitch.onClick = [this]
    {
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (lastTapTimeMs > 0.0)
        {
            const double deltaMs = now - lastTapTimeMs;
            if (deltaMs >= 25.0 && deltaMs <= 3000.0)
            {
                tapIntervals.push_back (deltaMs);
                if (tapIntervals.size() > 4)
                    tapIntervals.erase (tapIntervals.begin());

                double sum = 0.0;
                for (double d : tapIntervals) sum += d;
                const double avgMs = sum / (double) tapIntervals.size();
                const float hz = (float) (1000.0 / avgMs);
                processor.setTempoFromTap (hz);
            }
            else
            {
                tapIntervals.clear();
            }
        }
        lastTapTimeMs = now;
    };
    addAndMakeVisible (tapFootswitch);

    // Exact canvas dimensions matching the hardware pedal casing
    setSize (824, 736);
    startTimerHz (30);
}

RevTremAudioProcessorEditor::~RevTremAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void RevTremAudioProcessorEditor::setupKnob (juce::Slider& slider, const char* paramId)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (-2.35619f, 2.35619f, true);
    slider.setDoubleClickReturnValue (true, 0.5);
    slider.setMouseDragSensitivity (180);
    slider.setScrollWheelEnabled (true);
    addAndMakeVisible (slider);

    sliderAttachments.push_back (
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts, paramId, slider));
}

void RevTremAudioProcessorEditor::paint (juce::Graphics& g)
{
    // 1) Draw Background Hardware Chassis
    if (backgroundImage.isValid())
    {
        g.drawImage (backgroundImage, getLocalBounds().toFloat());
    }
    else
    {
        g.fillAll (juce::Colour (190, 197, 213));
    }

    // 2) Red ONF Status LED at (196, 449)
    const float redLedX = 196.0f;
    const float redLedY = 449.0f;
    const bool isEngaged = processor.isEffectActive();

    if (isEngaged)
    {
        // Glowing Neon Red Halo over hardware LED
        juce::ColourGradient halo (juce::Colour (255, 30, 30).withAlpha (0.60f), redLedX, redLedY,
                                   juce::Colour (255, 30, 30).withAlpha (0.0f), redLedX + 14.0f, redLedY + 14.0f, true);
        g.setGradientFill (halo);
        g.fillEllipse (redLedX - 14.0f, redLedY - 14.0f, 28.0f, 28.0f);

        juce::ColourGradient core (juce::Colour (255, 175, 175), redLedX - 1.5f, redLedY - 1.5f,
                                   juce::Colour (220, 25, 25), redLedX + 4.0f, redLedY + 4.0f, false);
        g.setGradientFill (core);
        g.fillEllipse (redLedX - 5.0f, redLedY - 5.0f, 10.0f, 10.0f);
    }

    // 3) Green TAP Tempo LED at (723, 458)
    const float grnLedX = 723.0f;
    const float grnLedY = 458.0f;
    const float lfoNorm = 0.5f + 0.5f * std::cos (localLfoPhase);

    if (lfoNorm > 0.40f)
    {
        const float alpha = (lfoNorm - 0.40f) / 0.60f;
        juce::ColourGradient halo (juce::Colour (40, 255, 120).withAlpha (0.55f * alpha), grnLedX, grnLedY,
                                   juce::Colour (40, 255, 120).withAlpha (0.0f), grnLedX + 14.0f, grnLedY + 14.0f, true);
        g.setGradientFill (halo);
        g.fillEllipse (grnLedX - 14.0f, grnLedY - 14.0f, 28.0f, 28.0f);

        juce::ColourGradient core (juce::Colour (205, 255, 225), grnLedX - 1.5f, grnLedY - 1.5f,
                                   juce::Colour (25, 215, 85), grnLedX + 4.0f, grnLedY + 4.0f, false);
        g.setGradientFill (core);
        g.fillEllipse (grnLedX - 5.0f, grnLedY - 5.0f, 10.0f, 10.0f);
    }
}

void RevTremAudioProcessorEditor::resized()
{
    // Row 1 Knobs (centered around y = 155, diameter 120)
    reverbSlider.setBounds    (76,  95, 120, 120);
    characterSlider.setBounds (258, 96, 120, 120);
    lengthSlider.setBounds    (443, 95, 120, 120);
    airSlider.setBounds       (623, 95, 120, 120);

    // Mix/Wet Switch (Horizontal, centered at 140, 258)
    mixWetSwitch.setBounds (116, 245, 48, 26);

    // Row 2 Knobs (centered around y = 345, diameter 120)
    noiseSlider.setBounds   (257, 285, 120, 120);
    tremoloSlider.setBounds (441, 284, 120, 120);
    tempoSlider.setBounds   (623, 285, 120, 120);

    // Mom/Onf Switch (Vertical, centered at 140, 390)
    momOnfSwitch.setBounds (127, 366, 26, 48);

    // Filter Knob (Row 3, centered at 412, 617, diameter 120)
    filterSlider.setBounds (352, 557, 120, 120);

    // Large Arcade Footswitches
    // Left: Engage / Momentary button (centered at 153, 579)
    engageFootswitch.setBounds (73, 499, 160, 160);

    // Right: Tap Tempo button (centered at 659, 570)
    tapFootswitch.setBounds (579, 490, 160, 160);
}

void RevTremAudioProcessorEditor::timerCallback()
{
    if (auto* pTempo = processor.apvts.getRawParameterValue ("tempo"))
    {
        const float tempoHz = pTempo->load();
        localLfoPhase += juce::MathConstants<float>::twoPi * tempoHz * (1.0f / 30.0f);
        if (localLfoPhase > juce::MathConstants<float>::twoPi)
            localLfoPhase -= juce::MathConstants<float>::twoPi;
    }

    // Refresh glowing LED regions
    repaint (196 - 18, 449 - 18, 36, 36);
    repaint (723 - 18, 458 - 18, 36, 36);
}
