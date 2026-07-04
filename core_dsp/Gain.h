#pragma once
#include "AudioTypes.h"

namespace core_dsp::gain
{
    class GainProcessor {
    public:
        GainProcessor() = default;
        
        // Aplica o ganho linear a uma amostra.
        // A conversão float/fix15 e curvas logarítmicas podem ser geridas pela 
        // classe wrapper que lerá o ParameterStore.
        inline SampleType process(SampleType input, SampleType gainLevel) {
            if (gainLevel == SAMPLE_ZERO) {
                return SAMPLE_ZERO;
            }
            return MultiplySamples(input, gainLevel);
        }
    };
}
