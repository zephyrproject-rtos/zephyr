#!/usr/bin/env python3
#
# Copyright (c) 2018 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Script to generate a linker script organizing application memory partitions

Applications may declare build-time memory domain partitions with
K_APPMEM_PARTITION_DEFINE, and assign globals to them using K_APP_DMEM
or K_APP_BMEM macros. For each of these partitions, we need to
route all their data into appropriately-sized memory areas which meet the
size/alignment constraints of the memory protection hardware.

This linker script is created very early in the build process, before
the build attempts to link the kernel binary, as the linker script this
tool generates is a necessary pre-condition for kernel linking. We extract
the set of memory partitions to generate by looking for variables which
have been assigned to input sections that follow a defined naming convention.
We also allow entire libraries to be pulled in to assign their globals
to a particular memory partition via command line directives.

This script takes as inputs:

- The base directory to look for compiled objects
- key/value pairs mapping static library files to what partitions their globals
  should end up in.

The output is a linker script fragment containing the definition of the
app shared memory section, which is further divided, for each partition
found, into data and BSS for each partition.
"""

import sys
import argparse
import os
import re
from collections import OrderedDict
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

SZ = 'size'
SRC = 'sources'
LIB = 'libraries'

# This script will create sections and linker variables to place the
# application shared memory partitions.
# these are later read by the macros defined in app_memdomain.h for
# initialization purpose when USERSPACE is enabled.
data_template = """
		/* Auto generated code do not modify */
		SMEM_PARTITION_ALIGN(z_data_smem_{0}_bss_end - z_data_smem_{0}_part_start);
		z_data_smem_{0}_part_start = .;
		KEEP(*(data_smem_{0}_data))
"""

library_data_template = """
		*{0}:*(.data .data.*)
"""

bss_template = """
		z_data_smem_{0}_bss_start = .;
		KEEP(*(data_smem_{0}_bss))
"""

library_bss_template = """
		*{0}:*(.bss .bss.* COMMON COMMON.*)
"""

footer_template = """
		z_data_smem_{0}_bss_end = .;
		SMEM_PARTITION_ALIGN(z_data_smem_{0}_bss_end - z_data_smem_{0}_part_start);
		z_data_smem_{0}_part_end = .;
"""

linker_start_seq = """
	SECTION_PROLOGUE(_APP_SMEM_SECTION_NAME,,)
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
	z_data_smem_{0}_part_size = z_data_smem_{0}_part_end - z_data_smem_{0}_part_start;
	z_data_smem_{0}_bss_size = z_data_smem_{0}_bss_end - z_data_smem_{0}_bss_start;
"""

section_regex = re.compile(r'data_smem_([A-Za-z0-9_]*)_(data|bss)')

elf_part_size_regex = re.compile(r'z_data_smem_(.*)_part_size')

def find_obj_file_partitions(filename, partitions):
    with open(filename, 'rb') as f:
        full_lib = ELFFile(f)
        if not full_lib:
            sys.exit("Error parsing file: " + filename)

        sections = [x for x in full_lib.iter_sections()]
        for section in sections:
            m = section_regex.match(section.name)
            if not m:
                continue

            partition_name = m.groups()[0]
            if partition_name not in partitions:
                partitions[partition_name] = {SZ: section.header.sh_size}

                if args.verbose:
                    partitions[partition_name][SRC] = filename

            else:
                partitions[partition_name][SZ] += section.header.sh_size


    return partitions


def parse_obj_files(partitions):
    # Iterate over all object files to find partitions
    for dirpath, _, files in os.walk(args.directory):
        for filename in files:
            if re.match(r".*\.obj$", filename):
                fullname = os.path.join(dirpath, filename)
                find_obj_file_partitions(fullname, partitions)


def parse_elf_file(partitions):
    with open(args.elf, 'rb') as f:
        elffile = ELFFile(f)

        symbol_tbls = [s for s in elffile.iter_sections()
                       if isinstance(s, SymbolTableSection)]

        for section in symbol_tbls:
            for symbol in section.iter_symbols():
                if symbol['st_shndx'] != "SHN_ABS":
                    continue

                x = elf_part_size_regex.match(symbol.name)
                if not x:
                    continue

                partition_name = x.groups()[0]
                size = symbol['st_value']
                if partition_name not in partitions:
                    partitions[partition_name] = {SZ: size}

                    if args.verbose:
                        partitions[partition_name][SRC] = args.elf

                else:
                    partitions[partition_name][SZ] += size


def generate_final_linker(linker_file, partitions):
    string = linker_start_seq
    size_string = ''
    for partition, item in partitions.items():
        string += data_template.format(partition)
        if LIB in item:
            for lib in item[LIB]:
                string += library_data_template.format(lib)
        string += bss_template.format(partition)
        if LIB in item:
            for lib in item[LIB]:
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
    parser.add_argument("-d", "--directory", required=False, default=None,
                        help="Root build directory")
    parser.add_argument("-e", "--elf", required=False, default=None,
                        help="ELF file")
    parser.add_argument("-o", "--output", required=False,
                        help="Output ld file")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Verbose Output")
    parser.add_argument("-l", "--library", nargs=2, action="append", default=[],
                        metavar=("LIBRARY", "PARTITION"),
                        help="Include globals for a particular library or object filename into a designated partition")

    args = parser.parse_args()


def main():
    parse_args()
    partitions = {}

    if args.directory is not None:
        parse_obj_files(partitions)
    elif args.elf is not None:
        parse_elf_file(partitions)
    else:
        return

    for lib, ptn in args.library:
        if ptn not in partitions:
            partitions[ptn] = {}

        if LIB not in partitions[ptn]:
            partitions[ptn][LIB] = [lib]
        else:
            partitions[ptn][LIB].append(lib)


    # Sample partitions.items() list before sorting:
    #   [ ('part1', {'size': 64}), ('part3', {'size': 64}, ...
    #     ('part0', {'size': 334}) ]
    decreasing_tuples = sorted(partitions.items(),
                           key=lambda x: (x[1][SZ], x[0]), reverse=True)

    partsorted = OrderedDict(decreasing_tuples)

    generate_final_linker(args.output, partsorted)
    if args.verbose:
        print("Partitions retrieved:")
        for key in partsorted:
            print("    {0}: size {1}: {2}".format(key,
                                                  partsorted[key][SZ],
                                                  partsorted[key][SRC]))


if __name__ == '__main__':
    main()
