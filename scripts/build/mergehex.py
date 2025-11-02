#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# This merges a set of input hex files into a single output hex file.
# Any conflicts will result in an error being reported.

from intelhex import IntelHex
from intelhex import AddressOverlapError

import argparse


def merge_hex_files(output, input_hex_files, overlap, output_bin):
    ih = IntelHex()

    for hex_file_path in input_hex_files:
        to_merge = IntelHex(hex_file_path)

        # Since 'arm-none-eabi-objcopy' incorrectly inserts record
        # type '03 - Start Segment Address', we need to remove the
        # start_addr to avoid conflicts when merging.
        to_merge.start_addr = None

        try:
            ih.merge(to_merge, overlap=overlap)
        except AddressOverlapError:
            raise AddressOverlapError("{} has merge issues".format(hex_file_path))

    output_format = "bin" if output_bin else "hex"
    ih.tofile(output, format=output_format)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Merge hex files.",
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)
    parser.add_argument("-o", "--output", required=False, default="merged.hex",
                        help="Output file name.")
    parser.add_argument("--overlap", default="error",
                        help="What to do when files overlap (error, ignore, replace). "
                             "See IntelHex.merge() for more info.")
    parser.add_argument("--output-bin", action='store_true',
                        help="Save the merged content as binary file.")
    parser.add_argument("input_files", nargs='*')
    return parser.parse_args()


def main():
    args = parse_args()

    merge_hex_files(args.output, args.input_files, args.overlap, args.output_bin)


if __name__ == "__main__":
    main()
