#include "PluginProcessor.h"

juce::AudioProcessorValueTreeState::ParameterLayout Sh101AudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Parâmetros do filtro
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("cutoff", 1), "Cutoff", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("resonance", 1), "Resonance", 0.0f, 1.0f, 0.2f));
    
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
            // Alocação simplificada buscando primeira voz inativa
            for (auto& voice : voices) {
                if (!voice.isActive && !voice.envelope.isActive()) {
                    float freq = (float)juce::MidiMessage::getMidiNoteInHertz(msg.getNoteNumber());
                    voice.noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), freq);
                    break;
                }
            }
        } else if (msg.isNoteOff()) {
            for (auto& voice : voices) {
                if (voice.midiNote == msg.getNoteNumber()) {
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
    
    // 1. Obtém os valores crus do APVTS e atualiza Smoothers
    float rawCutoff = apvts.getRawParameterValue("cutoff")->load();
    float rawRes = apvts.getRawParameterValue("resonance")->load();
    float rawVol = apvts.getRawParameterValue("masterVol")->load();
    
    float att = apvts.getRawParameterValue("attack")->load();
    float dec = apvts.getRawParameterValue("decay")->load();
    float sus = apvts.getRawParameterValue("sustain")->load();
    float rel = apvts.getRawParameterValue("release")->load();
    
    s_cutoff.setTargetValue(rawCutoff);
    s_resonance.setTargetValue(rawRes);
    s_masterVol.setTargetValue(rawVol);
    
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
        
        for (auto& voice : voices) {
            // Processa a voz se estiver ativa ou o envelope estiver fazendo release fade
            if (voice.isActive || voice.envelope.isActive()) {
                
                // Exemplo rápido: Apenas puxando do SAW
                float saw = voice.sawOsc.getSample();
                
                // Aplica o filtro de core_dsp!
                float filtered = voice.filter.process(saw, currentCutoff, currentRes);
                
                // Aplica o ADSR de core_dsp!
                float envVal = voice.envelope.getNextValue();
                
                // Mistura com velocity suavizada
                mixedSample += filtered * envVal * voice.smoothedVelocity.getNextValue();
            }
        }
        
        // Aplica o Master Gain nativo do core_dsp
        mixedSample *= 0.25f; // Headroom de segurança para as 4 vozes
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
