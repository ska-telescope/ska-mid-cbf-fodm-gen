#ifndef DELAY_MODEL_STORE_H
#define DELAY_MODEL_STORE_H

namespace ska_mid_cbf_fodm_gen
{

struct FoPoly 
{
	double ho_poly_start_time_ms;
	double start_time_ms;
	double stop_time_ms;
	double poly[2];     // Units: ns/s^(num_coeffs - index - 1)

    inline double delay_const() { return poly[1]; }
    inline double delay_linear() { return poly[0]; }
};


}; // namespace ska_mid_cbf_fodm_gen

#endif