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
		SMEM_PARTITION_ALIGN(data_smem_{0}_bss_end - data_smem_{0}_data_start);
		data_smem_{0}_data_start = .;
		KEEP(*(data_smem_{0}_data))
		data_smem_{0}_bss_start = .;
		KEEP(*(data_smem_{0}_bss))
		SMEM_PARTITION_ALIGN(data_smem_{0}_bss_end - data_smem_{0}_data_start);
		data_smem_{0}_bss_end = .;
"""
#code for library functions. that need to placed in partitions.
print_template_libc = """
		/* Auto generated code do not modify */
		SMEM_PARTITION_ALIGN(data_smem_{0}_bss_end - data_smem_{0}_data_start);
		data_smem_{0}_data_start = .;
		*{0}.a:*(.data .data.*)
		KEEP(*(data_smem_{0}_data))
		data_smem_{0}_bss_start = .;
		*{0}.a:*(.bss .bss.* COMMON COMMON.*)
		KEEP(*(data_smem_{0}_bss))
		SMEM_PARTITION_ALIGN(data_smem_{0}_bss_end - data_smem_{0}_data_start);
		data_smem_{0}_bss_end = .;
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
	data_smem_{0}_size = data_smem_{0}_bss_end - data_smem_{0}_data_start;
	data_smem_{0}_data_size = data_smem_{0}_bss_start - data_smem_{0}_data_start;
	data_smem_{0}_bss_size = data_smem_{0}_bss_end - data_smem_{0}_bss_start;
"""


def find_partitions(filename, full_list_of_partitions, partitions_source_file):
    with open(filename, 'rb') as f:
        full_lib = ELFFile( f)
        if (not full_lib):
            print("Error parsing file: ",filename)
            os.exit(1)

        sections = [ x for x in full_lib.iter_sections()]
        for section in sections:
            if ("smem" in  section.name and not ".rel" in section.name):
                partition_name = section.name.split("data_smem_")[1].split("_data")[0].split("_bss")[0]
                if partition_name == "libc":
                    continue
                if partition_name not in full_list_of_partitions:
                    full_list_of_partitions.append(partition_name)
                    if args.verbose:
                        partitions_source_file.update({partition_name: filename})

    return( full_list_of_partitions, partitions_source_file)

def cleanup_remove_bss_regions(full_list_of_partitions):
    for partition in full_list_of_partitions:
        if (partition+"_bss" in full_list_of_partitions):
            full_list_of_partitions.remove(partition+"_bss")
    return full_list_of_partitions


def generate_final_linker(linker_file, full_list_of_partitions):
    string = linker_start_seq
    size_string = ''
    for partition in full_list_of_partitions:
        string += print_template.format(partition)
        size_string += size_cal_string.format(partition)

    # for libc we need to do a bit more linker magic to get it to
    # work with APP_SHARED_MEM. Here we just check if libc is enabled
    # if so we add some linker magic to get this working.
    if(args.newlib_libc):
        string += print_template_libc.format("libc")
        size_string += size_cal_string.format("libc")

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
    parser.add_argument("--newlib-libc", required=False, action='store_true',
                        help="Newlib specific code is included")
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

    full_list_of_partitions = cleanup_remove_bss_regions(full_list_of_partitions)
    generate_final_linker(linker_file, full_list_of_partitions)
    if args.verbose:
        print("Partitions retrieved: PARTITION, FILENAME")
        print([key + " "+ partitions_source_file[key] for key in full_list_of_partitions])

if __name__ == '__main__':
    main()
