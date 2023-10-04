#!/usr/bin/env python3

# Copyright (c) 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

"""
Zephyr's NTC Thermistor DTS Table generator
###########################################

This script can be used to generate a "zephyr,ntc-thermistor-rt-table" compatible
Device Tree node for NTC thermistors.  This uses the Beta Parameter Equation

https://devxplained.eu/en/blog/temperature-measurement-with-ntcs#beta-parameter-equation

Look-up the following electrical characteristics in the thermistor's data sheet:
    Nominal resistance (R25) - The thermistor's resistance measured at 25C
    Beta value (25/85) - This is the resistance value at 25C and at 85C

Usage::

    python3 ntc_thermistor_table.py \
        -r25 10000                  \
        -b 3974 > thermistor.dtsi

"""

import argparse
import os
import math

def c_to_k(c: float):
    """ Convert Celicius to Kelvin """
    return c + 273.15

def beta_equation_calc_resistance(r25, beta, temp_c):
    t0_kelvin = c_to_k(25)

    exp = beta * ((1 / c_to_k(temp_c)) - (1 / t0_kelvin))
    resistance = r25 * math.exp(exp)

    return resistance


def main(
    r25: float, beta: float, interval: int, temp_init: int, temp_final: int
) -> None:
    temps_range = range(temp_init, temp_final + interval - 1, interval)

    print(f"/* NTC Thermistor Table Generated with {os.path.basename(__file__)} */\n")
    print(f"/ {{")
    print(
        f"\tthermistor_R25_{int(r25)}_B_{int(beta)}: thermistor-R25-{int(r25)}-B-{int(beta)} {{"
    )
    print(f'\t\tstatus = "disabled";')
    print(f'\t\tcompatible = "zephyr,ntc-thermistor-rt-table";\n')

    table = []
    for temp in temps_range:
        resistance = beta_equation_calc_resistance(r25, beta, temp)
        table.append(f"<({int(temp)}) {int(resistance)}>")

    tbl_string = ',\n\t\t\t'.join(table)
    print(f"\t\t/* TR Table Format <temp resistance> */")
    print(f"\t\ttr-table = {tbl_string};")
    print(f"\t}};")
    print(f"}};")


if __name__ == "__main__":
    parser = argparse.ArgumentParser("NTC Thermistor generator", allow_abbrev=False)
    parser.add_argument(
        "-r25", type=float, required=True, help="Nominal resistance of thermistor"
    )
    parser.add_argument(
        "-b", "--beta", type=float, required=True, help="Beta(25/85) value"
    )
    parser.add_argument(
        "-i",
        "--interval",
        type=int,
        required=False,
        help="Generated table interval",
        default=10,
    )
    parser.add_argument(
        "-ti",
        type=int,
        required=False,
        help="First temperature",
        default=-25,
    )
    parser.add_argument(
        "-tf",
        type=int,
        required=False,
        help="Final temperature",
        default=125,
    )
    args = parser.parse_args()

    main(args.r25, args.beta, args.interval, args.ti, args.tf)
