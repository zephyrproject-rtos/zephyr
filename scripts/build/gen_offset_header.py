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

import argparse
import os
import re
import subprocess
import sys

from elftools.common.exceptions import ELFError
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection


def get_symbol_table(obj):
    for section in obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            return section

    raise LookupError("Could not find symbol table")


def iter_macho_absolute_symbols(path, nm):
    """Yield the absolute offset symbols of a Mach-O object.

    Mach-O has no equivalent of the ELF absolute section, so the symbols
    GEN_ABSOLUTE_SYM() defines are read back from the symbol table instead.
    """
    output = subprocess.check_output([nm, path], text=True)
    for line in output.splitlines():
        match = re.match(r"^([0-9a-fA-F]+)\s+([A-Za-z])\s+(\S+)$", line.strip())
        if not match:
            continue

        value, sym_type, name = match.groups()
        if sym_type != "A":
            continue
        if not name.endswith(("_OFFSET", "_SIZEOF")):
            continue

        yield name, int(value, 16)


def gen_offset_header(input_file, output_file, nm):
    basename = os.path.basename(output_file.name).upper().replace('.', '_').replace('-', '_')
    include_guard = f"__GEN_{basename}__"
    output_file.write(
        f"""/* THIS FILE IS AUTO GENERATED.  PLEASE DO NOT EDIT.
 *
 * This header file provides macros for the offsets of various structure
 * members.  These offset macros are primarily intended to be used in
 * assembly code.
 */

#ifndef {include_guard}
#define {include_guard}\n\n"""
    )

    try:
        obj = ELFFile(input_file)
    except ELFError:
        obj = None

    if obj is None:
        for name, value in iter_macho_absolute_symbols(input_file.name, nm):
            output_file.write(f"#define {name} 0x{value:x}\n")
    else:
        for sym in get_symbol_table(obj).iter_symbols():
            if isinstance(sym.name, bytes):
                sym.name = str(sym.name, 'ascii')

            if not sym.name.endswith(('_OFFSET', '_SIZEOF')):
                continue
            if sym.entry['st_shndx'] != 'SHN_ABS':
                continue
            if sym.entry['st_info']['bind'] != 'STB_GLOBAL':
                continue

            output_file.write(f"#define {sym.name} 0x{sym.entry['st_value']:x}\n")

    output_file.write(f"\n#endif /* {include_guard} */\n")

    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument("-i", "--input", required=True, help="Input object file")
    parser.add_argument("-o", "--output", required=True, help="Output header file")
    parser.add_argument("--nm", default="nm", help="Symbol table tool, used for Mach-O objects")

    args = parser.parse_args()

    with open(args.input, 'rb') as input_file, open(args.output, 'w') as output_file:
        ret = gen_offset_header(input_file, output_file, args.nm)

    sys.exit(ret)
