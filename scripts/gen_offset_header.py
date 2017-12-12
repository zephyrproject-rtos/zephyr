#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection
import argparse
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

    output_file.write("\n#endif /* %s */\n" % include_guard)

    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
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
