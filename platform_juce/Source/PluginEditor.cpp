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
    setSize(980, 550);
    
    // Setup Global & Arp
    setupButton(polyModeBtn, "polyMode", "Poly Mode");
    setupSlider(glideTimeSlider, "glideTime", "Glide Time");
    setupButton(arpEnabledBtn, "arpEnabled", "Arp On/Off");
    setupButton(arpHostSyncBtn, "arpHostSync", "Host Sync");
    
    // Setup BPM Slider especificamente
    addAndMakeVisible(arpInternalBpmSlider);
    arpInternalBpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    arpInternalBpmSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, "arpInternalBpm", arpInternalBpmSlider));
    
    setupComboBox(arpModeBox, "arpMode", {"Up", "Down", "Up/Down", "Random"}, "Arp Mode");
    setupComboBox(arpSyncBox, "arpSync", {"1/4", "1/8", "1/16", "1/32"}, "Arp Sync");
    
    glideTimeSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffffa500));
    arpInternalBpmSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xffffa500));

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
    
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(audioProcessor.apvts, paramID, slider));
}

void Sh101AudioProcessorEditor::setupButton(juce::ToggleButton& button, const juce::String& paramID, const juce::String& name) {
    addAndMakeVisible(button);
    button.setButtonText(name);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(audioProcessor.apvts, paramID, button));
}

void Sh101AudioProcessorEditor::setupComboBox(juce::ComboBox& box, const juce::String& paramID, const juce::StringArray& items, const juce::String& name) {
    addAndMakeVisible(box);
    box.addItemList(items, 1);
    box.setTooltip(name);
    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, paramID, box));
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
    
    // Larguras manuais para as colunas
    int vcoWidth = 200;
    int vcfWidth = 140;
    int envWidth = 140;
    int masterWidth = 120;
    
    auto vcoArea = area.removeFromLeft(vcoWidth);
    area.removeFromLeft(15);
    auto vcfArea = area.removeFromLeft(vcfWidth);
    area.removeFromLeft(15);
    auto envArea = area.removeFromLeft(envWidth);
    area.removeFromLeft(15);
    auto masterArea = area.removeFromLeft(masterWidth);
    area.removeFromLeft(15);
    auto globalArea = area;
    
    drawSectionBackground(g, vcoArea, "OSC & LFO", juce::Colour(0xff00d2ff));
    drawSectionBackground(g, vcfArea, "VCF FILTER", juce::Colour(0xff00ff88));
    drawSectionBackground(g, envArea, "ENVELOPES", juce::Colour(0xffff007f));
    drawSectionBackground(g, masterArea, "MASTER", juce::Colour(0xffff3333));
    drawSectionBackground(g, globalArea, "GLOBAL & ARP", juce::Colour(0xffffa500));
    
    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    
    // Labels dos sliders
    for (auto* s : allSliders) {
        auto bounds = s->getBounds();
        juce::String text = s->getTooltip();
        g.drawText(text, bounds.getX() - 10, bounds.getBottom() + 2, bounds.getWidth() + 20, 15, juce::Justification::centred, false);
    }
    
    // Labels dos ComboBoxes
    for (auto* c : {&arpModeBox, &arpSyncBox}) {
        auto bounds = c->getBounds();
        juce::String text = c->getTooltip();
        g.drawText(text, bounds.getX() - 10, bounds.getY() - 15, bounds.getWidth() + 20, 15, juce::Justification::centred, false);
    }
}

void Sh101AudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(15);
    
    int vcoWidth = 200;
    int vcfWidth = 140;
    int envWidth = 140;
    int masterWidth = 120;
    
    auto vcoArea = area.removeFromLeft(vcoWidth).withTrimmedTop(45).reduced(10);
    area.removeFromLeft(15);
    auto vcfArea = area.removeFromLeft(vcfWidth).withTrimmedTop(45).reduced(10);
    area.removeFromLeft(15);
    auto envArea = area.removeFromLeft(envWidth).withTrimmedTop(45).reduced(10);
    area.removeFromLeft(15);
    auto masterArea = area.removeFromLeft(masterWidth).withTrimmedTop(45).reduced(10);
    area.removeFromLeft(15);
    auto globalArea = area.withTrimmedTop(45).reduced(10);
    
    int knobSize = 65;
    
    // VCO (2 colunas, 5 linhas) centralizado
    int vcoStartX = vcoArea.getX() + (vcoArea.getWidth() - (knobSize * 2 + 10)) / 2;
    int col1 = vcoStartX;
    int col2 = vcoStartX + knobSize + 10;
    
    sawSlider.setBounds(col1, vcoArea.getY(), knobSize, knobSize);
    pulseSlider.setBounds(col2, vcoArea.getY(), knobSize, knobSize);
    subSlider.setBounds(col1, vcoArea.getY() + knobSize + 30, knobSize, knobSize);
    noiseSlider.setBounds(col2, vcoArea.getY() + knobSize + 30, knobSize, knobSize);
    
    pwSlider.setBounds(col1, vcoArea.getY() + (knobSize + 30) * 2, knobSize, knobSize);
    pwmEnvAmtSlider.setBounds(col2, vcoArea.getY() + (knobSize + 30) * 2, knobSize, knobSize);
    
    pwmLfoAmtSlider.setBounds(col1, vcoArea.getY() + (knobSize + 30) * 3, knobSize, knobSize);
    lfoPitchAmtSlider.setBounds(col2, vcoArea.getY() + (knobSize + 30) * 3, knobSize, knobSize); 
    
    pwmLfoRateSlider.setBounds(vcoArea.getX() + (vcoArea.getWidth() - knobSize)/2, vcoArea.getY() + (knobSize + 30) * 4, knobSize, knobSize);

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
    
    // GLOBAL & ARP (Agrupado com cuidado para não sobrepor)
    polyModeBtn.setBounds(globalArea.getX(), globalArea.getY(), globalArea.getWidth(), 24);
    
    glideTimeSlider.setBounds(globalArea.getX() + (globalArea.getWidth() - knobSize)/2, globalArea.getY() + 30, knobSize, knobSize);
    
    arpEnabledBtn.setBounds(globalArea.getX(), globalArea.getY() + knobSize + 50, globalArea.getWidth(), 24);
    arpHostSyncBtn.setBounds(globalArea.getX(), globalArea.getY() + knobSize + 80, globalArea.getWidth(), 24);
    
    arpInternalBpmSlider.setBounds(globalArea.getX(), globalArea.getY() + knobSize + 110, globalArea.getWidth(), 24);
    
    arpModeBox.setBounds(globalArea.getX() + 10, globalArea.getY() + knobSize + 160, globalArea.getWidth() - 20, 24);
    arpSyncBox.setBounds(globalArea.getX() + 10, globalArea.getY() + knobSize + 210, globalArea.getWidth() - 20, 24);
}
