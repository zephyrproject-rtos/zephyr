#!/usr/bin/env python3
#
# Copyright (c) 2026 Antmicro <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0

from argparse import ArgumentParser
from decimal import Decimal

INT32_MIN = -(2**31)
INT32_MAX = 2**31 - 1


def to_q31(val1: int, val2: int, shift: int) -> int:
    whole = val1 * (1 << 31)
    frac = int((val2 * (1 << 31)) / 1000000)
    return max(INT32_MIN, min(INT32_MAX, (whole + frac) >> shift))


def sensor_value_str(val1: int, val2: int) -> str:
    sign = "-" if val1 < 0 or val2 < 0 else ""
    return f"{sign}{abs(val1)}.{abs(val2):06d}"


def q31_str(q: int, shift: int) -> str:
    val = (Decimal(q) * Decimal(2) ** (shift - 31)).normalize()
    return f"{val:.06f}"


def all_shifts(val1: int, val2: int) -> list[tuple[int, int, int]]:
    return [(val1, val2, shift) for shift in range(0, 33)]


# (val1, val2, shift)
CASES: list[tuple[int, int, int]] = [
    # Zero
    *all_shifts(0, 0),
    # One
    *all_shifts(1, 0),
    *all_shifts(-1, 0),
    # Smallest fraction
    *all_shifts(0, 1),
    *all_shifts(0, -1),
    *all_shifts(INT32_MIN, -1),
    *all_shifts(INT32_MAX, 1),
    # Biggest fraction
    *all_shifts(0, 999999),
    *all_shifts(0, -999999),
    *all_shifts(INT32_MIN, -999999),
    *all_shifts(INT32_MAX, 999999),
    # INT32 limits
    *all_shifts(INT32_MIN, 0),
    *all_shifts(INT32_MAX, 0),
    # ~ 2^32 / 10^6 limits
    *all_shifts(4294, 967295),
    *all_shifts(-4294, -967295),
    *all_shifts(4294, 967296),
    *all_shifts(-4294, -967296),
    *all_shifts(4294, 967297),
    *all_shifts(-4294, -967297),
    # Typcical values
    *all_shifts(0, 500000),
    *all_shifts(0, -500000),
    *all_shifts(1, 500000),
    *all_shifts(-1, -500000),
    *all_shifts(21, 370000),
    *all_shifts(-21, -370000),
    *all_shifts(36, 600000),
    *all_shifts(-36, -600000),
    *all_shifts(114, 2345),
    *all_shifts(-144, -2345),
    *all_shifts(1023, 908070),
    *all_shifts(-1023, -908070),
    *all_shifts(12345, 67890),
    *all_shifts(-12345, -67890),
    *all_shifts(83865, 600000),
    *all_shifts(83865, -600000),
]


def main():
    parser = ArgumentParser("Generate test q31 conversion table", allow_abbrev=False)
    parser.add_argument("filepath", help="Output filepath")
    args = parser.parse_args()

    with open(args.filepath, "w", encoding="UTF-8") as fp:
        fp.write("{")

        for val1, val2, shift in CASES:
            q31 = to_q31(val1, val2, shift)
            fp.write(f"\t/* {sensor_value_str(val1, val2)} -> {q31_str(q31, shift)} */\n")
            fp.write(f"\t{{ {{{val1}, {val2}}}, {shift}, (q31_t)({q31}) }},\n")

        fp.write("}")


if __name__ == "__main__":
    main()
