#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "TeilLookAndFeel.h"
#include <BinaryData.h>

//==============================================================================
// Custom Slide Switch (using juce::Button for native DAW attachment)
//==============================================================================
class SlideSwitch : public juce::Button
{
public:
    SlideSwitch (const juce::String& name, bool isVerticalSwitch)
        : juce::Button (name), isVertical (isVerticalSwitch)
    {
        setClickingTogglesState (true);
    }

    void paintButton (juce::Graphics& g, bool /*isHighlighted*/, bool isButtonDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);

        // Switch metal bezel / recess
        g.setColour (juce::Colour (28, 30, 34));
        g.fillRoundedRectangle (bounds, 3.0f);
        g.setColour (juce::Colour (95, 100, 110));
        g.drawRoundedRectangle (bounds, 3.0f, 1.2f);

        // Slide track
        auto track = bounds.reduced (isVertical ? 3.0f : 2.0f, isVertical ? 2.0f : 3.0f);
        g.setColour (juce::Colour (14, 15, 17));
        g.fillRoundedRectangle (track, 2.0f);

        // Textured black rectangular switch thumb
        const bool toggled = getToggleState();
        juce::Rectangle<float> thumb;
        if (isVertical)
        {
            const float thumbH = track.getHeight() * 0.48f;
            const float thumbY = toggled ? track.getY() : (track.getBottom() - thumbH);
            thumb = juce::Rectangle<float> (track.getX() + 1.f, thumbY, track.getWidth() - 2.f, thumbH);
        }
        else
        {
            const float thumbW = track.getWidth() * 0.48f;
            const float thumbX = toggled ? (track.getRight() - thumbW) : track.getX();
            thumb = juce::Rectangle<float> (thumbX, track.getY() + 1.f, thumbW, track.getHeight() - 2.f);
        }

        // Thumb shadow and gradient
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (thumb.translated (0.5f, 1.0f), 2.0f);

        juce::ColourGradient cg (isButtonDown ? juce::Colour (48, 52, 58) : juce::Colour (68, 72, 80),
                                 thumb.getTopLeft(),
                                 juce::Colour (32, 35, 40), thumb.getBottomRight(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (thumb, 2.0f);

        // Ridge lines on thumb
        g.setColour (juce::Colour (18, 20, 24));
        if (isVertical)
        {
            const float midY = thumb.getCentreY();
            g.drawLine (thumb.getX() + 3.f, midY - 3.f, thumb.getRight() - 3.f, midY - 3.f, 1.0f);
            g.drawLine (thumb.getX() + 3.f, midY,       thumb.getRight() - 3.f, midY,       1.0f);
            g.drawLine (thumb.getX() + 3.f, midY + 3.f, thumb.getRight() - 3.f, midY + 3.f, 1.0f);
        }
        else
        {
            const float midX = thumb.getCentreX();
            g.drawLine (midX - 3.f, thumb.getY() + 3.f, midX - 3.f, thumb.getBottom() - 3.f, 1.0f);
            g.drawLine (midX,       thumb.getY() + 3.f, midX,       thumb.getBottom() - 3.f, 1.0f);
            g.drawLine (midX + 3.f, thumb.getY() + 3.f, midX + 3.f, thumb.getBottom() - 3.f, 1.0f);
        }
    }

private:
    bool isVertical = false;
};

//==============================================================================
// Custom Arcade Footswitch (Engage or Tap)
//==============================================================================
class ArcadeFootswitch : public juce::Component
{
public:
    ArcadeFootswitch() {}

    std::function<void(bool isPressed)> onStateChange;
    std::function<void()> onClick;

    void mouseDown (const juce::MouseEvent&) override
    {
        isPressed = true;
        repaint();
        if (onStateChange) onStateChange (true);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        isPressed = false;
        repaint();
        if (onStateChange) onStateChange (false);
        if (getLocalBounds().contains (e.getPosition()) && onClick)
            onClick();
    }

    void paint (juce::Graphics& g) override
    {
        // Resting: Draws nothing! The authentic hardware arcade button from pedal_bg.png shows through!
        // Pressed: Draws a subtle inner shadow overlay giving tactile physical feedback
        if (isPressed)
        {
            const float w = (float) getWidth();
            const float h = (float) getHeight();
            const float cx = w * 0.5f;
            const float cy = h * 0.5f;
            const float r = juce::jmin (w, h) * 0.40f;

            juce::ColourGradient shadow (juce::Colours::black.withAlpha (0.45f), cx, cy,
                                         juce::Colours::black.withAlpha (0.12f), cx + r, cy + r, true);
            g.setGradientFill (shadow);
            g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        }
    }

private:
    bool isPressed = false;
};

//==============================================================================
// Main Editor
//==============================================================================
class RevTremAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    public juce::Timer
{
public:
    explicit RevTremAudioProcessorEditor (RevTremProcessor&);
    ~RevTremAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    RevTremProcessor& processor;
    TeilLookAndFeel lookAndFeel;

    juce::Image backgroundImage;

    // 8 Knobs
    juce::Slider reverbSlider, characterSlider, lengthSlider, airSlider;
    juce::Slider noiseSlider, tremoloSlider, tempoSlider;
    juce::Slider filterSlider;

    // Slider attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;

    // Slide Switches (with native ButtonAttachment)
    SlideSwitch mixWetSwitch { "Mix/Wet", false }; // Horizontal
    SlideSwitch momOnfSwitch { "Mom/Onf", true };  // Vertical
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> mixWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> momOnfAttachment;

    // Footswitches
    ArcadeFootswitch engageFootswitch;
    ArcadeFootswitch tapFootswitch;

    // Tap tempo tracking
    double lastTapTimeMs = 0.0;
    std::vector<double> tapIntervals;

    // Local LFO animation for green LED when stopped
    float localLfoPhase = 0.0f;

    void setupKnob (juce::Slider& slider, const char* paramId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RevTremAudioProcessorEditor)
};
