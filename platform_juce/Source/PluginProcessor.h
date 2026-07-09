#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

// Inclui os módulos do core_dsp 
// (os caminhos relativos ou incluídos via target_include_directories pelo CMake)
#include "../core_dsp/Oscillators.h"
#include "../core_dsp/Filters.h"
#include "../core_dsp/Envelope.h"
#include "../core_dsp/Gain.h"
#include "../core_dsp/Arpeggiator.h"

// NOTA SOBRE O SampleType:
// Como não estamos definindo a macro 'PLATFORM_PICO' no CMakeLists deste projeto JUCE,
// o arquivo AudioTypes.h automaticamente fará o fallback (via #else) definindo:
// `using SampleType = float;`
// Isso garante que todos os módulos do core_dsp instanciados aqui operem nativamente
// em ponto flutuante de 32 bits, o que é o formato padrão e ideal para buffers em DAWs.

class Sh101AudioProcessor : public juce::AudioProcessor {
public:
    Sh101AudioProcessor();
    ~Sh101AudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "shIOI"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int /*index*/) override {}
    const juce::String getProgramName(int /*index*/) override { return {}; }
    void changeProgramName(int /*index*/, const juce::String& /*newName*/) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    // Estrutura estática da Voz contendo os módulos core_dsp
    struct Voice {
        core_dsp::oscillator::Saw sawOsc;
        core_dsp::oscillator::Pulse pulseOsc;
        core_dsp::oscillator::Pulse subOsc;
        core_dsp::oscillator::Noise noiseOsc;
        core_dsp::filter::MoogLadder filter;
        core_dsp::envelope::ADSR envelope;
        
        int midiNote = -1;
        bool isActive = false;
        float velocity = 0.0f;
        
        // Smoothers nativos do JUCE para a velocidade e frequência (Glide)
        juce::LinearSmoothedValue<float> smoothedVelocity {0.0f};
        juce::LinearSmoothedValue<float> smoothedFreq {440.0f};

        void init(float sampleRate) {
            sawOsc.setSampleRate(sampleRate);
            pulseOsc.setSampleRate(sampleRate);
            subOsc.setSampleRate(sampleRate);
            envelope.init(sampleRate);
            smoothedVelocity.reset(sampleRate, 0.005);
            smoothedVelocity.setCurrentAndTargetValue(0.0f);
            smoothedFreq.reset(sampleRate, 0.005);
            smoothedFreq.setCurrentAndTargetValue(440.0f);
        }
        
        void noteOn(int note, float vel, float freq, float glideTimeSeconds, float sampleRate) {
            midiNote = note;
            isActive = true;
            velocity = vel;
            smoothedVelocity.setTargetValue(vel);
            
            // Só reseta a fase se não estivermos fazendo portamento/glide
            if (glideTimeSeconds <= 0.001f) {
                sawOsc.resetPhase();
                pulseOsc.resetPhase();
                subOsc.resetPhase();
                
                smoothedFreq.reset(sampleRate, 0.001); // Instantâneo
                smoothedFreq.setCurrentAndTargetValue(freq);
            } else {
                smoothedFreq.reset(sampleRate, glideTimeSeconds);
                smoothedFreq.setTargetValue(freq);
            }
            envelope.noteOn();
        }
        
        void updateFrequency(float vibratoMod) {
            float baseFreq = smoothedFreq.getNextValue();
            float finalFreq = baseFreq + (baseFreq * vibratoMod);
            
            sawOsc.setFrequency(finalFreq);
            pulseOsc.setFrequency(finalFreq);
            subOsc.setFrequency(finalFreq * 0.5f);
        }
        
        void noteOff() {
            isActive = false;
            envelope.noteOff();
        }
    };

    // Pool de vozes pré-alocado
    static constexpr int NUM_VOICES = 4;
    std::array<Voice, NUM_VOICES> voices;
    
    // Gerenciamento de Parâmetros
public:
    juce::AudioProcessorValueTreeState apvts;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Global LFO (Para PWM e outras modulações)
    core_dsp::oscillator::ModLFO modLfo;
    
    // Smoothers para prevenir Clicks e Zipper Noise (Atualização Sample-a-Sample)
    juce::LinearSmoothedValue<float> s_cutoff {0.5f};
    juce::LinearSmoothedValue<float> s_resonance {0.2f};
    juce::LinearSmoothedValue<float> s_masterVol {0.8f};
    
    // Cache de Parâmetros Secundários (Atualização por Bloco)
    float cached_sawLevel = 1.0f;
    float cached_pulseLevel = 0.5f;
    float cached_subLevel = 0.2f;
    float cached_noiseLevel = 0.0f;
    
    float cached_basePulseWidth = 0.5f;
    float cached_pwmLfoAmount = 0.1f;
    float cached_pwmLfoRate = 0.5f;
    float cached_pwmEnvAmount = 0.2f;
    
    float cached_filterEnvAmount = 0.0f;
    float cached_filterKeyboardTracking = 0.3f;
    
    // Novos caches: Glide, Vibrato e Arpeggiator
    float cached_glideTime = 0.0f;
    float cached_lfoPitchAmount = 0.0f;
    
    bool cached_arpEnabled = false;
    int cached_arpMode = 0;
    int cached_arpSync = 1; // 1 = 1/8 note
    bool cached_polyMode = true;
    
    // Motor do Arpeggiator e controle de tempo
    core_dsp::arpeggiator::Arpeggiator arp;
    double currentArpPhase = 0.0;
    int currentArpNote = -1;
    
    double currentSampleRate = 44100.0;
    
    core_dsp::gain::GainProcessor masterGain;
    
    // Lida com alocação das vozes (MIDI)
    void handleMidi(juce::MidiBuffer& midiMessages);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sh101AudioProcessor)
};
