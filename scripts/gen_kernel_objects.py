#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
import argparse
import re
import os

def main():
    parser = argparse.ArgumentParser(
        description='This script will create a header file to use for linker script creation \
                     when using separate app and kernel memory space in Zephyr.')

    parser.add_argument('-i', '--file-list', required=True,
                        help='List of file to place in output file, separated by ;')
    parser.add_argument('-o', '--out-file', required=True,
                        help='Header file to write containing define with provided list of file')
    parser.add_argument('-e', '--exclude-pattern', required=True,
                        help='Exclude pattern for files in list which should be dis-regarded.')

    args = parser.parse_args()

    list=args.file_list.split(';')
    exclude_patterns=args.exclude_pattern.split(' ')

    for item in list:
        filename = os.path.basename(item)
        for pattern in exclude_patterns:
            if re.match(pattern, filename):
                list.remove(item)

    out_string = '#define KERNELSPACE_OBJECT_FILES '

    out_string += ' '.join(list)

    with open(args.out_file, 'w') as fp:
        fp.write(out_string)
        fp.close()

if __name__ == "__main__":
    main()
