#!/usr/bin/env python3
import sys
import argparse
import os
import re
import string
from elf_helper import ElfHelper
from elftools.elf.elffile import ELFFile


# This script will create linker comands for power of two aligned MPU
# when APP_SHARED_MEM is enabled.
print_template = """
/* Auto generated code do not modify */
. = ALIGN( 1 << LOG2CEIL(data_smem_{0}b_end - data_smem_{0}));
data_smem_{0} = .;
KEEP(*(SORT(data_smem_{0}*)))
. = ALIGN(_app_data_align);
data_smem_{0}b_end = .;
. = ALIGN( 1 << LOG2CEIL(data_smem_{0}b_end - data_smem_{0}));
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
                partition_name = section.name.split("data_smem_")[1]
                if partition_name not in full_list_of_partitions:
                    full_list_of_partitions.append(partition_name)
                    if args.verbose:
                        partitions_source_file.update({partition_name: filename})

    return( full_list_of_partitions, partitions_source_file)

def cleanup_remove_bss_regions(full_list_of_partitions):
    for partition in full_list_of_partitions:
        if (partition+"b" in full_list_of_partitions):
            full_list_of_partitions.remove(partition+"b")
    return full_list_of_partitions

def generate_final_linker(linker_file, full_list_of_partitions):
    string= ''
    for partition in full_list_of_partitions:
        string += print_template.format(partition)

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

    full_list_of_partitions = cleanup_remove_bss_regions(full_list_of_partitions)
    generate_final_linker(linker_file, full_list_of_partitions)
    if args.verbose:
        print("Partitions retrieved: PARTITION, FILENAME")
        print([key + " "+ partitions_source_file[key] for key in full_list_of_partitions])

if __name__ == '__main__':
    main()
