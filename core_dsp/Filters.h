#pragma once
#include "AudioTypes.h"

namespace core_dsp::filter
{
    class MoogLadder {
    public:
        MoogLadder() { stage1 = stage2 = stage3 = stage4 = SAMPLE_ZERO; }
        
        SampleType process(SampleType input, SampleType cutoff_normalized, SampleType res_normalized) {
            SampleType g = MapCutoff(cutoff_normalized);
            SampleType res = MapResonance(res_normalized);
            
            SampleType fb_input = input - MultiplySamples(res, stage4);
            fb_input = ClampFeedback(fb_input);
            
            stage1 = stage1 + MultiplySamples(g, fb_input - stage1);
            stage2 = stage2 + MultiplySamples(g, stage1 - stage2);
            stage3 = stage3 + MultiplySamples(g, stage2 - stage3);
            stage4 = stage4 + MultiplySamples(g, stage3 - stage4);
            
            // Makeup gain (approx 2.25x base + resonance compensation)
            SampleType s4_half = MultiplySamples(stage4, SAMPLE_HALF);
            SampleType output = stage4 + s4_half;
            
            SampleType output_half = MultiplySamples(output, SAMPLE_HALF);
            output = output + output_half;
            
            SampleType resonance_compensation = MultiplySamples(res_normalized, output_half);
            return output + resonance_compensation;
        }
        
    private:
        SampleType stage1, stage2, stage3, stage4;
    };
}
