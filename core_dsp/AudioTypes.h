#pragma once
#include <cstdint>

#ifdef PLATFORM_PICO
    #include "../Fix15.h"
    
    using SampleType = fix15;
    
    constexpr SampleType SAMPLE_ZERO = FIX15_ZERO;
    constexpr SampleType SAMPLE_ONE  = FIX15_ONE;
    constexpr SampleType SAMPLE_HALF = FIX15_HALF;
    
    inline SampleType MultiplySamples(SampleType a, SampleType b) { return multfix15(a, b); }
    inline SampleType FloatToSample(float a) { return float2fix15(a); }
    
    // Waveform extraction
    inline SampleType PhaseToSaw(uint32_t phase) { 
        return (SampleType)(int16_t)(phase >> 16); 
    }
    
    inline SampleType PhaseToSquare(uint32_t phase) { 
        return (phase & 0x80000000) ? -SAMPLE_ONE : SAMPLE_ONE; 
    }
    
    inline SampleType PhaseToPulse(uint32_t phase, SampleType width) {
        uint32_t threshold = (uint32_t)((uint64_t)width << 17);
        return (phase < threshold) ? SAMPLE_ONE : -SAMPLE_ONE;
    }
    
    inline SampleType PhaseToTriangle(uint32_t phase) {
        if (phase < 0x80000000U) {
            return (SampleType)((int32_t)(phase >> 15) - SAMPLE_ONE);
        } else {
            return (SampleType)(SAMPLE_ONE - (int32_t)((phase - 0x80000000U) >> 15));
        }
    }

    // Phase increment calculation helpers
    inline SampleType CalculateFrequencyFactor(float sampleRate) {
        double increment_per_hz = 4294967296.0 / sampleRate;
        double factorDouble = increment_per_hz / 32768.0;
        return float2fix15(factorDouble);
    }
    inline uint32_t CalculatePhaseIncrement(SampleType frequency, SampleType factor) {
        return (uint32_t)multfix15(frequency, factor);
    }

    // Filter normalization helpers
    inline SampleType MapResonance(SampleType q_normalized) {
        // Maps 0.0-1.0 (0-32768) to 0.0-4.5 (147456)
        return multfix15(q_normalized, 147456);
    }
    inline SampleType MapCutoff(SampleType cutoff_normalized) {
        // Maps 0-1 to 0.001-0.85 (33 to 27787)
        return multfix15(cutoff_normalized, 27787) + 33;
    }
    inline SampleType ClampFeedback(SampleType fb) {
        if (fb > FIX15_ONE) return FIX15_ONE;
        if (fb < -FIX15_ONE) return -FIX15_ONE;
        return fb;
    }

#else
    using SampleType = float;
    
    constexpr SampleType SAMPLE_ZERO = 0.0f;
    constexpr SampleType SAMPLE_ONE  = 1.0f;
    constexpr SampleType SAMPLE_HALF = 0.5f;
    
    inline SampleType MultiplySamples(SampleType a, SampleType b) { return a * b; }
    inline SampleType FloatToSample(float a) { return a; }
    
    // Waveform extraction preserving 16-bit integer character
    inline SampleType PhaseToSaw(uint32_t phase) { 
        return (SampleType)(int16_t)(phase >> 16) / 32768.0f; 
    }
    
    inline SampleType PhaseToSquare(uint32_t phase) { 
        return (phase & 0x80000000) ? -SAMPLE_ONE : SAMPLE_ONE; 
    }
    
    inline SampleType PhaseToPulse(uint32_t phase, SampleType width) {
        uint32_t threshold = (uint32_t)((double)width * 4294967296.0);
        return (phase < threshold) ? SAMPLE_ONE : -SAMPLE_ONE;
    }
    
    inline SampleType PhaseToTriangle(uint32_t phase) {
        if (phase < 0x80000000U) {
            return (SampleType)((int32_t)(phase >> 15) - 32768) / 32768.0f;
        } else {
            return (SampleType)(32768 - (int32_t)((phase - 0x80000000U) >> 15)) / 32768.0f;
        }
    }

    // Phase increment calculation helpers
    inline SampleType CalculateFrequencyFactor(float sampleRate) {
        double increment_per_hz = 4294967296.0 / sampleRate;
        return (SampleType)increment_per_hz;
    }
    inline uint32_t CalculatePhaseIncrement(SampleType frequency, SampleType factor) {
        return (uint32_t)((double)frequency * (double)factor);
    }

    // Filter normalization helpers
    inline SampleType MapResonance(SampleType q_normalized) {
        return q_normalized * 4.5f;
    }
    inline SampleType MapCutoff(SampleType cutoff_normalized) {
        return (cutoff_normalized * 0.848f) + 0.001f;
    }
    inline SampleType ClampFeedback(SampleType fb) {
        if (fb > 1.0f) return 1.0f;
        if (fb < -1.0f) return -1.0f;
        return fb;
    }
#endif
