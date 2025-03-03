import csv
from decimal import Decimal
import math
import sys

def _to_int(val, scale=1):
    return round(Decimal(val) * Decimal(scale))

def _mod_pmhalf(val):
    """modulo to plus-minus a half [-0.5 to 0.5)"""
    return (((val % Decimal(1)) + Decimal(1.5)) % Decimal(1)) - Decimal(0.5)

def _run_test(test_input_csv, output_csv):
    with open(test_input_csv) as f_in, open(output_csv, 'w') as f_out:

        output_fields = [
            'first_input_timestamp',
            'delay_constant',
            'phase_constant',
            'delay_linear',
            'phase_linear',
            'validity_period',
            'output_PPS',
            'first_output_timestamp',
            ]
        writer = csv.DictWriter(f_out, fieldnames=output_fields, lineterminator="\n")
        writer.writeheader()

        reader = csv.DictReader(f_in)
        for row in reader:
            fo_delay_const = Decimal(row['fo_delay_const']) / Decimal(1e9)
            fo_delay_linear = Decimal(row['fo_delay_linear']) / Decimal(1e9)
            hodm_start_t = Decimal(row['hodm_start_t']) / Decimal(1e3)
            fodm_start_t = Decimal(row['fodm_start_t']) / Decimal(1e3)
            fodm_stop_t = Decimal(row['fodm_stop_t']) / Decimal(1e3)
            fodm = {
                "delay_constant": fo_delay_const,
                "delay_linear": fo_delay_linear,
                "start_time": fodm_start_t,
                "stop_time": fodm_stop_t,
            }
            input_sample_rate = row['input_sample_rate']
            output_sample_rate = row['output_sample_rate']
            f_wb = row['f_wb']
            f_as = row['f_as']
            f_ds = row['f_ds']
            f_scfo = row['f_scfo']

            out_row = calc_fodm_register_values(fodm=fodm, hodm_start_t=hodm_start_t, input_sample_rate=input_sample_rate, output_sample_rate=output_sample_rate, f_wb=f_wb, f_as=f_as, f_ds=f_ds, f_scfo=f_scfo)
            writer.writerow(out_row)

# Modified version of _fodm_loader() from FW notebook's talon_FSP.py.
# This takes one set of FODM, input and output sampling rates, and frequency
# shifts, and calculate the values that would be written to the FODM register
def calc_fodm_register_values(fodm, hodm_start_t, input_sample_rate, output_sample_rate, f_wb, f_as, f_ds, f_scfo):

    fodm_reg_values = {}
    isr = Decimal(input_sample_rate)
    osr = Decimal(output_sample_rate)
    resampling_rate = isr / osr

    hodm_offset_seconds = Decimal(0)
    hodm_seconds = Decimal(int(hodm_start_t) - hodm_offset_seconds)
    hodm_input_timestamp_samples = hodm_seconds * isr
    hodm_output_timestamp_samples = hodm_seconds * osr

    current_output_timestamp_samples = Decimal(
        int((Decimal(fodm["start_time"]) - hodm_seconds) * osr)
    )
    next_output_timestamp_samples = Decimal(
        int((Decimal(fodm["stop_time"]) - hodm_seconds) * osr)
    )
    fodm_output_samples = (
        next_output_timestamp_samples
        - current_output_timestamp_samples
    )  # units of whole output samples

    # calculate the input timestamp based on the output timestamp.
    current_input_timestamp_samples = (
        current_output_timestamp_samples * resampling_rate
    )  # in units of input samples.

    delay_linear = resampling_rate * (Decimal(fodm["delay_linear"])+1)

    delay_linear_int = _to_int(delay_linear, 2**31)
    fodm_reg_values["delay_linear"] = delay_linear_int

    # Maybe remove a small amout of delay, so that hit zero resampling error in the middle of the FODM instead of only
    # at the end - giving an overall positive (200fs?) delay error. Being related to the sample rate (k) here makes some sense.
    delay_linear_error_samples = (
        (Decimal(delay_linear_int) * Decimal(2**-31)) - delay_linear
    ) * fodm_output_samples

    delay_const_input_samples = (
        (Decimal(fodm["delay_constant"])) * isr
    ) - delay_linear_error_samples / 2

    first_input_timestamp_fractional_samples = (
        current_input_timestamp_samples + delay_const_input_samples
    )
    delay_const = first_input_timestamp_fractional_samples - int(
        first_input_timestamp_fractional_samples
    )

    first_input_timestamp_fractional_samples += (
        hodm_input_timestamp_samples
    )
    fodm_reg_values['first_input_timestamp'] = int(
        first_input_timestamp_fractional_samples
    )

    fodm_reg_values['delay_constant'] = _to_int(delay_const, 2**32)

    # OS = Decimal(10) / Decimal(9)  # FIXME - magic number.

    # VDSF =self.vcc_cfg.get("INPUT_FRAME_SIZE") # i.e. 18 # Down Sampling Factor for the VCC
    FFS_j = Decimal(input_sample_rate)  # Freq Slice Sample Rate.
    FFS_0 = Decimal(output_sample_rate)  # Common sample rate.
    F_DS = Decimal(f_ds)  # Frequency Down Shift at VCC.
    F_WB = Decimal(f_wb)  # Wideband shift applied to align a frequency slice to start of a band.
    F_AS = Decimal(f_as)  # additional shift to apply to align channels between frequency slices.
    F_SCFO = Decimal(f_scfo)  # Frequency shift due to SCFO sampling

    # phase_linear = 2* np.pi * ((F_SCFO + F_AS) + (F_WB - F_DS) * delay_linear)/FFS_0
    # phase_const= np.mod(k * T1Pv * phase_linear + 2 * np.pi * (F_WB - F_DS) * delay_const, 2 * np.pi)
    # Divide through by 2*pi, since registers are -0.5 to 0.5
    # phase_linear = ((F_SCFO + F_AS) + (F_WB - F_DS) * delay_linear)/FFS_0
    # phase_const= np.mod(k * T1Pv * phase_linear + (F_WB - F_DS) * delay_const, 1)
    # Correct for the phase due to the resampling.
    phase_linear = (
        (F_SCFO + F_AS)
        + (F_WB - F_DS) * Decimal(fodm["delay_linear"])
    ) / FFS_0

    phase_const = phase_linear * (
        Decimal(current_output_timestamp_samples)
        + hodm_output_timestamp_samples
    ) + (F_WB - F_DS) * Decimal(
        fodm["delay_constant"]
        # Correct for the phase added by the VCC due to the as yet uncorrected delay.
    )
    
    phase_const_mod = _mod_pmhalf(phase_const)
    phase_linear_mod = _mod_pmhalf(phase_linear)
    
    # print(f"start_ts_s = {fodm_start_t}")
    # print(f"stop_ts_s = {fodm_stop_t}")
    # print(f"fo_delay_linear = {fo_delay_linear}")
    # print(f"fo_delay_constant = {fo_delay_const}")
    # print(f"delay_linear = {delay_linear}")
    # print(f"delay_constant = {delay_const}")
    # print(f"current_output_timestamp_samples = {current_output_timestamp_samples}")
    # print(f"next_output_timestamp_samples = {next_output_timestamp_samples}")
    # print(f"validity_interval_samples = {fodm_output_samples}")
    # print(f"delay_linear_int = {delay_linear_int}")
    # print(f"delay_linear_unscaled = {delay_linear_unscaled}")            
    # print(f"delay_linear_error_samples = {delay_linear_error_samples}")
    # print(f"(F_WB - F_DS) * Decimal(fo_delay_linear) = {(F_WB - F_DS) * Decimal(fo_delay_linear)}")            
    # print(f"phase_linear_temp = {phase_linear}")
    # print(f"phase_constant_temp = {phase_const}")
    # print(f"phase_linear_mod = {phase_linear_mod}" )
    # print(f"phase_const_mod = {phase_const_mod}" )
    # print("---------------")

    fodm_reg_values['phase_constant'] = int(
        round(phase_const_mod * 2**31)
    )
    fodm_reg_values['phase_linear'] = int(
        round(phase_linear_mod * 2**31)
    )
    fodm_reg_values['validity_period'] = int(fodm_output_samples) - 1

    # need to limit to 32 bit
    output_pps_timestamp = (
        Decimal(math.ceil(current_output_timestamp_samples / osr))
        * osr
    )
    output_pps_timestamp += hodm_output_timestamp_samples
    fodm_reg_values['output_PPS'] = int(output_pps_timestamp) & 0xffffffff 

    first_output_timestamp_samples = (
        current_output_timestamp_samples
        + hodm_output_timestamp_samples
    )
    fodm_reg_values['first_output_timestamp'] = int(
        first_output_timestamp_samples
    )

    return fodm_reg_values

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"Usage: python3 fodm_calc_ref.py <input_csv> <output_csv>")
        sys.exit(1)
    input_csv = sys.argv[1]
    output_csv = sys.argv[2]
    _run_test(input_csv, output_csv)
