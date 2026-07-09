#include "PluginEditor.h"

// --- ModernLookAndFeel ---
ModernLookAndFeel::ModernLookAndFeel() {
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00d2ff)); // Cyan default
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2a2a));
    setColour(juce::Slider::thumbColourId, juce::Colours::white);
}

void ModernLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, const float rotaryStartAngle,
                                         const float rotaryEndAngle, juce::Slider& slider) {
    auto radius = (float) juce::jmin(width / 2, height / 2) - 4.0f;
    auto centreX = (float) x + (float) width  * 0.5f;
    auto centreY = (float) y + (float) height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Fundo do botão (Dial Escuro)
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillEllipse(rx, ry, rw, rw);
    
    g.setColour(juce::Colour(0xff333333));
    g.drawEllipse(rx, ry, rw, rw, 1.5f);

    // Arco de preenchimento
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(findColour(juce::Slider::rotarySliderOutlineColourId));
    g.strokePath(backgroundArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    // Cor de destaque
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
    g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
    g.strokePath(valueArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Indicador (Thumb)
    juce::Path p;
    auto pointerLength = radius * 0.6f;
    auto pointerThickness = 2.0f;
    p.addRectangle(-pointerThickness * 0.5f, -radius + 2.0f, pointerThickness, pointerLength);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    
    g.setColour(juce::Colours::white);
    g.fillPath(p);
}


// --- Sh101AudioProcessorEditor ---
Sh101AudioProcessorEditor::Sh101AudioProcessorEditor(Sh101AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&modernLookAndFeel);
    setSize(1100, 600);
    
    // Setup Global & Arp
    setupSlider(polyModeSlider, "polyMode", "Poly Mode");
    setupSlider(glideTimeSlider, "glideTime", "Glide Time");
    setupSlider(arpEnabledSlider, "arpEnabled", "Arp On/Off");
    setupSlider(arpModeSlider, "arpMode", "Arp Mode");
    setupSlider(arpSyncSlider, "arpSync", "Arp Sync");
    
    juce::Colour globalColour = juce::Colour(0xffffa500); // Laranja
    for (auto* s : {&polyModeSlider, &glideTimeSlider, &arpEnabledSlider, &arpModeSlider, &arpSyncSlider}) {
        s->setColour(juce::Slider::rotarySliderFillColourId, globalColour);
    }

    // Setup de todos os Sliders
    setupSlider(sawSlider, "sawLevel", "Saw Level");
    setupSlider(pulseSlider, "pulseLevel", "Pulse Lvl");
    setupSlider(subSlider, "subLevel", "Sub Lvl");
    setupSlider(noiseSlider, "noiseLevel", "Noise Lvl");
    setupSlider(pwSlider, "pulseWidth", "Pulse Width");
    setupSlider(pwmEnvAmtSlider, "pwmEnvAmount", "PWM Env Amt");
    setupSlider(pwmLfoAmtSlider, "pwmLfoAmount", "PWM LFO Amt");
    setupSlider(pwmLfoRateSlider, "pwmLfoRate", "LFO Rate");
    setupSlider(lfoPitchAmtSlider, "lfoPitchAmount", "Vibrato Amt");
    
    juce::Colour vcoColour = juce::Colour(0xff00d2ff);
    for (auto* s : {&sawSlider, &pulseSlider, &subSlider, &noiseSlider, &pwSlider, &pwmLfoAmtSlider, &pwmLfoRateSlider, &pwmEnvAmtSlider, &lfoPitchAmtSlider}) {
        s->setColour(juce::Slider::rotarySliderFillColourId, vcoColour);
    }

    setupSlider(cutoffSlider, "cutoff", "Cutoff");
    setupSlider(resSlider, "resonance", "Resonance");
    setupSlider(filterEnvAmtSlider, "filterEnvAmount", "Env Amount");
    setupSlider(filterKbdSlider, "filterKeyboardTracking", "KBD Track");
    
    juce::Colour vcfColour = juce::Colour(0xff00ff88);
    for (auto* s : {&cutoffSlider, &resSlider, &filterEnvAmtSlider, &filterKbdSlider}) {
        s->setColour(juce::Slider::rotarySliderFillColourId, vcfColour);
    }

    setupSlider(attackSlider, "attack", "Attack");
    setupSlider(decaySlider, "decay", "Decay");
    setupSlider(sustainSlider, "sustain", "Sustain");
    setupSlider(releaseSlider, "release", "Release");
    
    juce::Colour envColour = juce::Colour(0xffff007f);
    for (auto* s : {&attackSlider, &decaySlider, &sustainSlider, &releaseSlider}) {
        s->setColour(juce::Slider::rotarySliderFillColourId, envColour);
    }

    setupSlider(masterVolSlider, "masterVol", "Master Vol");
    masterVolSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffff3333));
}

Sh101AudioProcessorEditor::~Sh101AudioProcessorEditor() {
    setLookAndFeel(nullptr);
}

void Sh101AudioProcessorEditor::setupSlider(juce::Slider& slider, const juce::String& paramID, const juce::String& name) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setTooltip(name); // Usado para renderizar o label customizado
    
    addAndMakeVisible(slider);
    allSliders.push_back(&slider);
    
    attachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, paramID, slider));
}

void Sh101AudioProcessorEditor::drawSectionBackground(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title, juce::Colour accentColour) {
    g.setColour(juce::Colour(0xff121212));
    g.fillRoundedRectangle(area.toFloat(), 8.0f);
    
    g.setColour(juce::Colour(0xff222222));
    g.drawRoundedRectangle(area.toFloat(), 8.0f, 2.0f);
    
    juce::Rectangle<int> titleArea = area.removeFromTop(30);
    g.setColour(accentColour.withAlpha(0.2f));
    g.fillRoundedRectangle(titleArea.toFloat(), 8.0f);
    g.fillRect(titleArea.getX(), titleArea.getBottom() - 8, titleArea.getWidth(), 8);

    g.setColour(accentColour);
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText(title, titleArea, juce::Justification::centred, false);
}

void Sh101AudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff181818));
    
    auto area = getLocalBounds().reduced(15);
    int boxWidth = (area.getWidth() - (15 * 4)) / 5;
    
    auto globalArea = area.removeFromLeft(boxWidth);
    area.removeFromLeft(15);
    auto vcoArea = area.removeFromLeft(boxWidth);
    area.removeFromLeft(15);
    auto vcfArea = area.removeFromLeft(boxWidth);
    area.removeFromLeft(15);
    auto envArea = area.removeFromLeft(boxWidth);
    area.removeFromLeft(15);
    auto masterArea = area;
    
    drawSectionBackground(g, globalArea, "GLOBAL & ARP", juce::Colour(0xffffa500));
    drawSectionBackground(g, vcoArea, "OSC & LFO", juce::Colour(0xff00d2ff));
    drawSectionBackground(g, vcfArea, "VCF FILTER", juce::Colour(0xff00ff88));
    drawSectionBackground(g, envArea, "ENVELOPES", juce::Colour(0xffff007f));
    drawSectionBackground(g, masterArea, "MASTER", juce::Colour(0xffff3333));
    
    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    
    for (auto* s : allSliders) {
        auto bounds = s->getBounds();
        juce::String text = s->getTooltip();
        g.drawText(text, bounds.getX() - 10, bounds.getBottom() + 2, bounds.getWidth() + 20, 15, juce::Justification::centred, false);
    }
}

void Sh101AudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(15);
    int boxWidth = (area.getWidth() - (15 * 4)) / 5;
    
    auto globalArea = area.removeFromLeft(boxWidth).withTrimmedTop(45).reduced(10);
    area.removeFromLeft(15);
    auto vcoArea = area.removeFromLeft(boxWidth).withTrimmedTop(45).reduced(10);
    area.removeFromLeft(15);
    auto vcfArea = area.removeFromLeft(boxWidth).withTrimmedTop(45).reduced(10);
    area.removeFromLeft(15);
    auto envArea = area.removeFromLeft(boxWidth).withTrimmedTop(45).reduced(10);
    area.removeFromLeft(15);
    auto masterArea = area.withTrimmedTop(45).reduced(10);
    
    int knobSize = 65;
    
    // GLOBAL & ARP
    polyModeSlider.setBounds(globalArea.getX(), globalArea.getY(), knobSize, knobSize);
    glideTimeSlider.setBounds(globalArea.getX() + knobSize + 10, globalArea.getY(), knobSize, knobSize);
    arpEnabledSlider.setBounds(globalArea.getX() + (globalArea.getWidth() - knobSize)/2, globalArea.getY() + knobSize + 30, knobSize, knobSize);
    arpModeSlider.setBounds(globalArea.getX(), globalArea.getY() + (knobSize + 30) * 2, knobSize, knobSize);
    arpSyncSlider.setBounds(globalArea.getX() + knobSize + 10, globalArea.getY() + (knobSize + 30) * 2, knobSize, knobSize);
    
    // VCO (2 colunas, 5 linhas)
    sawSlider.setBounds(vcoArea.getX(), vcoArea.getY(), knobSize, knobSize);
    pulseSlider.setBounds(vcoArea.getX() + knobSize + 10, vcoArea.getY(), knobSize, knobSize);
    subSlider.setBounds(vcoArea.getX(), vcoArea.getY() + knobSize + 30, knobSize, knobSize);
    noiseSlider.setBounds(vcoArea.getX() + knobSize + 10, vcoArea.getY() + knobSize + 30, knobSize, knobSize);
    
    pwSlider.setBounds(vcoArea.getX(), vcoArea.getY() + (knobSize + 30) * 2, knobSize, knobSize);
    pwmEnvAmtSlider.setBounds(vcoArea.getX() + knobSize + 10, vcoArea.getY() + (knobSize + 30) * 2, knobSize, knobSize);
    
    pwmLfoAmtSlider.setBounds(vcoArea.getX(), vcoArea.getY() + (knobSize + 30) * 3, knobSize, knobSize);
    lfoPitchAmtSlider.setBounds(vcoArea.getX() + knobSize + 10, vcoArea.getY() + (knobSize + 30) * 3, knobSize, knobSize); // Vibrato fica do lado do PWM Amt
    
    pwmLfoRateSlider.setBounds(vcoArea.getX() + (vcoArea.getWidth() - knobSize)/2, vcoArea.getY() + (knobSize + 30) * 4, knobSize, knobSize); // LFO Rate centralizado no fundo

    // VCF
    cutoffSlider.setBounds(vcfArea.getX() + (vcfArea.getWidth() - knobSize)/2, vcfArea.getY(), knobSize, knobSize);
    resSlider.setBounds(vcfArea.getX() + (vcfArea.getWidth() - knobSize)/2, vcfArea.getY() + knobSize + 30, knobSize, knobSize);
    filterEnvAmtSlider.setBounds(vcfArea.getX() + (vcfArea.getWidth() - knobSize)/2, vcfArea.getY() + (knobSize + 30)*2, knobSize, knobSize);
    filterKbdSlider.setBounds(vcfArea.getX() + (vcfArea.getWidth() - knobSize)/2, vcfArea.getY() + (knobSize + 30)*3, knobSize, knobSize);

    // ENV
    attackSlider.setBounds(envArea.getX() + (envArea.getWidth() - knobSize)/2, envArea.getY(), knobSize, knobSize);
    decaySlider.setBounds(envArea.getX() + (envArea.getWidth() - knobSize)/2, envArea.getY() + knobSize + 30, knobSize, knobSize);
    sustainSlider.setBounds(envArea.getX() + (envArea.getWidth() - knobSize)/2, envArea.getY() + (knobSize + 30)*2, knobSize, knobSize);
    releaseSlider.setBounds(envArea.getX() + (envArea.getWidth() - knobSize)/2, envArea.getY() + (knobSize + 30)*3, knobSize, knobSize);

    // MASTER
    masterVolSlider.setBounds(masterArea.getX() + (masterArea.getWidth() - knobSize)/2, masterArea.getY(), knobSize, knobSize);
}
