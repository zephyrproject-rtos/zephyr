#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

"""
This script scans a specified object file and generates a header file
that defined macros for the offsets of various found structure members
(particularly symbols ending with ``_OFFSET`` or ``_SIZEOF``), primarily
intended for use in assembly code.
"""

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection
import magic
import argparse
import subprocess
import sys


def get_symbol_table(obj):
    for section in obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            return section

    raise LookupError("Could not find symbol table")


def gen_offset_header(input_name, input_file, output_file):
    include_guard = "__GEN_OFFSETS_H__"
    output_file.write("""/* THIS FILE IS AUTO GENERATED.  PLEASE DO NOT EDIT.
 *
 * This header file provides macros for the offsets of various structure
 * members.  These offset macros are primarily intended to be used in
 * assembly code.
 */

#ifndef %s
#define %s\n\n""" % (include_guard, include_guard))

    filetype = magic.from_file(input_name)

    if filetype.startswith('ELF'):
        obj = ELFFile(input_file)
        for sym in get_symbol_table(obj).iter_symbols():
            if isinstance(sym.name, bytes):
                sym.name = str(sym.name, 'ascii')

            if not sym.name.endswith(('_OFFSET', '_SIZEOF')):
                continue
            if sym.entry['st_shndx'] != 'SHN_ABS':
                continue
            if sym.entry['st_info']['bind'] != 'STB_GLOBAL':
                continue

            output_file.write(
                "#define %s 0x%x\n" %
                (sym.name, sym.entry['st_value']))

    elif filetype.startswith('Mach-O'):
        process = subprocess.run('nm -g {}'.format(input_name), shell=True,
                                 check=True, stdout=subprocess.PIPE, universal_newlines=True)
        output = process.stdout

        for line in output.splitlines():
            word = line.split()
            if len(word) != 3:
                continue

            sym = type('', (), {})()
            sym.entry = word[0]
            sym.type = word[1]
            sym.name = word[2]

            if not sym.name.endswith(('_OFFSET', '_SIZEOF')):
                continue
            if sym.type != 'A':
                continue

            output_file.write('#define {} 0x{}\n'.format(sym.name, sym.entry))

    output_file.write("\n#endif /* %s */\n" % include_guard)

    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument(
        "-i",
        "--input",
        required=True,
        help="Input object file")
    parser.add_argument(
        "-o",
        "--output",
        required=True,
        help="Output header file")

    args = parser.parse_args()

    input_file = open(args.input, 'rb')
    output_file = open(args.output, 'w')

    ret = gen_offset_header(args.input, input_file, output_file)
    sys.exit(ret)
