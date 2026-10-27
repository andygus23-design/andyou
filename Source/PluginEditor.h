#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "TeilLookAndFeel.h"
#include <BinaryData.h>

//==============================================================================
// Custom Slide Switch (Horizontal or Vertical)
//==============================================================================
class SlideSwitch : public juce::Component
{
public:
    SlideSwitch (bool isVerticalSwitch) : isVertical (isVerticalSwitch) {}

    void setToggled (bool state)
    {
        if (toggled != state)
        {
            toggled = state;
            repaint();
            if (onToggle) onToggle (toggled);
        }
    }

    bool getToggled() const noexcept { return toggled; }

    std::function<void(bool)> onToggle;

    void mouseDown (const juce::MouseEvent&) override
    {
        setToggled (! toggled);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);

        // Switch metal bezel / recess
        g.setColour (juce::Colour (30, 32, 36));
        g.fillRoundedRectangle (bounds, 3.0f);
        g.setColour (juce::Colour (100, 105, 115));
        g.drawRoundedRectangle (bounds, 3.0f, 1.2f);

        // Slide track
        auto track = bounds.reduced (isVertical ? 4.0f : 2.0f, isVertical ? 2.0f : 4.0f);
        g.setColour (juce::Colour (15, 16, 18));
        g.fillRoundedRectangle (track, 2.0f);

        // Textured black rectangular switch thumb
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
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (thumb.translated (0.5f, 1.0f), 2.0f);

        juce::ColourGradient cg (juce::Colour (70, 75, 82), thumb.getTopLeft(),
                                 juce::Colour (35, 38, 42), thumb.getBottomRight(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (thumb, 2.0f);

        // Ridge lines on thumb
        g.setColour (juce::Colour (20, 22, 25));
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
    bool toggled = false;
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
        if (reallyContains (e.getPosition(), true) && onClick)
            onClick();
    }

    void paint (juce::Graphics& g) override
    {
        const float w = (float) getWidth();
        const float h = (float) getHeight();
        const float cx = w * 0.5f;
        const float cy = h * 0.5f;
        const float baseR = juce::jmin (w, h) * 0.44f;
        const float r = isPressed ? (baseR * 0.94f) : baseR;

        // Subtle drop shadow when resting, deeper inset shadow when pressed
        if (! isPressed)
        {
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillEllipse (cx - r + 2.f, cy - r + 3.f, r * 2.f, r * 2.f);
        }

        // Outer glossy bevel of inner plunger
        juce::ColourGradient bevelGrad (juce::Colour (50, 54, 60), cx - r * 0.6f, cy - r * 0.6f,
                                        juce::Colour (10, 11, 13), cx + r * 0.8f, cy + r * 0.8f, false);
        g.setGradientFill (bevelGrad);
        g.fillEllipse (cx - r, cy - r, r * 2.f, r * 2.f);

        // Domed top face
        const float innerR = r * 0.86f;
        const float offsetY = isPressed ? 1.5f : 0.f;
        juce::ColourGradient domeGrad (isPressed ? juce::Colour (35, 38, 42) : juce::Colour (65, 70, 78),
                                       cx - innerR * 0.4f, cy - innerR * 0.5f + offsetY,
                                       juce::Colour (18, 19, 22),
                                       cx + innerR * 0.6f, cy + innerR * 0.7f + offsetY, false);
        g.setGradientFill (domeGrad);
        g.fillEllipse (cx - innerR, cy - innerR + offsetY, innerR * 2.f, innerR * 2.f);

        // Specular reflection highlight curve
        juce::Path highlight;
        const float arcR = innerR * 0.72f;
        highlight.addCentredArc (cx, cy + offsetY, arcR, arcR * 0.8f, 0.f,
                                 -juce::MathConstants<float>::pi * 0.85f,
                                 -juce::MathConstants<float>::pi * 0.15f, true);
        g.setColour (juce::Colours::white.withAlpha (isPressed ? 0.08f : 0.22f));
        g.strokePath (highlight, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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

    // Slide Switches
    SlideSwitch mixWetSwitch { false }; // Horizontal
    SlideSwitch momOnfSwitch { true };  // Vertical

    // Footswitches
    ArcadeFootswitch engageFootswitch;
    ArcadeFootswitch tapFootswitch;

    // Tap tempo tracking
    double lastTapTimeMs = 0.0;
    std::vector<double> tapIntervals;

    void setupKnob (juce::Slider& slider, const char* paramId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RevTremAudioProcessorEditor)
};
