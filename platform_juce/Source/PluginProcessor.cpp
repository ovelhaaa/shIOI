#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout Sh101AudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Parâmetros do Mixer dos Osciladores
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sawLevel", 1), "Saw Level", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pulseLevel", 1), "Pulse Level", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("subLevel", 1), "Sub Level", 0.0f, 1.0f, 0.2f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("noiseLevel", 1), "Noise Level", 0.0f, 1.0f, 0.0f));

    // Parâmetros de PWM (Modulação de Pulso)
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pulseWidth", 1), "Pulse Width", 0.05f, 0.95f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pwmLfoAmount", 1), "PWM LFO Amount", 0.0f, 0.95f, 0.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pwmLfoRate", 1), "PWM Rate", 0.05f, 4.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pwmEnvAmount", 1), "PWM Env Amount", -1.0f, 1.0f, 0.2f));

    // Parâmetros do filtro base e modulações
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("cutoff", 1), "Cutoff", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("resonance", 1), "Resonance", 0.0f, 1.0f, 0.2f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("filterEnvAmount", 1), "Filter Env Amount", -1.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("filterKeyboardTracking", 1), "Filter KBD", 0.0f, 1.0f, 0.3f));
    
    // Parâmetros de VCA/Master
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("masterVol", 1), "Master Volume", 0.0f, 1.0f, 0.8f));
    
    // Parâmetros do Envelope
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("attack", 1), "Attack", 0.001f, 5.0f, 0.01f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("decay", 1), "Decay", 0.001f, 5.0f, 0.2f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sustain", 1), "Sustain", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("release", 1), "Release", 0.001f, 5.0f, 0.5f));

    return layout;
}

Sh101AudioProcessor::Sh101AudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

void Sh101AudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/) {
    for (auto& voice : voices) {
        voice.init((float)sampleRate);
    }
    
    modLfo.setSampleRate((float)sampleRate);
    
    // Configura os smoothers para terem fade de 10ms (prevenindo cliques de automação)
    s_cutoff.reset(sampleRate, 0.01);
    s_resonance.reset(sampleRate, 0.01);
    s_masterVol.reset(sampleRate, 0.01);
}

void Sh101AudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary(*xml, destData);
}

void Sh101AudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

void Sh101AudioProcessor::handleMidi(juce::MidiBuffer& midiMessages) {
    for (const auto meta : midiMessages) {
        auto msg = meta.getMessage();
        if (msg.isNoteOn()) {
            // Verifica se a nota já está tocando para roubar dela (retrigger rápido)
            bool retriggered = false;
            for (auto& voice : voices) {
                if (voice.midiNote == msg.getNoteNumber() && (voice.isActive || voice.envelope.isActive())) {
                    float freq = (float)juce::MidiMessage::getMidiNoteInHertz(msg.getNoteNumber());
                    voice.noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), freq);
                    retriggered = true;
                    break;
                }
            }
            
            // Se não foi um retrigger, busca a primeira inativa
            if (!retriggered) {
                for (auto& voice : voices) {
                    if (!voice.isActive && !voice.envelope.isActive()) {
                        float freq = (float)juce::MidiMessage::getMidiNoteInHertz(msg.getNoteNumber());
                        voice.noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), freq);
                        break;
                    }
                }
            }
        } else if (msg.isNoteOff()) {
            for (auto& voice : voices) {
                if (voice.isActive && voice.midiNote == msg.getNoteNumber()) {
                    voice.noteOff();
                }
            }
        } else if (msg.isAllNotesOff()) {
            for (auto& voice : voices) {
                voice.noteOff();
            }
        }
    }
}

void Sh101AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    
    // Processa eventos MIDI que ligam e desligam as notas
    handleMidi(midiMessages);
    
    // 1. Obtém os valores crus do APVTS
    float rawCutoff = apvts.getRawParameterValue("cutoff")->load();
    float rawRes = apvts.getRawParameterValue("resonance")->load();
    float rawVol = apvts.getRawParameterValue("masterVol")->load();
    
    float att = apvts.getRawParameterValue("attack")->load();
    float dec = apvts.getRawParameterValue("decay")->load();
    float sus = apvts.getRawParameterValue("sustain")->load();
    float rel = apvts.getRawParameterValue("release")->load();
    
    // Atualiza Smoothers Base
    s_cutoff.setTargetValue(rawCutoff);
    s_resonance.setTargetValue(rawRes);
    s_masterVol.setTargetValue(rawVol);
    
    // Atualiza Caches por bloco (Oscilladores e Modulação)
    cached_sawLevel = apvts.getRawParameterValue("sawLevel")->load();
    cached_pulseLevel = apvts.getRawParameterValue("pulseLevel")->load();
    cached_subLevel = apvts.getRawParameterValue("subLevel")->load();
    cached_noiseLevel = apvts.getRawParameterValue("noiseLevel")->load();
    
    cached_basePulseWidth = apvts.getRawParameterValue("pulseWidth")->load();
    cached_pwmLfoAmount = apvts.getRawParameterValue("pwmLfoAmount")->load();
    cached_pwmLfoRate = apvts.getRawParameterValue("pwmLfoRate")->load();
    cached_pwmEnvAmount = apvts.getRawParameterValue("pwmEnvAmount")->load();
    
    cached_filterEnvAmount = apvts.getRawParameterValue("filterEnvAmount")->load();
    cached_filterKeyboardTracking = apvts.getRawParameterValue("filterKeyboardTracking")->load();
    
    // Configura o LFO global de PWM
    modLfo.setFrequency(cached_pwmLfoRate);
    
    // Update de parâmetros bloqueados na voz
    for (auto& voice : voices) {
        voice.envelope.setAttackTime(att);
        voice.envelope.setDecayTime(dec);
        voice.envelope.setSustainLevel(sus);
        voice.envelope.setReleaseTime(rel);
    }
    
    int numSamples = buffer.getNumSamples();
    
    // 2. Processamento Amostra-a-Amostra
    for (int s = 0; s < numSamples; ++s) {
        float currentCutoff = s_cutoff.getNextValue();
        float currentRes = s_resonance.getNextValue();
        float currentVol = s_masterVol.getNextValue();
        
        float mixedSample = 0.0f;
        
        // Puxa o sinal do LFO Global
        float lfoValue = modLfo.getSample();
        
        for (auto& voice : voices) {
            // Processa a voz se estiver ativa ou o envelope estiver fazendo release fade
            if (voice.isActive || voice.envelope.isActive()) {
                
                // Extrai nível do envelope
                float envVal = voice.envelope.getNextValue();
                
                // Calcula Modulação de Pulse Width (PWM)
                float lfoMod = lfoValue * cached_pwmLfoAmount * 0.45f;
                float envMod = envVal * cached_pwmEnvAmount * 0.45f;
                float modulatedWidth = cached_basePulseWidth + lfoMod + envMod;
                
                // Clampa (5% a 95% do Duty Cycle)
                if (modulatedWidth < 0.05f) modulatedWidth = 0.05f;
                if (modulatedWidth > 0.95f) modulatedWidth = 0.95f;
                voice.pulseOsc.setPulseWidth(modulatedWidth);
                
                // Extrai osciladores
                float saw = voice.sawOsc.getSample();
                float pulse = voice.pulseOsc.getSample();
                float sub = voice.subOsc.getSample();
                float noise = voice.noiseOsc.getSample();
                
                // Mixa Osciladores (Additive Synthesis)
                float oscMix = (saw * cached_sawLevel) + 
                               (pulse * cached_pulseLevel) + 
                               (sub * cached_subLevel) + 
                               (noise * cached_noiseLevel);
                               
                // Calcula Keyboard Tracking Offset do Filtro (Relativo a C4 = 60)
                float kbdOffset = ((voice.midiNote - 60.0f) / 12.0f) * 0.15f * cached_filterKeyboardTracking;
                
                // Modula o Cutoff
                float modulatedCutoff = currentCutoff + (envVal * cached_filterEnvAmount) + kbdOffset;
                if (modulatedCutoff < 0.0f) modulatedCutoff = 0.0f;
                if (modulatedCutoff > 1.0f) modulatedCutoff = 1.0f;
                
                // Aplica Filtro
                float filtered = voice.filter.process(oscMix, modulatedCutoff, currentRes);
                
                // Adiciona voz na mix final
                mixedSample += filtered * envVal * voice.smoothedVelocity.getNextValue();
            }
        }
        
        // Aplica o Master Gain nativo do core_dsp com headroom de 1/8 (>> 3 na implementação hardware original)
        mixedSample *= 0.125f; 
        mixedSample = masterGain.process(mixedSample, currentVol);
        
        // Joga no output buffer estático sem alocar
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.setSample(ch, s, mixedSample);
        }
    }
}

// Boilerplate JUCE
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new Sh101AudioProcessor();
}

juce::AudioProcessorEditor* Sh101AudioProcessor::createEditor() {
    return new Sh101AudioProcessorEditor(*this);
}
