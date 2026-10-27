#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <BinaryData.h>

class TeilLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TeilLookAndFeel()
    {
        if (BinaryData::knob_masked_pngSize > 0)
        {
            knobImage = juce::ImageFileFormat::loadFrom (BinaryData::knob_masked_png,
                                                         BinaryData::knob_masked_pngSize);
        }
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& /*slider*/) override
    {
        const float toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        const float cx = (float) x + (float) width * 0.5f;
        const float cy = (float) y + (float) height * 0.5f;

        // 1. Soft stationary ambient drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.40f));
        g.fillEllipse (cx - 43.0f, cy - 40.0f, 86.0f, 86.0f);

        // 2. Draw rotated authentic photographic knob (false ensures full colors & pointer line are drawn!)
        if (knobImage.isValid())
        {
            const float imgW = (float) knobImage.getWidth();
            const float imgH = (float) knobImage.getHeight();

            auto transform = juce::AffineTransform::rotation (toAngle, imgW * 0.5f, imgH * 0.5f)
                                                  .translated (cx - imgW * 0.5f, cy - imgH * 0.5f);

            g.drawImageTransformed (knobImage, transform, false);
        }
        else
        {
            // Procedural fallback
            const float r = 42.0f;
            juce::ColourGradient grad (juce::Colour (60, 64, 72), cx - r * 0.5f, cy - r * 0.5f,
                                       juce::Colour (25, 27, 32), cx + r, cy + r, false);
            g.setGradientFill (grad);
            g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

            juce::Path p;
            p.startNewSubPath (0.f, -10.f);
            p.lineTo (0.f, -38.f);
            g.setColour (juce::Colour (245, 248, 255));
            g.strokePath (p, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                          juce::AffineTransform::rotation (toAngle).translated (cx, cy));
        }
    }

private:
    juce::Image knobImage;
};
