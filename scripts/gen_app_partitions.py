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
print_template = """
		/* Auto generated code do not modify */
		SMEM_PARTITION_ALIGN(data_smem_{0}_bss_end - data_smem_{0}_start);
		data_smem_{0}_start = .;
		KEEP(*(data_smem_{0}_data))
		data_smem_{0}_bss_start = .;
		KEEP(*(data_smem_{0}_bss))
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

def find_partitions(filename, full_list_of_partitions, partitions_source_file):
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
            if partition_name not in full_list_of_partitions:
                full_list_of_partitions.append(partition_name)
                if args.verbose:
                    partitions_source_file.update({partition_name: filename})

    return (full_list_of_partitions, partitions_source_file)

def generate_final_linker(linker_file, full_list_of_partitions):
    string = linker_start_seq
    size_string = ''
    for partition in full_list_of_partitions:
        string += print_template.format(partition)
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
    args = parser.parse_args()


def main():
    parse_args()
    root_directory = args.directory
    linker_file = args.output
    full_list_of_partitions = []
    partitions_source_file= {}

    for dirpath, dirs, files in os.walk(root_directory):
        for filename in files:
            if re.match(".*\.obj$",filename):
                fullname = os.path.join(dirpath, filename)
                full_list_of_partitions, partitions_source_file = find_partitions(fullname, full_list_of_partitions, partitions_source_file)

    generate_final_linker(linker_file, full_list_of_partitions)
    if args.verbose:
        print("Partitions retrieved:")
        for key in full_list_of_partitions:
            print("    %s: %s\n", key, partitions_source_file[key])

if __name__ == '__main__':
    main()
