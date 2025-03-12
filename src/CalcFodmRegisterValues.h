#ifndef CALC_FODM_REG_VALUES_H
#define CALC_FODM_REG_VALUES_H

#include <cstdint>
#include <cmath>

#include "DelayModelStore.h"

namespace ska_mid_cbf_fodm_gen
{

// The version 2+ First Order Delay Models register
struct FirstOrderDelayModelRegisterValues
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

// The version 1 First Order Delay Models register.
// There are only 32bit resolution for delay linear and phase linear
struct FirstOrderDelayModelRegisterValuesVer1
{
    uint64_t first_input_timestamp;
    uint32_t delay_constant;
    int32_t phase_constant;
    uint32_t delay_linear;
    int32_t phase_linear;
    uint32_t validity_period;
    uint32_t output_PPS;
    uint64_t first_output_timestamp;
};

// Calculates the FODM register values for
// register version 2 and higher.
FirstOrderDelayModelRegisterValues CalcFodmRegisterValues( 
    const FoPoly &fo_poly,
    uint32_t input_sample_rate,
    uint32_t output_sample_rate,
    double freq_down_shift,
    double freq_align_shift,
    double freq_wb_shift,
    double freq_scfo_shift );

// Calculates the FODM register values for
// register version 1.
FirstOrderDelayModelRegisterValuesVer1 CalcFodmRegisterValuesV1(
    const FoPoly &fo_poly,
    uint32_t input_sample_rate,
    uint32_t output_sample_rate,
    double freq_down_shift,
    double freq_align_shift,
    double freq_wb_shift,
    double freq_scfo_shift );

// Used to convert floating point values to integer values.
template <typename T, typename U>
T ToInt(U val, U scale)
{
    return static_cast<T>(round(val * scale));
}

}; // namespace ska_mid_cbf_fodm_gen


#endif