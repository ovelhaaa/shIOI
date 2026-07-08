#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// LookAndFeel customizado para estética "Synth Moderno"
class ModernLookAndFeel : public juce::LookAndFeel_V4 {
public:
    ModernLookAndFeel();
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle,
                          const float rotaryEndAngle, juce::Slider& slider) override;
};

class Sh101AudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    Sh101AudioProcessorEditor(Sh101AudioProcessor&);
    ~Sh101AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    Sh101AudioProcessor& audioProcessor;
    ModernLookAndFeel modernLookAndFeel;
    
    // Sliders - VCO
    juce::Slider sawSlider, pulseSlider, subSlider, noiseSlider;
    juce::Slider pwSlider, pwmLfoAmtSlider, pwmLfoRateSlider, pwmEnvAmtSlider;
    
    // Sliders - VCF
    juce::Slider cutoffSlider, resSlider, filterEnvAmtSlider, filterKbdSlider;
    
    // Sliders - ENV
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    
    // Sliders - Master
    juce::Slider masterVolSlider;
    
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;
    std::vector<juce::Slider*> allSliders;
    
    void setupSlider(juce::Slider& slider, const juce::String& paramID, const juce::String& name);
    void drawSectionBackground(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title, juce::Colour accentColour);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sh101AudioProcessorEditor)
};
