#!/usr/bin/env python3

# Copyright (c) 2025 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0
#
# Extracts decimal numbers in the specified file and generates definitions
# that can be used with the EXPR APIs.
# This is primarily intended for use with devicetree_generated.h.

import argparse
import re


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("infiles", nargs="*", help="Input files containing decimal to process")
    parser.add_argument(
        "--outfile", required=True, help="Output File for the generated definitions"
    )
    return parser.parse_args()


def extract_decimal(output_file: str, input_files):
    """
    Extracts the decimal numbers and generates definitions for these as 32 arguments

    Args:
        input_file (str): Path to the input file.
        output_file (str): Path to the output file.
    """

    nums = []

    for f in input_files:
        try:
            with open(f) as infile:
                nums = map(int, re.findall(r'\b(?:0|[1-9]\d*)(?:[ULul]*)?\b', infile.read()))
                nums = sorted(set(v for x in nums for v in (x - 1, x, x + 1, x * 2) if v >= 0))
        except Exception:
            pass

    with open(output_file, 'w') as outfile:
        for n in nums:
            outfile.write(f"#define Z_EXPR_BITS_{n}U " + ", ".join(f"{n:032b}") + "\n")
        for n in nums:
            outfile.write(f"#define Z_EXPR_BIN_TO_DEC_0B{n:032b} " + f"{n}" + "\n")


if __name__ == "__main__":
    args = parse_args()
    extract_decimal(args.outfile, args.infiles)
