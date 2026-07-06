#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

// Inclui os módulos do core_dsp 
// (os caminhos relativos ou incluídos via target_include_directories pelo CMake)
#include "../core_dsp/Oscillators.h"
#include "../core_dsp/Filters.h"
#include "../core_dsp/Envelope.h"
#include "../core_dsp/Gain.h"

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
    
    juce::AudioProcessorEditor* createEditor() override { return nullptr; } // Retorna nulo se usarmos UI genérica por agora
    bool hasEditor() const override { return false; }

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
        
        // Smoothers nativos do JUCE para a velocidade
        juce::LinearSmoothedValue<float> smoothedVelocity {0.0f};

        void init(float sampleRate) {
            sawOsc.setSampleRate(sampleRate);
            pulseOsc.setSampleRate(sampleRate);
            subOsc.setSampleRate(sampleRate);
            envelope.init(sampleRate);
            smoothedVelocity.reset(sampleRate, 0.005);
            smoothedVelocity.setCurrentAndTargetValue(0.0f);
        }
        
        void noteOn(int note, float vel, float freq) {
            midiNote = note;
            isActive = true;
            velocity = vel;
            smoothedVelocity.setTargetValue(vel);
            
            sawOsc.resetPhase();
            pulseOsc.resetPhase();
            subOsc.resetPhase();
            
            sawOsc.setFrequency(freq);
            pulseOsc.setFrequency(freq);
            subOsc.setFrequency(freq * 0.5f);
            
            envelope.noteOn();
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
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Smoothers para prevenir Clicks e Zipper Noise
    juce::LinearSmoothedValue<float> s_cutoff {0.5f};
    juce::LinearSmoothedValue<float> s_resonance {0.2f};
    juce::LinearSmoothedValue<float> s_masterVol {0.8f};
    
    core_dsp::gain::GainProcessor masterGain;
    
    // Lida com alocação das vozes (MIDI)
    void handleMidi(juce::MidiBuffer& midiMessages);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sh101AudioProcessor)
};
