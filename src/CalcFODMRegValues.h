#ifndef CALC_FODM_REG_VALUES_H
#define CALC_FODM_REG_VALUES_H

#include <cstdint>

#include "DelayModelStore.h"

namespace ska_mid_cbf_fodm_gen
{

struct FirstOrderDelayModelsRegisterSet
{
    uint64_t first_input_timestamp;
    uint32_t delay_constant;
    int32_t phase_constant;
    uint64_t delay_linear;
    int64_t phase_linear;
    uint32_t validity_period;
    uint32_t output_PPS;
    uint64_t first_output_timestamp;
};

FirstOrderDelayModelsRegisterSet CalcFodmRegValues( 
    const FoPoly &fo_poly,
    uint32_t input_sample_rate,
    uint32_t output_sample_rate,
    double freq_down_shift,
    double freq_align_shift,
    double freq_wb_shift,
    double freq_scfo_shift );

}; // namespace ska_mid_cbf_fodm_gen


#endif