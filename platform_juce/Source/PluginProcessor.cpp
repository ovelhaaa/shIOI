#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout Sh101AudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Parâmetros Globais / Modo
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("polyMode", 1), "Poly Mode", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("glideTime", 1), "Glide Time", 0.0f, 2.0f, 0.0f));
    
    // Parâmetros do Arpeggiator
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("arpEnabled", 1), "Arp Enabled", false));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("arpMode", 1), "Arp Mode", juce::StringArray{"Up", "Down", "Up/Down", "Random"}, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("arpSync", 1), "Arp Sync", juce::StringArray{"1/4", "1/8", "1/16", "1/32"}, 1));
    
    // Parâmetros do Mixer dos Osciladores
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sawLevel", 1), "Saw Level", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pulseLevel", 1), "Pulse Level", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("subLevel", 1), "Sub Level", 0.0f, 1.0f, 0.2f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("noiseLevel", 1), "Noise Level", 0.0f, 1.0f, 0.0f));

    // Parâmetros de LFO e PWM
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pulseWidth", 1), "Pulse Width", 0.05f, 0.95f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pwmLfoAmount", 1), "PWM LFO Amount", 0.0f, 0.95f, 0.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pwmLfoRate", 1), "LFO Rate", 0.05f, 15.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("pwmEnvAmount", 1), "PWM Env Amount", -1.0f, 1.0f, 0.2f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("lfoPitchAmount", 1), "Vibrato Amount", 0.0f, 1.0f, 0.0f));

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
    currentSampleRate = sampleRate;
    for (auto& voice : voices) {
        voice.init((float)sampleRate);
    }
    
    modLfo.setSampleRate((float)sampleRate);
    
    s_cutoff.reset(sampleRate, 0.01);
    s_resonance.reset(sampleRate, 0.01);
    s_masterVol.reset(sampleRate, 0.01);
    
    arp.reset();
    currentArpPhase = 0.0;
    currentArpNote = -1;
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
    // Força mono se o Glide estiver ativo (ou Arp, ou PolyMode off)
    bool forceMono = !cached_polyMode || cached_glideTime > 0.001f || cached_arpEnabled;
    
    for (const auto meta : midiMessages) {
        auto msg = meta.getMessage();
        if (msg.isNoteOn()) {
            float freq = (float)juce::MidiMessage::getMidiNoteInHertz(msg.getNoteNumber());
            
            if (forceMono) {
                // Modo Mono (Legato/Glide/Arp)
                voices[0].noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), freq, cached_glideTime, (float)currentSampleRate);
            } else {
                // Modo Poly
                bool retriggered = false;
                for (auto& voice : voices) {
                    if (voice.midiNote == msg.getNoteNumber() && (voice.isActive || voice.envelope.isActive())) {
                        voice.noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), freq, 0.0f, (float)currentSampleRate);
                        retriggered = true;
                        break;
                    }
                }
                
                if (!retriggered) {
                    for (auto& voice : voices) {
                        if (!voice.isActive && !voice.envelope.isActive()) {
                            voice.noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), freq, 0.0f, (float)currentSampleRate);
                            break;
                        }
                    }
                }
            }
        } else if (msg.isNoteOff()) {
            if (forceMono) {
                if (voices[0].midiNote == msg.getNoteNumber()) voices[0].noteOff();
            } else {
                for (auto& voice : voices) {
                    if (voice.isActive && voice.midiNote == msg.getNoteNumber()) {
                        voice.noteOff();
                    }
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
    
    // Leitura APVTS
    cached_polyMode = apvts.getRawParameterValue("polyMode")->load() > 0.5f;
    cached_glideTime = apvts.getRawParameterValue("glideTime")->load();
    cached_lfoPitchAmount = apvts.getRawParameterValue("lfoPitchAmount")->load();
    
    cached_arpEnabled = apvts.getRawParameterValue("arpEnabled")->load() > 0.5f;
    cached_arpMode = (int)apvts.getRawParameterValue("arpMode")->load();
    cached_arpSync = (int)apvts.getRawParameterValue("arpSync")->load();
    
    float rawCutoff = apvts.getRawParameterValue("cutoff")->load();
    float rawRes = apvts.getRawParameterValue("resonance")->load();
    float rawVol = apvts.getRawParameterValue("masterVol")->load();
    s_cutoff.setTargetValue(rawCutoff);
    s_resonance.setTargetValue(rawRes);
    s_masterVol.setTargetValue(rawVol);
    
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
    
    modLfo.setFrequency(cached_pwmLfoRate);
    
    float att = apvts.getRawParameterValue("attack")->load();
    float dec = apvts.getRawParameterValue("decay")->load();
    float sus = apvts.getRawParameterValue("sustain")->load();
    float rel = apvts.getRawParameterValue("release")->load();
    
    for (auto& voice : voices) {
        voice.envelope.setAttackTime(att);
        voice.envelope.setDecayTime(dec);
        voice.envelope.setSustainLevel(sus);
        voice.envelope.setReleaseTime(rel);
    }
    
    // Atualiza Arpeggiator Setup
    arp.mode = static_cast<core_dsp::arpeggiator::Mode>(cached_arpMode);
    
    // Arpeggiator ou MIDI
    if (cached_arpEnabled) {
        // Alimenta notas para o Arp
        for (const auto meta : midiMessages) {
            auto msg = meta.getMessage();
            if (msg.isNoteOn()) arp.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
            else if (msg.isNoteOff()) arp.noteOff(msg.getNoteNumber());
            else if (msg.isAllNotesOff()) arp.reset();
        }
    } else {
        handleMidi(midiMessages);
        arp.reset();
        currentArpNote = -1;
    }
    
    // Configurações do Clock / Arp Tick
    double bpm = 120.0;
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            if (pos->getBpm().hasValue()) bpm = *pos->getBpm();
        }
    }
    
    double samplesPerBeat = (60.0 / bpm) * currentSampleRate;
    double syncDivisor = 1.0; 
    if (cached_arpSync == 0) syncDivisor = 1.0; // 1/4
    else if (cached_arpSync == 1) syncDivisor = 2.0; // 1/8
    else if (cached_arpSync == 2) syncDivisor = 4.0; // 1/16
    else if (cached_arpSync == 3) syncDivisor = 8.0; // 1/32
    double stepSizeSamples = samplesPerBeat / syncDivisor;
    
    int numSamples = buffer.getNumSamples();
    
    for (int s = 0; s < numSamples; ++s) {
        
        // --- TICK DO ARPEGGIATOR ---
        if (cached_arpEnabled) {
            currentArpPhase += 1.0;
            if (currentArpPhase >= stepSizeSamples) {
                currentArpPhase -= stepSizeSamples;
                
                int nextNote = arp.getNextNote();
                if (nextNote != currentArpNote) {
                    if (currentArpNote != -1) {
                        voices[0].noteOff(); // Desliga a anterior (é forçado Mono no Arp)
                    }
                    if (nextNote != -1) {
                        float freq = (float)juce::MidiMessage::getMidiNoteInHertz(nextNote);
                        voices[0].noteOn(nextNote, arp.getLastVelocity(), freq, cached_glideTime, (float)currentSampleRate);
                    }
                    currentArpNote = nextNote;
                } else if (nextNote != -1) {
                    // Retrigger da mesma nota
                    voices[0].noteOff();
                    float freq = (float)juce::MidiMessage::getMidiNoteInHertz(nextNote);
                    voices[0].noteOn(nextNote, arp.getLastVelocity(), freq, cached_glideTime, (float)currentSampleRate);
                }
            }
            if (!arp.isActive() && currentArpNote != -1) {
                voices[0].noteOff();
                currentArpNote = -1;
            }
        }
        
        // --- DSP ÁUDIO ---
        float currentCutoff = s_cutoff.getNextValue();
        float currentRes = s_resonance.getNextValue();
        float currentVol = s_masterVol.getNextValue();
        
        float lfoValue = modLfo.getSample();
        float mixedSample = 0.0f;
        
        // Quantidade de afinação (Vibrato) baseada no LFO (+/- ~10% de frequencia máx)
        float vibratoMod = lfoValue * cached_lfoPitchAmount * 0.1f;
        
        for (auto& voice : voices) {
            if (voice.isActive || voice.envelope.isActive()) {
                
                // Atualiza a Frequência com Glide + Vibrato
                voice.updateFrequency(vibratoMod);
                
                float envVal = voice.envelope.getNextValue();
                
                float lfoMod = lfoValue * cached_pwmLfoAmount * 0.45f;
                float envMod = envVal * cached_pwmEnvAmount * 0.45f;
                float modulatedWidth = cached_basePulseWidth + lfoMod + envMod;
                if (modulatedWidth < 0.05f) modulatedWidth = 0.05f;
                if (modulatedWidth > 0.95f) modulatedWidth = 0.95f;
                voice.pulseOsc.setPulseWidth(modulatedWidth);
                
                float saw = voice.sawOsc.getSample();
                float pulse = voice.pulseOsc.getSample();
                float sub = voice.subOsc.getSample();
                float noise = voice.noiseOsc.getSample();
                
                float oscMix = (saw * cached_sawLevel) + (pulse * cached_pulseLevel) + 
                               (sub * cached_subLevel) + (noise * cached_noiseLevel);
                               
                float kbdOffset = ((voice.midiNote - 60.0f) / 12.0f) * 0.15f * cached_filterKeyboardTracking;
                float modulatedCutoff = currentCutoff + (envVal * cached_filterEnvAmount) + kbdOffset;
                if (modulatedCutoff < 0.0f) modulatedCutoff = 0.0f;
                if (modulatedCutoff > 1.0f) modulatedCutoff = 1.0f;
                
                float filtered = voice.filter.process(oscMix, modulatedCutoff, currentRes);
                mixedSample += filtered * envVal * voice.smoothedVelocity.getNextValue();
            }
        }
        
        mixedSample *= 0.125f; 
        mixedSample = masterGain.process(mixedSample, currentVol);
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.setSample(ch, s, mixedSample);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new Sh101AudioProcessor();
}

juce::AudioProcessorEditor* Sh101AudioProcessor::createEditor() {
    return new Sh101AudioProcessorEditor(*this);
}
