#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import binascii
import sys


COREDUMP_PREFIX_STR = "#CD:"

COREDUMP_BEGIN_STR = COREDUMP_PREFIX_STR + "BEGIN#"
COREDUMP_END_STR = COREDUMP_PREFIX_STR + "END#"
COREDUMP_ERROR_STR = COREDUMP_PREFIX_STR + "ERROR CANNOT DUMP#"


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument("infile", help="Serial Log File")
    parser.add_argument("outfile",
            help="Output file for use with coredump GDB server")

    return parser.parse_args()


def main():
    args = parse_args()

    infile = open(args.infile, "r")
    if not infile:
        print(f"ERROR: Cannot open input file: {args.infile}, exiting...")
        sys.exit(1)

    outfile = open(args.outfile, "wb")
    if not outfile:
        print(f"ERROR: Cannot open output file for write: {args.outfile}, exiting...")
        sys.exit(1)

    print(f"Input file {args.infile}")
    print(f"Output file {args.outfile}")

    has_begin = False
    has_end = False
    has_error = False
    go_parse_line = False
    bytes_written = 0
    for line in infile.readlines():
        if line.find(COREDUMP_BEGIN_STR) >= 0:
            # Found "BEGIN#" - beginning of log
            has_begin = True
            go_parse_line = True
            continue

        if line.find(COREDUMP_END_STR) >= 0:
            # Found "END#" - end of log
            has_end = True
            go_parse_line = False
            break

        if line.find(COREDUMP_ERROR_STR) >= 0:
            # Error was encountered during dumping:
            # log is not usable
            has_error = True
            go_parse_line = False
            break

        if not go_parse_line:
            continue

        prefix_idx = line.find(COREDUMP_PREFIX_STR)

        if prefix_idx < 0:
            continue

        prefix_idx += len(COREDUMP_PREFIX_STR)
        hex_str = line[prefix_idx:].strip()

        binary_data = binascii.unhexlify(hex_str)
        outfile.write(binary_data)
        bytes_written += len(binary_data)

    if not has_begin:
        print("ERROR: Beginning of log not found!")
    elif not has_end:
        print("WARN: End of log not found! Is log complete?")
    elif has_error:
        print("ERROR: log has error.")
    else:
        print(f"Bytes written {bytes_written}")

    infile.close()
    outfile.close()


if __name__ == "__main__":
    main()
