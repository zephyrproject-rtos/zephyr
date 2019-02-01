#!/usr/bin/env python3
#
# Copyright (c) 2018 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import sys
import argparse
import os
import re
import string
from elf_helper import ElfHelper
from elftools.elf.elffile import ELFFile


# This script will create sections and linker variables to place the
# application shared memory partitions.
# these are later read by the macros defined in app_memdomain.h for
# initialization purpose when APP_SHARED_MEM is enabled.
data_template = """
		/* Auto generated code do not modify */
		SMEM_PARTITION_ALIGN(data_smem_{0}_bss_end - data_smem_{0}_start);
		data_smem_{0}_start = .;
		KEEP(*(data_smem_{0}_data))
"""

library_data_template = """
		*{0}:*(.data .data.*)
"""

bss_template = """
		data_smem_{0}_bss_start = .;
		KEEP(*(data_smem_{0}_bss))
"""

library_bss_template = """
		*{0}:*(.bss .bss.* COMMON COMMON.*)
"""

footer_template = """
		SMEM_PARTITION_ALIGN(data_smem_{0}_bss_end - data_smem_{0}_start);
		data_smem_{0}_bss_end = .;
		data_smem_{0}_end = .;
"""

linker_start_seq = """
	SECTION_PROLOGUE(_APP_SMEM_SECTION_NAME, (OPTIONAL),)
	{
		APP_SHARED_ALIGN;
		_app_smem_start = .;
"""

linker_end_seq = """
		APP_SHARED_ALIGN;
		_app_smem_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
"""

size_cal_string = """
	data_smem_{0}_size = data_smem_{0}_end - data_smem_{0}_start;
	data_smem_{0}_bss_size = data_smem_{0}_bss_end - data_smem_{0}_bss_start;
"""

section_regex = re.compile(r'data_smem_([A-Za-z0-9_]*)_(data|bss)')

def find_partitions(filename, partitions, sources):
    with open(filename, 'rb') as f:
        full_lib = ELFFile( f)
        if (not full_lib):
            print("Error parsing file: ",filename)
            os.exit(1)

        sections = [x for x in full_lib.iter_sections()]
        for section in sections:
            m = section_regex.match(section.name)
            if not m:
                continue

            partition_name = m.groups()[0]
            if partition_name not in partitions:
                partitions[partition_name] = []
                if args.verbose:
                    sources.update({partition_name: filename})

    return (partitions, sources)


def generate_final_linker(linker_file, partitions):
    string = linker_start_seq
    size_string = ''
    for partition, libs in partitions.items():
        string += data_template.format(partition)
        for lib in libs:
            string += library_data_template.format(lib)
        string += bss_template.format(partition)
        for lib in libs:
            string += library_bss_template.format(lib)
        string += footer_template.format(partition)
        size_string += size_cal_string.format(partition)

    string += linker_end_seq
    string += size_string
    with open(linker_file, "w") as fw:
        fw.write(string)


def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-d", "--directory", required=True,
                        help="Root build directory")
    parser.add_argument("-o", "--output", required=False,
                        help="Output ld file")
    parser.add_argument("-v", "--verbose", action="count", default =0,
                        help="Verbose Output")
    parser.add_argument("-l", "--library", nargs=2, action="append", default=[],
                        metavar=("LIBRARY", "PARTITION"),
                        help="Include globals for a particular library or object filename into a designated partition")

    args = parser.parse_args()


def main():
    parse_args()
    linker_file = args.output
    partitions = {}
    sources = {}

    for dirpath, dirs, files in os.walk(args.directory):
        for filename in files:
            if re.match(".*\.obj$",filename):
                fullname = os.path.join(dirpath, filename)
                find_partitions(fullname, partitions,
                                sources)

    for lib, ptn in args.library:
        if ptn not in partitions:
            partitions[ptn] = [lib]
        else:
            partitions[ptn].append(lib)

    generate_final_linker(args.output, partitions)
    if args.verbose:
        print("Partitions retrieved:")
        for key in partitions:
            print("    %s: %s\n", key, sources[key])

if __name__ == '__main__':
    main()
