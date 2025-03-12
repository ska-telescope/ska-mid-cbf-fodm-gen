import csv
from decimal import Decimal
import math
import sys

from HODM_to_FODM import calc_fodm_regs

def _to_int(val, scale=1):
    return round(Decimal(val) * Decimal(scale))

def _get_fs_index(input_sample_rate, output_sample_rate, f_scfo):
    fs_index = int(round(f_scfo * 10 / 9 / (input_sample_rate - output_sample_rate)))
    return fs_index

def _to_reg_values(fodm_reg_gen):
    linear_resolution = 2**63
    fodm_regs = {}
    vals = next(fodm_reg_gen)
    fodm_regs["first_input_timestamp"] = vals["first_input_timestamp"]
    fodm_regs["delay_linear"] = _to_int(vals['delay_linear'], linear_resolution)
    fodm_regs["delay_constant"] = _to_int(vals['delay_constant'], 2**32)
    fodm_regs["phase_constant"] = _to_int(vals['phase_constant'], 2**31)
    fodm_regs["phase_linear"] = _to_int(vals['phase_linear'], linear_resolution)
    fodm_regs["validity_period"] = vals['validity_period'] - 1
    fodm_regs["output_PPS"] = vals['output_PPS'] & 0xffffffff 
    fodm_regs["first_output_timestamp"] = vals['first_output_timestamp']
    return fodm_regs

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
            input_sample_rate = Decimal(row['input_sample_rate'])
            output_sample_rate = Decimal(row['output_sample_rate'])
            f_wb = Decimal(row['f_wb'])
            f_as = Decimal(row['f_as'])
            f_ds = Decimal(row['f_ds'])
            f_scfo = Decimal(row['f_scfo'])

            # all the freq shifts are provided to RDT software.
            # FW notebook calculates SCFO and down-shift here.
            # So here we reverse the formula and feed freq slice index to
            # the python
            fs_index = _get_fs_index(input_sample_rate, output_sample_rate, f_scfo)

            fodms = [
                {
                    "delay_constant": fo_delay_const,  # seconds
                    "delay_linear": fo_delay_linear,  # seconds/second
                    "start_time": fodm_start_t, # time of application, seconds
                    "end_time" : fodm_stop_t
                }
            ]

            start_RDT = 0
            fodm_reg_gen = calc_fodm_regs(
                fodms,
                input_sample_rate,
                output_sample_rate,
                fs_index,
                start_RDT,
                f_wb,
                f_as
            )

            fodm_regs = _to_reg_values(fodm_reg_gen)

            writer.writerow(fodm_regs)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"Usage: python3 fodm_calc_ref.py <input_csv> <output_csv>")
        sys.exit(1)
    input_csv = sys.argv[1]
    output_csv = sys.argv[2]
    _run_test(input_csv, output_csv)
