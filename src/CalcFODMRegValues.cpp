#include "CalcFODMRegValues.h"

// to support higher precision
#include "mpreal.h"

// // TODO: REMOVE
// #include <iostream>
// #include <iomanip>

namespace ska_mid_cbf_fodm_gen
{

using mpfr::mpreal;

/**
 * Uncomment to not apply the delay_linear_error_samples to delay_constant
 */
// #define DISABLE_DELAY_LINEAR_ERROR

mpreal NS_TO_SECONDS(mpreal ns) {
  return ns / mpreal(1000000000);
}

mpreal MS_TO_SECONDS(mpreal ms) {
  return ms / mpreal(1000);
}

mpreal mod_pmhalf(mpreal val)
{
  // Python version:
  // (((val % Decimal(1)) + Decimal(1.5)) % Decimal(1)) - Decimal(0.5)
  return fmod((fmod(val,mpreal(1)) + mpreal(1.5)),mpreal(1)) - mpreal(0.5);
}


/**
 * Write a first order delay model to the FPGA registers.
 *
 * fo_poly: FO delay model to write
 * input_sample_rate: Input sample rate in samples/second
 * output_sample_rate: Output sample rate in samples/second
 * max_delay_jump_samples: Maximum discontinuity in delay between FO models in samples
 * freq_down_shift: Frequency down-shift at the VCC-OSPPFB [Hz]
 * freq_align_shift: Frequency shift applied to align fine channels between FSs [Hz]
 * freq_wb_shift: Net Wideband (WB) frequency shift [Hz] 
 * freq_scfo_shift: Frequency shift required due to SCFO sampling [Hz]
 * 
 * Description: 
 * Reference [R1]: Derivation of First Order Delay/Phase Polynomials for the
 *                 Mid.CBF ReSampler, by Thushara Gunaratne, Ver.1.0-2021-07-15 
 *
 */
FirstOrderDelayModelsRegisterSet CalcFodmRegValues( 
    const FoPoly &fo_poly,
    uint32_t input_sample_rate,
    uint32_t output_sample_rate,
    double freq_down_shift,
    double freq_align_shift,
    double freq_wb_shift,
    double freq_scfo_shift )
{
  // Note: fo_poly defines the time delay D(.) to be applied to the signal data 
  // as a linear function of time, D(t) = a * t + b, t in [Tk, Tk+1),  where:
  // fo_poly.poly[0]  = a, units: [ns/s] (a = dD, the rate of change of D(.))  
  // fo_poly.poly[1]  = b, units: [ns]
  // fo_poly.start_ts = Tk, in milliseconds, measured from the SKA Epoch
  // fo_poly.stop_ts  = Tk+1

  //  SKA_epoch
  //    0              ho_start 
  //    |                 |
  // ---|-----------------|--------|--------|--------|--------|--------|------
  //    ^                 ^        ^        ^        ^
  //    |                T0       T1       T2       T3  ....
  // 
  //  ho_start = the start time of the high-order (HO) Delay Model (DM) 
  //             polynomial from which f0_poly has been derived, measured 
  //             relative to the SKA_epoch
  //  Tk       = the start of the validity period for the current polynomial 
  //             (the (k+1)-th time this method was entered)
  //             Note: typically T0 = ho_start, but in general it does not need 
  //             to be; however, there might be an implicit assumption that 
  //             T0 = ho_start.
  mpreal::set_default_prec(mpfr::digits2bits(50)); // 50 digits precision

  // FO polynomial start time, measured from the start of the HO poly from which this FO has been derived (in seconds):
  mpreal ho_start_ts_s = MS_TO_SECONDS(fo_poly.ho_poly_start_time_ms);
  
  // FO polynomial start/stop time, measured from the SKA epoch
  mpreal start_ts_s  = MS_TO_SECONDS(fo_poly.start_time_ms);
  mpreal stop_ts_s   = MS_TO_SECONDS(fo_poly.stop_time_ms);

  // Renaming, for readability:
  mpreal fo_delay_linear   = NS_TO_SECONDS(fo_poly.poly[0]); // nondimensional
  mpreal fo_delay_constant = NS_TO_SECONDS(fo_poly.poly[1]); // [s]

  // Calculate the 'double' version of the FPGA register fields
  // delay_linear and delay_constant (measured in samples):

  // Note correction of delay_linear w.r.t. the previous version, to agree with
  // the json file definition:
  mpreal input_sample_rate_f(input_sample_rate);
  mpreal output_sample_rate_f(output_sample_rate);
  mpreal resampling_rate = input_sample_rate_f / output_sample_rate_f;
  mpreal delay_linear   = resampling_rate * (fo_delay_linear + 1);

  // Calculate delay_linear_scaled here, since it is required in the
  // delay_linear_error_samples calculation below:
  // Need to be cast to int before writing to the register
  // using boost::math::round;
  mpreal delay_linear_scaled = round(delay_linear * pow(2, 63));

  // Calculate output_timestamp_samples_f,  used to populate the 
  // first_output_timestamp FPGA register field:
  mpreal output_timestamp_samples_f = output_sample_rate_f * start_ts_s; 

  // the output sample closest to the FO poly start time
  mpreal current_output_timestamp_samples = floor(output_timestamp_samples_f);

  mpreal next_output_timestamp_samples = mpreal(floor(stop_ts_s * output_sample_rate_f));

  // FO validity interval (measured in output samples)
  // Note: implicit assumption that the FO polynomial validity intervals do
  //       not overlap; currently this assumption holds true. (Otherwise, the 
  //       definition of the validity interval FPGA register would imply that 
  //       samples in the overlap would be output twice.)
  mpreal  validity_interval_samples = 
    next_output_timestamp_samples - current_output_timestamp_samples;

  // As an extra refinement, remove a v. small amount of delay, so that to hit 
  // zero resampling error in the middle of the FODM instead of only at the end
  // giving an overall positive delay error.
  mpreal delay_linear_unscaled = delay_linear_scaled * mpreal(pow(2,-63));
  mpreal delay_linear_error_samples = 
    delay_linear_unscaled * validity_interval_samples - delay_linear * validity_interval_samples;
  // mpreal delay_linear_error_samples = 
  //   (delay_linear_scaled * mpreal(pow(2,-63)) - delay_linear) * validity_interval_samples;
 
  #ifdef DISABLE_DELAY_LINEAR_ERROR
    mpreal delay_constant_input_samps =  fo_delay_constant * input_sample_rate_f;
  #else
      // Apply  /2 to fo_delay_constant (in samps):
      mpreal delay_constant_input_samps = 
        fo_delay_constant * input_sample_rate_f - delay_linear_error_samples / 2;
  #endif


  // Calculate the integer and fractional part of first_input_timestamp_fractional_samples;
  // (See the definitions of the first_input_timestamp and delay_constant fields 
  // in the FPGA JSON interface file first_order_delay_models.json.) 
  // This calculation was updated to align with talon_FSP.py in the HW notebooks. 
  mpreal current_input_timestamp_samples = resampling_rate * current_output_timestamp_samples;
  mpreal first_input_timestamp_fractional_samples = current_input_timestamp_samples + delay_constant_input_samps;

  uint64_t first_input_timestamp_samples_int  = static_cast<uint64_t>(first_input_timestamp_fractional_samples);
  mpreal delay_constant = first_input_timestamp_fractional_samples - first_input_timestamp_samples_int;

  mpreal delay_constant_scaled = round(delay_constant * mpreal(pow(2, 32)));

  // -------------------------------------------------------------------------
  // Initialize parameters for First Order Phase Polynomials (FOPP) calculation

  // Mapping between the C++ variable names and the [R1] notations: 
  // fs_index         = FSI
  // freq_down_shift  = F_DS
  // freq_wb_shift    = F_WB
  // freq_align_shift = F_AS
  // freq_scfo_shift  = F_SCFO
  // vcc_os_factor    = OS

  // temporary flag; TODO remove when confirmed:
  bool use_tech_note_formula = false; 
  if (use_tech_note_formula)
  {
    // In the HW test notebooks (talon_FSP.py) freq_down_shift has opposite sign
    // This may have to do with the selected sign convention. in [R1] it was 
    // assumed F_DS is positive when decreased (i.e. down-shift), 
    // whereas Wideband-Shift (F_WB) is positive when increased. 
    freq_down_shift = -freq_down_shift;
  }

  // -----------------------------------------------------------------------
  
  // Calculate phase_linear_temp and phase_constant_temp of the FOPP 
  // ([R1] eq. 4, 5);
  // Note that the 2*PI factor from R1 eq. 4, 5 is not applied here, nor the 
  // mod(*, 2*PI) for phase_constant:
  mpreal f_ds_offset = mpreal(freq_wb_shift - freq_down_shift);
  mpreal f_ds_delay_linear_temp = f_ds_offset * fo_delay_linear;
  mpreal phase_linear_temp = 
    (mpreal(freq_scfo_shift + freq_align_shift) + 
    f_ds_offset * fo_delay_linear) / output_sample_rate_f;

  // Calculate the time_factor, as per  [R1] eq. 5:
  // time_factor = k * T1 where T1 is the validity period of the FO (= 10 ms),
  // where k >= 0, is measured relative to the start time of the HO polynomial 
  // from which that FO delay poly was derived;
  
  // Note: current implementation tries to match how time_factor is 
  // defined in talon_FSP.py, which is output samples relative to the SKA epoch. 
  // After some testing, using this definition appears to give the best
  // correlation result when 0 delay coefficients are used in HODMs. Most 
  // notably the phase in cross-correlation is much closer to 0 than 
  // when time_factor is set relative to the HODM start time. See CIP-3148.
  //
  // TODO: it is unclear why time factor should be relative to the SKA epoch, further 
  // investigation may be needed. 
  //
  mpreal time_factor =  start_ts_s; 

  bool use_tech_note_kT1_defn = false;
  if (use_tech_note_kT1_defn) {
    time_factor = start_ts_s - ho_start_ts_s;
  }

  time_factor = floor(time_factor * output_sample_rate_f);
  
  // Calculate phase_constant_temp (see [R1] eq. 5):
  mpreal phase_constant_temp = 
    time_factor * phase_linear_temp + 
    f_ds_offset * fo_delay_constant;
  
  // Take mod of phase_linear_temp and phase_constant_temp to get to the final value
  mpreal phase_linear   = mod_pmhalf(phase_linear_temp);
  mpreal phase_constant = mod_pmhalf(phase_constant_temp); 

  int64_t phase_linear_scaled = static_cast<int64_t>(round(phase_linear * pow(2, 63)));
  int32_t phase_constant_scaled = static_cast<int32_t>(round(phase_constant * pow(2, 31)));

  // std::cout << std::setprecision(26) 
  //   << "start_ts_s = " << start_ts_s << std::endl
  //   << "stop_ts_s = " << stop_ts_s << std::endl
  //   << "fo_delay_linear = " << fo_delay_linear << std::endl
  //   << "fo_delay_constant = " << fo_delay_constant << std::endl
  //   << "delay_linear = " << delay_linear << std::endl
  //   << "delay_constant = " << delay_constant << std::endl
  //   << "current_output_timestamp_samples = " << current_output_timestamp_samples << std::endl
  //   << "next_output_timestamp_samples = " << next_output_timestamp_samples << std::endl
  //   << "validity_interval_samples = " << validity_interval_samples << std::endl
  //   << "delay_linear_scaled = " << delay_linear_scaled << std::endl
  //   << "delay_linear_unscaled = " << delay_linear_unscaled << std::endl
  //   << "delay_linear_error_samples = " << delay_linear_error_samples << std::endl
  //   << "f_ds_delay_linear_temp = " << f_ds_delay_linear_temp << std::endl
  //   << "phase_linear_temp = " << phase_linear_temp << std::endl
  //   << "phase_constant_temp = " << phase_constant_temp << std::endl 
  //   << "phase_linear_mod = " << phase_linear << std::endl
  //   << "phase_constant_mod = " << phase_constant << std::endl 
  //   << "-------" << std::endl;
    

  // -------------------------------------------------------------------------

  FirstOrderDelayModelsRegisterSet fodm_reg_values;

  // First input timestamp. Need to add back the offset in input samples.
  //   buf.last_fo_timestamp_in_buffer = first_input_timestamp_samples_int;
  fodm_reg_values.first_input_timestamp = first_input_timestamp_samples_int;

  // Delay constant fraction
  fodm_reg_values.delay_constant = static_cast<uint32_t>(delay_constant_scaled);

  // Linear delay ratio
  fodm_reg_values.delay_linear = (uint64_t)delay_linear_scaled;

  // Constant part of the FO phase polynomial
  fodm_reg_values.phase_constant = phase_constant_scaled;

  // Linear part of the FO phase polynomial
  fodm_reg_values.phase_linear = phase_linear_scaled;

  // Fill in as per FPGA register definition: "The number of output samples 
  // that should be output for this FODM less 1"
  fodm_reg_values.validity_period = static_cast<uint32_t>(validity_interval_samples) - 1;

  // PPS
  // Note on casting the output_pps_samples:
  // Needs the 64bit casting intermediate step with calculating output pps as 
  // there are some issue casting double generated by ceil(start_ts_s) * output_sample_rate)
  // directly to a uint32_t. It seems to become an unsigned interger max instead of
  // the lower 32bits of the double.
  uint64_t output_pps_samples_64bit = 
    static_cast<uint64_t> (ceil(current_output_timestamp_samples / output_sample_rate) * 
                                  output_sample_rate);
  
  fodm_reg_values.output_PPS = static_cast<uint32_t> (output_pps_samples_64bit & 0xffffffff);

  // First output timestamp
  fodm_reg_values.first_output_timestamp = static_cast<uint64_t>(current_output_timestamp_samples);

  return fodm_reg_values;
}

}; // namespace ska_mid_cbf_fodm_gen