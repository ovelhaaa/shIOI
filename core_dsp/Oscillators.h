#pragma once

#include "AudioTypes.h"
#include <cmath>

namespace core_dsp::oscillator
{
    struct Phase
    {
        void resetPhase() { phase = 0; }
        void setSampleRate(float sampleRate);
        void setFrequency(SampleType frequency);

        uint32_t next();
        uint32_t getCurrentPhase() const { return phase; }

        uint32_t phase = 0;
        uint32_t increment = 0;
        SampleType freqToIncrementFactor = SAMPLE_ZERO;
    };

    struct Saw
    {
        void resetPhase() { phase.resetPhase(); }
        void setSampleRate(float sampleRate) { phase.setSampleRate(sampleRate); }
        void setFrequency(SampleType frequency) { phase.setFrequency(frequency); }

        SampleType getSample();

    private:
        Phase phase;
    };

    struct Pulse
    {
        void resetPhase() { phase.resetPhase(); }
        void setSampleRate(float sampleRate) { phase.setSampleRate(sampleRate); }
        void setFrequency(SampleType frequency) { phase.setFrequency(frequency); }
        void setPulseWidth(SampleType width) { pulseWidth = width; }

        SampleType getSample();

    private:
        Phase phase;
        SampleType pulseWidth = SAMPLE_HALF;  // Default 50% duty cycle
    };

    struct Sub
    {
        void resetPhase() { phase.resetPhase(); }
        void setSampleRate(float sampleRate) { phase.setSampleRate(sampleRate); }
        void setFrequency(SampleType frequency) {
            // Sub oscillator runs at half frequency (one octave down)
            SampleType halfFreq = MultiplySamples(frequency, SAMPLE_HALF);
            phase.setFrequency(halfFreq);
        }

        SampleType getSample();

    private:
        Phase phase;
    };

    struct Noise
    {
        SampleType getSample();

    private:
        uint32_t seed = 1;
    };

    // General purpose modulation LFO - can be used for PWM, filter, etc.
    struct ModLFO
    {
        void resetPhase() { phase.resetPhase(); }
        void setSampleRate(float sampleRate) { phase.setSampleRate(sampleRate); }
        void setFrequency(SampleType frequency) { phase.setFrequency(frequency); }

        SampleType getTriangle();    // Returns triangle wave -1 to +1
        SampleType getSine();        // Returns sine wave -1 to +1 (approximated)
        SampleType getSquare();      // Returns square wave -1 to +1
        SampleType getSaw();         // Returns sawtooth wave -1 to +1
        
        // Primary interface - returns triangle by default (classic analog LFO)
        SampleType getSample() { return getTriangle(); }

    private:
        Phase phase;
    };

    inline void Phase::setSampleRate(float sampleRate)
    {
        freqToIncrementFactor = CalculateFrequencyFactor(sampleRate);
    }

    inline void Phase::setFrequency(SampleType frequency)
    {
        increment = CalculatePhaseIncrement(frequency, freqToIncrementFactor);
    }

    inline uint32_t Phase::next()
    {
        uint32_t currentPhase = phase;
        phase += increment;
        return currentPhase;
    }

    inline SampleType Saw::getSample()
    {
        uint32_t p = phase.next();
        return PhaseToSaw(p);
    }

    inline SampleType Pulse::getSample()
    {
        uint32_t p = phase.next();
        return PhaseToPulse(p, pulseWidth);
    }

    inline SampleType Sub::getSample()
    {
        uint32_t p = phase.next();
        return PhaseToSquare(p);
    }

    inline SampleType Noise::getSample()
    {
        // Linear Congruential Generator for white noise
        // Using standard constants: a=1664525, c=1013904223
        seed = seed * 1664525U + 1013904223U;

        // Convert upper 16 bits to range (-1 to +1)
        int16_t noise_sample = (int16_t)(seed >> 16);

#ifdef PLATFORM_PICO
        return (SampleType)noise_sample;
#else
        return (SampleType)noise_sample / 32768.0f;
#endif
    }

    inline SampleType ModLFO::getTriangle()
    {
        uint32_t p = phase.next();
        return PhaseToTriangle(p);
    }

    inline SampleType ModLFO::getSine()
    {
        // Simple sine approximation using triangle wave with shaping
        SampleType tri = getTriangle();
        
        SampleType tri_abs = (tri < SAMPLE_ZERO) ? -tri : tri;
        SampleType tri_cubed = MultiplySamples(MultiplySamples(tri, tri_abs), tri);

#ifdef PLATFORM_PICO
        // Preserve original exact math with potential precedence behavior
        return tri - tri_cubed >> 2;
#else
        // For float, replicate the integer math exactly by doing bitwise shift emulation
        // The original `tri - tri_cubed >> 2` means `(tri - tri_cubed) >> 2` because `-` has higher precedence than `>>`.
        // Wait, let's replicate exactly the float representation of that fix15 math.
        // A fix15 shift right by 2 is division by 4.
        return (tri - tri_cubed) * 0.25f;
#endif
    }

    inline SampleType ModLFO::getSquare()
    {
        uint32_t p = phase.getCurrentPhase();
        return PhaseToSquare(p);
    }

    inline SampleType ModLFO::getSaw()
    {
        uint32_t p = phase.getCurrentPhase();
        return PhaseToSaw(p);
    }

} // namespace core_dsp::oscillator
