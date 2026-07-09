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
    
    // Sliders & Buttons - GLOBAL & ARP
    juce::ToggleButton polyModeBtn{"Polyphonic"};
    juce::ToggleButton arpEnabledBtn{"Arp On"};
    juce::ToggleButton arpHostSyncBtn{"Host Sync"};
    juce::ComboBox arpModeBox, arpSyncBox;
    juce::Slider glideTimeSlider;
    juce::Slider arpInternalBpmSlider;
    
    // Sliders - VCO & PWM & Vibrato
    juce::Slider sawSlider, pulseSlider, subSlider, noiseSlider;
    juce::Slider pwSlider, pwmLfoAmtSlider, pwmLfoRateSlider, pwmEnvAmtSlider;
    juce::Slider lfoPitchAmtSlider; // Vibrato
    
    // Sliders - VCF
    juce::Slider cutoffSlider, resSlider, filterEnvAmtSlider, filterKbdSlider;
    
    // Sliders - ENV
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    
    // Sliders - Master
    juce::Slider masterVolSlider;
    
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboBoxAttachment>> comboAttachments;
    
    std::vector<juce::Slider*> allSliders;
    
    void setupSlider(juce::Slider& slider, const juce::String& paramID, const juce::String& name);
    void setupButton(juce::ToggleButton& button, const juce::String& paramID, const juce::String& name);
    void setupComboBox(juce::ComboBox& box, const juce::String& paramID, const juce::StringArray& items, const juce::String& name);
    void drawSectionBackground(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title, juce::Colour accentColour);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sh101AudioProcessorEditor)
};
