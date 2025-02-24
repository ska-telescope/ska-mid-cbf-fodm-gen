#ifndef CALC_FODM_REG_VALUES_H
#define CALC_FODM_REG_VALUES_H

#include <cstdint>

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



}; // namespace ska_mid_cbf_fodm_gen


#endif