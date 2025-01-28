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
    parser.add_argument("infile", help="Input file containing decimal strings to process")
    parser.add_argument(
        "--outfile", required=True, help="Output File for the generated definitions"
    )
    return parser.parse_args()


def extract_decimal(input_file: str, output_file: str):
    """
    Extracts the decimal numbers and generates definitions for these as 32 arguments

    Args:
        input_file (str): Path to the input file.
        output_file (str): Path to the output file.
    """

    with open(input_file) as infile:
        numbers = set(map(int, re.findall(r'\b(?:0|[1-9]\d*)(?:[ULul]*)?\b', infile.read())))

    with open(output_file, 'w') as outfile:
        for number in sorted(numbers):
            outfile.write(f"#define Z_EXPR_DEC_{number} " + ", ".join(f"{number:032b}") + "\n")
            outfile.write(f"#define Z_EXPR_DEC_{number}U " + ", ".join(f"{number:032b}") + "\n")


if __name__ == "__main__":
    args = parse_args()
    extract_decimal(args.infile, args.outfile)
