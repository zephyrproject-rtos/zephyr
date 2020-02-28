#!/usr/bin/env python3
#
# Copyright (c) 2018 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# This script will parse the serial console log file and create the required
# gcda files.
# Usage python3 ${ZEPHYR_BASE}/scripts/gen_gcov_files.py -i console_output.log
# Add -v for verbose

import argparse
import os
import re

def retrieve_data(input_file):
    extracted_coverage_info = {}
    capture_data = False
    reached_end = False
    with open(input_file, 'r') as fp:
        for line in fp.readlines():
            if re.search("GCOV_COVERAGE_DUMP_START", line):
                capture_data = True
                continue
            if re.search("GCOV_COVERAGE_DUMP_END", line):
                reached_end = True
                break
            # Loop until the coverage data is found.
            if not capture_data:
                continue

            # Remove the leading delimiter "*"
            file_name = line.split("<")[0][1:]
            # Remove the trailing new line char
            hex_dump = line.split("<")[1][:-1]
            extracted_coverage_info.update({file_name:hex_dump})

    if not reached_end:
        print("incomplete data captured from %s" %input_file)
    return extracted_coverage_info

def create_gcda_files(extracted_coverage_info):
    if args.verbose:
        print("Generating gcda files")
    for filename, hexdump_val in extracted_coverage_info.items():
        if args.verbose:
            print(filename)
        # if kobject_hash is given for coverage gcovr fails
        # hence skipping it problem only in gcovr v4.1
        if "kobject_hash" in filename:
            filename = filename[:-4] + "gcno"
            try:
                os.remove(filename)
            except Exception:
                pass
            continue

        with open(filename, 'wb') as fp:
            fp.write(bytes.fromhex(hexdump_val))


def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-i", "--input", required=True,
                        help="Input dump data")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Verbose Output")
    args = parser.parse_args()



def main():
    parse_args()
    input_file = args.input

    extracted_coverage_info = retrieve_data(input_file)
    create_gcda_files(extracted_coverage_info)


if __name__ == '__main__':
    main()
