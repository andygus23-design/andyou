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

        if (knobImage.isValid())
        {
            const float imgW = (float) knobImage.getWidth();
            const float imgH = (float) knobImage.getHeight();
            const float targetSize = (float) juce::jmin (width, height);
            const float scale = targetSize / imgW;

            auto transform = juce::AffineTransform::rotation (toAngle, imgW * 0.5f, imgH * 0.5f)
                                                  .scaled (scale)
                                                  .translated (cx - imgW * 0.5f * scale,
                                                               cy - imgH * 0.5f * scale);

            g.drawImageTransformed (knobImage, transform, true);
        }
        else
        {
            // Procedural fallback if image is missing
            const float radius = (float) juce::jmin (width, height) * 0.42f;

            // Drop shadow
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.fillEllipse (cx - radius + 2.f, cy - radius + 3.f, radius * 2.f, radius * 2.f);

            // Outer fluted skirt
            g.setGradientFill (juce::ColourGradient (juce::Colour (55, 58, 65), cx - radius, cy - radius,
                                                     juce::Colour (25, 27, 30), cx + radius, cy + radius, false));
            g.fillEllipse (cx - radius, cy - radius, radius * 2.f, radius * 2.f);

            // Inner cap
            const float innerR = radius * 0.78f;
            g.setGradientFill (juce::ColourGradient (juce::Colour (70, 74, 82), cx - innerR * 0.5f, cy - innerR * 0.5f,
                                                     juce::Colour (35, 37, 42), cx + innerR, cy + innerR, false));
            g.fillEllipse (cx - innerR, cy - innerR, innerR * 2.f, innerR * 2.f);

            // White pointer line
            juce::Path p;
            const float lineLen = innerR * 0.85f;
            p.startNewSubPath (0.f, -innerR * 0.15f);
            p.lineTo (0.f, -lineLen);

            g.setColour (juce::Colour (225, 230, 240));
            g.strokePath (p, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                          juce::AffineTransform::rotation (toAngle).translated (cx, cy));
        }
    }

private:
    juce::Image knobImage;
};
