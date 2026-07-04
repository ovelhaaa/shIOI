#pragma once

#include "AudioTypes.h"
#include <algorithm>

namespace core_dsp::envelope
{
    enum class State { Idle, Attack, Decay, Sustain, Release, StealFade };

    class ADSR {
    public:
        ADSR(float sampleRate) : sampleRate(sampleRate) {
            stealFadeSamples = (uint32_t)(0.005f * sampleRate);
            updateSampleCounts();
        }
        
        ADSR() : sampleRate(44100.0f) {
            stealFadeSamples = 220; // 5ms at 44.1kHz
        }

        void init(float newSampleRate) {
            sampleRate = newSampleRate;
            stealFadeSamples = (uint32_t)(0.005f * sampleRate);
            updateSampleCounts();
        }

        void noteOn() {
            if (currentLevel > SAMPLE_ZERO) {
                state = State::StealFade;
                stealFadeStartLevel = currentLevel;
                sampleCounter = 0;
            } else {
                currentLevel = SAMPLE_ZERO;
                state = State::Attack;
                sampleCounter = 0;
            }
        }

        void noteOff() {
            if (state != State::Idle) {
                releaseStartLevel = currentLevel;
                state = State::Release;
                sampleCounter = 0;
            }
        }

        bool isActive() const { return state != State::Idle; }
        State getState() const { return state; }

        void setAttackTime(float seconds) {
            attackTimeSeconds = std::max(0.001f, seconds);
            attackSamples = (uint32_t)(attackTimeSeconds * sampleRate);
        }

        void setDecayTime(float seconds) {
            decayTimeSeconds = std::max(0.001f, seconds);
            decaySamples = (uint32_t)(decayTimeSeconds * sampleRate);
        }

        void setSustainLevel(SampleType level) {
            sustainLevel = level;
        }

        void setReleaseTime(float seconds) {
            releaseTimeSeconds = std::max(0.001f, seconds);
            releaseSamples = (uint32_t)(releaseTimeSeconds * sampleRate);
        }

        SampleType getNextValue(uint32_t currentAttackSamples, uint32_t currentDecaySamples, uint32_t currentReleaseSamples, SampleType currentSustainLevel) {
            
            switch (state) {
                case State::StealFade:
                    if (stealFadeSamples > 0) {
                        SampleType progress;
                        if (sampleCounter >= stealFadeSamples) {
                            progress = SAMPLE_ONE;
                        } else {
                            #ifdef PLATFORM_PICO
                            progress = (SampleType)(((uint64_t)sampleCounter << 15) / stealFadeSamples);
                            #else
                            progress = (float)sampleCounter / (float)stealFadeSamples;
                            #endif
                        }
                        
                        SampleType fade_factor = SAMPLE_ONE - progress;
                        currentLevel = MultiplySamples(stealFadeStartLevel, fade_factor);
                        
                        sampleCounter++;
                        if (sampleCounter >= stealFadeSamples) {
                            currentLevel = SAMPLE_ZERO;
                            state = State::Attack;
                            sampleCounter = 0;
                        }
                    } else {
                        currentLevel = SAMPLE_ZERO;
                        state = State::Attack;
                        sampleCounter = 0;
                    }
                    break;
                    
                case State::Attack:
                    if (currentAttackSamples > 0) {
                        SampleType progress;
                        if (sampleCounter >= currentAttackSamples) {
                            progress = SAMPLE_ONE;
                        } else {
                            #ifdef PLATFORM_PICO
                            progress = (SampleType)(((uint64_t)sampleCounter << 15) / currentAttackSamples);
                            #else
                            progress = (float)sampleCounter / (float)currentAttackSamples;
                            #endif
                        }
                        
                        if (progress > SAMPLE_ONE) {
                            if (currentLevel < SAMPLE_ONE) {
                                #ifdef PLATFORM_PICO
                                sampleCounter = (uint32_t)((uint64_t)currentLevel * currentAttackSamples >> 15);
                                #else
                                sampleCounter = (uint32_t)(currentLevel * currentAttackSamples);
                                #endif
                            }
                            progress = currentLevel; 
                        }
                        currentLevel = progress; 
                        
                        sampleCounter++;
                        if (sampleCounter >= currentAttackSamples) {
                            currentLevel = SAMPLE_ONE;
                            state = State::Decay;
                            sampleCounter = 0; 
                        }
                    } else {
                        currentLevel = SAMPLE_ONE;
                        state = State::Decay;
                        sampleCounter = 0;
                    }
                    break;
                    
                case State::Decay:
                    if (currentDecaySamples > 0) {
                        SampleType progress;
                        if (sampleCounter >= currentDecaySamples) {
                            progress = SAMPLE_ONE;
                        } else {
                            #ifdef PLATFORM_PICO
                            progress = (SampleType)(((uint64_t)sampleCounter << 15) / currentDecaySamples);
                            #else
                            progress = (float)sampleCounter / (float)currentDecaySamples;
                            #endif
                        }
                        
                        if (progress > SAMPLE_ONE) {
                            SampleType decay_range = SAMPLE_ONE - currentSustainLevel;
                            if (decay_range > SAMPLE_ZERO && currentLevel > currentSustainLevel && currentLevel <= SAMPLE_ONE) {
                                #ifdef PLATFORM_PICO
                                uint32_t reverse_progress = ((uint32_t)(SAMPLE_ONE - currentLevel) << 15) / decay_range;
                                if (reverse_progress <= 32768) {
                                    sampleCounter = (uint32_t)((uint64_t)reverse_progress * currentDecaySamples >> 15);
                                }
                                #else
                                float reverse_progress = (SAMPLE_ONE - currentLevel) / decay_range;
                                if (reverse_progress <= 1.0f) {
                                    sampleCounter = (uint32_t)(reverse_progress * currentDecaySamples);
                                }
                                #endif
                            }
                        } else {
                            SampleType decay_range = SAMPLE_ONE - currentSustainLevel;
                            SampleType decay_amount = MultiplySamples(progress, decay_range);
                            currentLevel = SAMPLE_ONE - decay_amount;
                        }
                        
                        sampleCounter++;
                        if (sampleCounter >= currentDecaySamples) {
                            currentLevel = currentSustainLevel;
                            state = State::Sustain;
                        }
                    } else {
                        currentLevel = currentSustainLevel;
                        state = State::Sustain;
                    }
                    break;
                    
                case State::Sustain:
                    currentLevel = currentSustainLevel;
                    if (currentSustainLevel == SAMPLE_ZERO) {
                        currentLevel = SAMPLE_ZERO;
                    }
                    break;
                    
                case State::Release:
                    if (currentReleaseSamples > 0) {
                        SampleType progress;
                        if (sampleCounter >= currentReleaseSamples) {
                            progress = SAMPLE_ONE;
                        } else {
                            #ifdef PLATFORM_PICO
                            progress = (SampleType)(((uint64_t)sampleCounter << 15) / currentReleaseSamples);
                            #else
                            progress = (float)sampleCounter / (float)currentReleaseSamples;
                            #endif
                        }
                        
                        if (progress > SAMPLE_ONE) {
                            if (releaseStartLevel > SAMPLE_ZERO && currentLevel >= SAMPLE_ZERO && currentLevel <= releaseStartLevel) {
                                #ifdef PLATFORM_PICO
                                uint32_t level_ratio = ((uint32_t)currentLevel << 15) / releaseStartLevel;
                                if (level_ratio <= 32768) {
                                    uint32_t reverse_progress = 32768 - level_ratio;
                                    sampleCounter = (uint32_t)((uint64_t)reverse_progress * currentReleaseSamples >> 15);
                                }
                                #else
                                float level_ratio = currentLevel / releaseStartLevel;
                                if (level_ratio <= 1.0f) {
                                    float reverse_progress = 1.0f - level_ratio;
                                    sampleCounter = (uint32_t)(reverse_progress * currentReleaseSamples);
                                }
                                #endif
                            }
                        } else {
                            SampleType fade_factor = SAMPLE_ONE - progress;
                            currentLevel = MultiplySamples(releaseStartLevel, fade_factor);
                        }
                        
                        sampleCounter++;
                        if (sampleCounter >= currentReleaseSamples) {
                            currentLevel = SAMPLE_ZERO;
                            state = State::Idle;
                            sampleCounter = 0; 
                        }
                    } else {
                        currentLevel = SAMPLE_ZERO;
                        state = State::Idle;
                        sampleCounter = 0; 
                    }
                    break;
                    
                case State::Idle:
                default:
                    currentLevel = SAMPLE_ZERO;
                    break;
            }

            return currentLevel;
        }

        // Convenience wrapper if no smoothers are used externally
        SampleType getNextValue() {
            return getNextValue(attackSamples, decaySamples, releaseSamples, sustainLevel);
        }

    private:
        void updateSampleCounts() {
            attackSamples = (uint32_t)(attackTimeSeconds * sampleRate);
            decaySamples = (uint32_t)(decayTimeSeconds * sampleRate);
            releaseSamples = (uint32_t)(releaseTimeSeconds * sampleRate);
        }

        float sampleRate;
        State state = State::Idle;
        SampleType currentLevel = SAMPLE_ZERO;
        SampleType sustainLevel = SAMPLE_HALF;
        
        uint32_t sampleCounter = 0;       
        uint32_t attackSamples = 441;     
        uint32_t decaySamples = 8820;      
        uint32_t releaseSamples = 22050;   
        SampleType releaseStartLevel = SAMPLE_ZERO; 
        
        uint32_t stealFadeSamples = 220;       
        SampleType stealFadeStartLevel = SAMPLE_ZERO; 
        
        float attackTimeSeconds = 0.01f;
        float decayTimeSeconds = 0.2f;
        float releaseTimeSeconds = 0.5f;
    };
}
