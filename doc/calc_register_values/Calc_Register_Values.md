# Calculating FPGA Register Values for FO Models
The `ska_mid_cbf_fodm_gen::CalcFodmRegisterValues` function takes in a first order delay model and applies delay and phase correction to arrive at the values to be stored in the FPGA registers. It takes in the following inputs:

| Input                | Data Type       | Description
| -                    | -               | -
| `fo_poly`            | `FoPoly&`       | The first-order delay model to calculate register values for.
| `input_sample_rate`  | `uint32_t`      | The input sample rate in Hz (samples/second).
| `output_sample_rate` | `uint32_t`      | The output sample rate in Hz (samples/second).
| `freq_down_shift`    | `double`        | The frequency down-shift at the VCC-OSPPFB, in Hz.
| `freq_align_shift`   | `double`        | The frequency shift applied to align fine channels between Frequency Slices, in Hz.
| `freq_wb_shift`      | `double`        | The net wideband (WB) frequency shift, in Hz.
| `freq_scfo_shift`    | `double`        | The frequency shift required due to SCFO sampling, in Hz.

The results we want to achieve are:

| Register                 | VHDL Type    | Bit Width   | Description
| -                        | -            | -           | -
| `first_input_timestamp`  | `natural`    | 64          | The timestamp of the first input sample for this FODM, i.e. the time at which this FODM becomes valid.
| `first_output_timestamp` | `natural`    | 64          | The timestamp of the first output sample for this FODM.
| `delay_linear`           | `natural`    | 32          | The corrected linear coefficient of the delay polynomial. In the near future this will be a 64-bit register.
| `delay_constant`         | `natural`    | 32          | The corrected constant coefficient of the delay polynomial.
| `phase_linear`           | `integer`    | 32          | The corrected linear coefficient of the phase polynomial. In the near future this will be a 64-bit register.
| `phase_constant`         | `integer`    | 32          | The corrected constant coefficient of the phase polynomial.
| `validity_period`        | `natural`    | 32          | The period during which the FODM is valid, measured as the number of output samples - 1.
| `output_PPS`             | `natural`    | 32          | The lower bits of the output timestamp in this FODM that should be flagged with a PPS marker. (???)

Note that this document is written and ordered for logical readability, and so operations are not necessarily in the same order nor do they have exactly the same variable names as in the code.

## Definitions
Let $c_1, c_0$ represent the input FO polynomial coefficients such that $D(t) = c_1t + c_0$.  
Let $t_{start}, t_{stop}$ represent the input FODM start and stop timestamps in seconds (converted from the values in `fo_poly`).  
Let $f_{down}, f_{align}, f_{wb}, f_{scfo}$ represent their respective frequency shift values.

## Resampling Rate
We calculate the *resampling rate*, which is used to convert an output time value to an input time value:
$$\operatorname{resampling\_rate} = \frac{\operatorname{input\_sample\_rate}}{\operatorname{output\_sample\_rate}}$$
For example, if the input sample rate is $220000200$ and the output sample rate is $220200960$, then:
$$\operatorname{resampling\_rate} = \frac{220000200}{220200960} = 0.999088287353515625$$

## Timestamps
We set the current output FODM start time, in number of samples, to be:
$$\operatorname{output\_start\_time} = \lfloor t_{start} \cdot \operatorname{output\_sample\_rate} \rfloor$$
We also use this value for the `first_output_timestamp` register.

We set the next output start time (AKA the current output stop time), in number of samples, to be:
$$\operatorname{next\_output\_start\_time} = \lfloor t_{stop} \cdot \operatorname{output\_sample\_rate} \rfloor$$

The output FODM validity interval is then the difference between the start of the current FODM and the start of the next FODM:
$$v = \operatorname{next\_output\_start\_time} - \operatorname{output\_start\_time}$$

and we set the `validity_period` register to 1 less than that value:
$$\operatorname{validity\_period} = v - 1$$

Finally, we use the resampling rate from earlier to get the input start time, in samples:
$$\operatorname{input\_start\_time} = \operatorname{resampling\_rate} \cdot \operatorname{output\_start\_time}$$

For example, given $t_{start} = 720000000.000,\ t_{stop} = 720000000.010$, and maintaining given/calculated values from earlier examples, then:
$$
\begin{aligned}
\operatorname{output\_start\_time} &= \lfloor 720000000.000 \cdot 220200960 \rfloor &=&\ 158544691200000000 \\
\operatorname{next\_output\_start\_time} &= \lfloor 720000000.010 \cdot 220200960 \rfloor &=&\ 158544691202202009 \\
v &= 158544691202202009 - 158544691200000000 &=&\ 2202009 \\
\operatorname{validity\_period} &= 2202009 - 1 &=&\ 2202008 \\
\operatorname{input\_start\_time} &= 0.999088287353515625 \cdot 158544691200000000 &=&\ 158400144000000000 \\
\end{aligned}
$$


## Delay Linear Coefficient
We calculate the (unscaled) output linear coefficient by applying a correction to the input linear coefficient:
$$d_1 = c_1 + \operatorname{resampling\_rate}$$
and then we scale by a factor of $2^{31}$ to fit the register's natural number data type:
$$\operatorname{delay\_linear} = \operatorname{round}(2^{31}d_1)$$
and so we have the final value for the `delay_linear` register.

Additionally, we calculate the total error over the validity period, in samples, caused by the rounding operation during scaling:
$$\operatorname{delay\_linear\_err} = v(2^{-31}(\operatorname{delay\_linear}) - d_1)$$

For example, given $c_1 = 0.000000000012$, and maintaining given/calculated values from earlier examples, then:
$$
\begin{aligned}
d_1 &= 0.000000000012 + 0.999088287353515625 &=&\ 0.999088287365515625 \\
\operatorname{delay\_linear} &= \operatorname{round}(2^{31} \cdot 0.999088287365515625) &=&\ 2145525760 \\
\operatorname{delay\_linear\_err} &= 2202009(2^{-31} \cdot 2145525760 - 0.999088287365515625) &\approx& -0.000026424108 \\
\end{aligned}
$$

## First Input Timestamp
Before calculating the constant delay coefficient we first need the first input timestamp, in samples:
$$t_f = \operatorname{input\_start\_time} + (c_0 \cdot \operatorname{input\_sample\_rate}) - \frac{\operatorname{delay\_linear\_err}}{2}$$
Or if the error term is disabled:
$$t_f = \operatorname{input\_start\_time} + (c_0 \cdot \operatorname{input\_sample\_rate})$$
And for the register, we only want the integer part:
$$\operatorname{first\_input\_timestamp} = \lfloor t_f \rfloor$$

For example, given $c_0 = 0.000002$, and maintaining given/calculated values from earlier examples, then:
$$
\begin{aligned}
t_f &= 158400144000000000 + (0.000002 \cdot 220000200) - \frac{-0.000026424108}{2} &=&\ 158400144000000440.0004 \\
\operatorname{first\_input\_timestamp} &= \lfloor 158400144000000440.0004 \rfloor &=&\ 158400144000000440 \\
\end{aligned}
$$


## Delay Constant Coefficient
We then get the corrected delay coefficient by taking the fractional part of the first input timestamp:
$$d_0 = t_f - \lfloor t_f \rfloor$$
and then we scale by a factor of $2^{32}$ to fit the register's natural number data type:
$$\operatorname{delay\_constant} = \operatorname{round}(2^{32}d_0)$$
and so we have the final value for the `delay_constant` register.

For example, maintaining given/calculated values from earlier examples, then:
$$
\begin{aligned}
d_0 &= 158400144000000440.0004 - \lfloor 158400144000000440.0004 \rfloor &=&\ 0.0004 \\
\operatorname{delay\_constant} &= \operatorname{round}(2^{32} \cdot 0.0004) &=&\ 1717987 \\
\end{aligned}
$$

## Phase Linear Coefficient
We start to calculate the linear phase coefficient as follows:
$$p_{1\_temp} = \frac{c_1 \cdot (f_{wb} - f_{down}) + f_{scfo} + f_{align}}{\operatorname{output\_sample\_rate}}$$
We then take this value modulo the interval $[-0.5, 0.5)$, using the formula:
$$p_1 = \operatorname{mod}(\operatorname{mod}(p_{1\_temp}, 1) + 1.5, 1) - 0.5$$

and then we scale by a factor of $2^{31}$ to fit the register's integer data type:
$$\operatorname{phase\_linear} = \operatorname{round}(2^{31}p_1)$$
and so we have the final value for the `phase_linear` register.

For example, given $f_{down} = -990000900,\ f_{align} = -46720,\ f_{wb} = 0,\ f_{scfo} = -903420$, and maintaining given/calculated values from earlier examples, then:
$$
\begin{aligned}
p_{1\_temp} &= \frac{0.000000000012 \cdot (0 - (-990000900)) + (-903420) + (-46720)}{220200960} &\approx&\ -0.004314876684 \\
p_1 &= \operatorname{mod}(\operatorname{mod}(-0.004314876684, 1) + 1.5, 1) - 0.5 &=&\ -0.004314876684 \\
\operatorname{phase\_linear} &= \operatorname{round}(2^{31} \cdot -0.004314876684) &=&\ -9266127 \\
\end{aligned}
$$

## Phase Constant Coefficient
We start to calculate the constant phase coefficient as follows:
$$p_{0\_temp} = c_0 \cdot (f_{wb} - f_{down}) + \frac{\operatorname{output\_start\_time} \cdot (f_{scfo} + f_{align})}{\operatorname{output\_sample\_rate}}$$
We then take this value modulo the interval $[-0.5, 0.5)$, using the same formula as for the linear coefficient:
$$p_0 = \operatorname{mod}(\operatorname{mod}(p_{0\_temp}, 1) + 1.5, 1) - 0.5$$

and then we scale by a factor of $2^{31}$ to fit the register's integer data type:
$$\operatorname{phase\_constant} = \operatorname{round}(2^{31}p_0)$$
and so we have the final value for the `phase_constant` register.

For example, maintaining given/calculated values from earlier examples, then:
$$
\begin{aligned}
p_{0\_temp} &= 0.000002 \cdot (0 - (-990000900)) + \frac{158544691200000000 \cdot (-903420 + (-46720))}{220200960} &=&\ -684100799998019.9982 \\
p_1 &= \operatorname{mod}(\operatorname{mod}(-684100799998019.9982, 1) + 1.5, 1) - 0.5 &=&\ 0.0018 \\
\operatorname{phase\_linear} &= \operatorname{round}(2^{31} \cdot 0.0018) &=&\ 3865471 \\
\end{aligned}
$$

## Output PPS
(Need more detail)
$$\operatorname{output\_pps} = \left \lfloor \left \lceil \frac{\operatorname{output\_start\_time}}{\operatorname{output\_sample\_rate}} \right \rceil \cdot \operatorname{output\_sample\_rate} \right \rfloor \& \ \text{0xFFFFFFFF}$$
