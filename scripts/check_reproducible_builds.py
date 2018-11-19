#!/usr/bin/env python
#
# Copyright (c) 2018 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# This script verify that builds are reproducible, i.e.
# producing exactly the same binary if built using the same code base.
# Using diffoscope tool which recursively transform various binary
# formats into more human-readable forms for comparison.
#
# Command to use test script:
# python check_reproducible_builds.py -i1 path_of_elf1 -i2 path_of_elf2

import os
import sys
import time
import re
import subprocess
import argparse
import colorama

def reproducible_builds(input_file1, input_file2):
    res = res1 = res2 = res3 = 0
    filename  = os.path.join('/tmp', 'out.txt')

    if os.path.exists("filename"):
        os.remove("filename")

    if os.path.exists(input_file1):
        if os.path.exists(input_file2):
            subprocess.call("diffoscope --text filename {0} {1}". format(input_file1, input_file2), shell=True)

    if os.path.isfile('filename'):
        with open('filename', 'r') as out_file:
            for line in out_file:
                if 'Start of section headers:' in line:
                    if res is 0:
                        res = int(re.search(r'\d+', line).group())
                    else:
                        res1 = int(re.search(r'\d+', line).group())
                        print(colorama.Fore.LIGHTRED_EX + 'Bytes in section header mismatch')
                        print(colorama.Fore.LIGHTRED_EX + 'Bytes in section header of elf file1 and file2 are {0} and  {1} respectively'.format(res, res1) + colorama.Fore.RESET)
                        res = res1 = 0
                        break
            out_file.seek(0, 0);
            for line in out_file:
                if 'Number of section headers:' in line:
                    if res is 0:
                        res = int(re.search(r'\d+', line).group())
                    else:
                        res1 = int(re.search(r'\d+', line).group())
                        print(colorama.Fore.LIGHTRED_EX + 'Number of section headers mismatch')
                        print(colorama.Fore.LIGHTRED_EX + 'Number of section headers of elf file1 and file2 are {0} and  {1} respectively'.format(res, res1)  + colorama.Fore.RESET)
                        res = res1 = 0
                        break

            out_file.seek(0, 0);
            for line in out_file:
                if 'Symbol table \'.symtab\' contains' in line:
                    if res is 0:
                        res = int(re.search(r'\d+', line).group())
                    else:
                        res1 = int(re.search(r'\d+', line).group())
                        print(colorama.Fore.LIGHTRED_EX + 'Entries in symbol table mismatch')
                        print(colorama.Fore.LIGHTRED_EX + 'Entries in Symbol table of elf file1 and file2 are {0} and  {1} respectively'.format(res, res1) + colorama.Fore.RESET)
                        break
    else:
        print "No difference found in the comparison of both input elf file"

if __name__=="__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument(
        "-i1",
        "--input1",
        required=True,
        help="First ELF file to compare")
    parser.add_argument(
        "-i2",
        "--input2",
        required=True,
        help="Second Input file to compare")

    args = parser.parse_args()

    ret = reproducible_builds(args.input1, args.input2)
    sys.exit(ret)
