import math
# import numpy as np
from decimal import Decimal
# class HODM_to_FODM_Iterator:
#     def __init__(
#         self,
#         seconds_per_fodm,
#         high_order_delay_polynomial=(0.0,),
#         start_time=0.0, # hodm_start_time, start_UTC.
#         at_time=0.0,    # start_RDT
#         validity_period=None,
#         repeat_period=None,
#     ) -> None:
#         self.seconds_per_fodm = Decimal(seconds_per_fodm)
#         self.poly = np.poly1d(list(reversed(high_order_delay_polynomial)))
#         self.start_time = Decimal(start_time)
#         self.eval_time = Decimal(at_time) - self.start_time  # time to start of rdt from start of hodm.
#         self.at_time = Decimal(at_time)
#         self.end_time = (
#             None if validity_period is None else validity_period
#         )
#         self.repeat_period = repeat_period

#     def __next__(self):
#         if self.end_time is not None and self.eval_time >= self.end_time:
#             raise StopIteration()
#         if self.repeat_period is not None:
#             fm_time_mod = self.eval_time % self.repeat_period
#             to_time_mod = fm_time_mod + self.seconds_per_fodm
#             if to_time_mod > self.repeat_period:
#                 to_time_mod = self.repeat_period
#         else:
#             fm_time_mod = self.eval_time
#             to_time_mod = fm_time_mod + self.seconds_per_fodm
        
#         fodm_len = to_time_mod - fm_time_mod
#         y0 = Decimal(self.poly(float(fm_time_mod)))
#         y1 = Decimal(self.poly(float(to_time_mod)))
#         m = (y1 - y0) / fodm_len
#         fodm = {
#             "delay_constant": y0,  # seconds
#             "delay_linear": m,  # seconds/second
#             "start_time": self.at_time, # time of application, seconds
#             "end_time" : self.at_time + fodm_len
#         }
#         self.eval_time += fodm_len
#         self.at_time += fodm_len
#         return fodm

#     def __iter__(self):
#         return self

#     def __str__(self) -> str:
#         return f"HODM Generator, poly={self.poly}, delta_t={self.seconds_per_fodm}, t_start={self.start_time}s, t_now={self.eval_time}, t_end={self.end_time}s."


def mod_pmhalf(val):
    """modulo to plus-minus a half [-0.5 to 0.5)"""
    return (((val % Decimal(1)) + Decimal(1.5)) % Decimal(1)) - Decimal(0.5)


def phase_correction(frequency_slice, input_sample_rate, output_sample_rate, wideband_freq_shift, alignment_freq_shift, fodm_data):
    FSI = Decimal(frequency_slice)  # Frequency Slice Index.
    FFS_j = Decimal(input_sample_rate)  # Freq Slice Sample Rate.  samp/sec
    FFS_o = Decimal(output_sample_rate)  # Common sample rate.     samp/sec
    F_DS = -FSI * FFS_j * 9 / 10  # Frequency Down Shift at VCC.    samp/sec
    F_WB = Decimal(
        wideband_freq_shift                                        # Hz
    )  # Wideband shift applied to align a frequency slice to start of a band.
    F_AS = Decimal(
        alignment_freq_shift                                       # Hz
    )  # additional shift to apply to align channels between frequency slices.
    # F_WB = Decimal(0)
    # F_AS = Decimal(0)
    F_SCFO = (
        FSI * (FFS_j - FFS_o) * 9 / 10                             # samp/sec
    )  # Frequency shift due to SCFO sampling

    # SKB-640: After testing with tones inserted by BITE, it's found that
    #          the sign of alignment shift needs to be flipped for
    #          the tones to appear at the expected frequencies.
    F_AS = -F_AS 

    # phase_linear = 2* np.pi * ((F_SCFO + F_AS) + (F_WB - F_DS) * delay_linear)/FFS_0
    # phase_const= np.mod(k * T1Pv * phase_linear + 2 * np.pi * (F_WB - F_DS) * delay_const, 2 * np.pi)
    # Divide through by 2*pi, since registers are -0.5 to 0.5
    # phase_linear = ((F_SCFO + F_AS) + (F_WB - F_DS) * delay_linear)/FFS_0
    # phase_const= np.mod(k * T1Pv * phase_linear + (F_WB - F_DS) * delay_const, 1)
    # Correct for the phase due to the resampling.
    phase_linear = (
        (F_SCFO + F_AS) +                                   #Hz
        (F_WB - F_DS) * fodm_data.get("delay_linear")       #Hz * s/s
    ) / FFS_o                                               # Hz = radians/sample. 
    phase_const = (
        (F_SCFO + F_AS) * fodm_data.get("start_time")
    #     phase_linear * output_timestamp_samples
    ) + (
        # Correct for the phase added by the VCC due to the as yet uncorrected delay.
        (F_WB - F_DS) * fodm_data.get("delay_constant") 
    )
    # return Decimal(0), Decimal(0)
    return mod_pmhalf(phase_const), mod_pmhalf(phase_linear)

def calc_fodm_regs(
    fodms,
    input_sample_rate,
    output_sample_rate,
    frequency_slice,
    start_RDT,
    wideband_freq_shift=0.0,
    alignment_freq_shift=0.0
):

    isr = Decimal(input_sample_rate)
    osr = Decimal(output_sample_rate)
    resampling_rate = isr / osr
    # Take an t0_seconds from which to calculate the FODMs.
    # This minimises the bit-width of the timestamps and avoids some numerical
    # issues with overflow and precision in floats.
    t0_seconds = Decimal(int(start_RDT) )
    t0_input_samples = t0_seconds * isr
    t0_output_samples = t0_seconds * osr
    
    for fodm_data in fodms:
        fodm_vals = {}
        current_fodm_start_time_seconds = fodm_data.get("start_time") - t0_seconds
        current_output_timestamp_samples = Decimal(
            int(current_fodm_start_time_seconds * osr)
        )
        next_fodm_start_time_seconds = fodm_data.get("end_time") - t0_seconds
        next_output_timestamp_samples = Decimal(
            int(next_fodm_start_time_seconds * osr)
        )

        fodm_output_samples = int(
            next_output_timestamp_samples
            - current_output_timestamp_samples
        )  # units of whole output samples
        if not (0.0 < fodm_output_samples <= osr):
            raise ValueError(
                f"fodm_output_samples is a maximum of {osr} (1 second), so can make sure we always output a pps. Got {fodm_output_samples} samples from next_time ({next_output_timestamp_samples}) - start_time ({current_output_timestamp_samples})"
            )

        # calculate the input timestamp based on the output timestamp.
        current_input_timestamp_samples = (
            current_output_timestamp_samples * resampling_rate
        )  # in units of input samples.

        # delay_linear = resampling_rate * (1 + fodm_data.get("delay_linear", 0.0)) # units samples-in per sample-out
        delay_linear = resampling_rate + fodm_data.get("delay_linear", 0.0) # units samples-in per sample-out

        fodm_vals["delay_linear"] = delay_linear

        delay_const_input_samples = (
            fodm_data.get("delay_constant") * isr
        )

        first_input_timestamp_fractional_samples = (
            current_input_timestamp_samples + delay_const_input_samples
        )
        delay_const = first_input_timestamp_fractional_samples - int(
            first_input_timestamp_fractional_samples
        )

        first_input_timestamp_fractional_samples += (
            t0_input_samples
        )
        fodm_vals["first_input_timestamp"] = int(
            first_input_timestamp_fractional_samples
        )

        fodm_vals['delay_constant'] = delay_const

        # Calculate the Phase Correction for the Frequency Slice.
        if frequency_slice is None:  # i.e. BITE
            phase_linear = 0
            phase_const = 0
        else:
            phase_const, phase_linear = phase_correction(
                frequency_slice, 
                input_sample_rate, 
                output_sample_rate, 
                wideband_freq_shift, 
                alignment_freq_shift, 
                fodm_data,
            )

        fodm_vals['phase_constant'] = phase_const
        fodm_vals['phase_linear'] = phase_linear

        assert 0 < fodm_output_samples <= 2**32
        fodm_vals['validity_period'] = fodm_output_samples

        # round up to next second (unless the first output should be the pps).
        output_pps_timestamp = (
            Decimal(math.ceil(current_output_timestamp_samples / osr))
            * osr
        )
        output_pps_timestamp += t0_output_samples
        # important to get output_pps right, otherwise the 16k Channeliser throws a hissy-fit.
        fodm_vals['output_PPS'] = int(output_pps_timestamp)
        first_output_timestamp_samples = (
            current_output_timestamp_samples
            + t0_output_samples
        )
        fodm_vals['first_output_timestamp'] = int(
            first_output_timestamp_samples
        )

        current_output_timestamp_samples = (
            current_output_timestamp_samples + fodm_output_samples
        )


        # print(f"start_ts_s = {fodm_data.get('start_time')}")
        # print(f"stop_ts_s = {fodm_data.get('end_time')}")
        # print(f"fo_delay_linear = {fodm_data.get('delay_linear')}")
        # print(f"fo_delay_constant = {fodm_data.get('delay_constant')}")
        # print(f"delay_linear = {delay_linear}")
        # print(f"delay_constant = {delay_const}")
        # print(f"current_output_timestamp_samples = {current_output_timestamp_samples}")
        # print(f"next_output_timestamp_samples = {next_output_timestamp_samples}")
        # print(f"validity_interval_samples = {fodm_output_samples}")    
        # print(f"phase_linear_temp = {phase_linear}")
        # print(f"phase_constant_temp = {phase_const}")
        # print("-------------------------")

        yield fodm_vals