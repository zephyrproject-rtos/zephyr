#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import sys
import struct
import os
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

gdt_pd_fmt = "<HIH"

FLAGS_GRAN = 1 << 7 # page granularity
ACCESS_EX = 1 << 3  # executable
ACCESS_DC = 1 << 2  # direction/conforming
ACCESS_RW = 1 << 1  # read or write permission

# 6 byte pseudo descriptor, but we're going to actually use this as the
# zero descriptor and return 8 bytes
def create_gdt_pseudo_desc(addr, size):
    # ...and take back one byte for the Intel god whose Ark this is...
    size = size - 1
    return struct.pack(gdt_pd_fmt, size, addr, 0)


# Limit argument always in bytes
def chop_base_limit(base, limit):
    base_lo = base & 0xFFFF
    base_mid = (base >> 16) & 0xFF
    base_hi = (base >> 24) & 0xFF

    limit_lo = limit & 0xFFFF
    limit_hi = (limit >> 16) & 0xF

    return (base_lo, base_mid, base_hi, limit_lo, limit_hi)


gdt_ent_fmt = "<HHBBBB"

def create_code_data_entry(base, limit, dpl, flags, access):
    base_lo, base_mid, base_hi, limit_lo, limit_hi = chop_base_limit(base,
            limit)

    # This is a valid descriptor
    present = 1

    # 32-bit protected mode
    size = 1

    # 1 = code or data, 0 = system type
    desc_type = 1

    # Just set accessed to 1 already so the CPU doesn't need it update it,
    # prevents freakouts if the GDT is in ROM, we don't care about this
    # bit in the OS
    accessed = 1

    access = access | (present << 7) | (desc_type << 4) | accessed
    flags = flags | (size << 6) | limit_hi

    return struct.pack(gdt_ent_fmt, limit_lo, base_lo, base_mid,
                       access, flags, base_hi)


def get_symbols(obj):
    for section in obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            return {sym.name: sym.entry.st_value
                    for sym in section.iter_symbols()}

    raise LookupError("Could not find symbol table")


def parse_args():
    global args
    parser = argparse.ArgumentParser(description = __doc__,
            formatter_class = argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-k", "--kernel", required=True,
            help="Zephyr kernel image")
    parser.add_argument("-v", "--verbose", action="store_true",
            help="Print extra debugging information")
    parser.add_argument("-o", "--output-gdt", required=True,
            help="output GDT binary")
    args = parser.parse_args()


def main():
    parse_args()

    with open(args.kernel, "rb") as fp:
        kernel = ELFFile(fp)
        syms = get_symbols(kernel)

    num_entries = 3

    gdt_base = syms["_gdt"]

    with open(args.output_gdt, "wb") as fp:
        # The pseudo descriptor is stuffed into the NULL descriptor
        # since the CPU never looks at it
        fp.write(create_gdt_pseudo_desc(gdt_base, num_entries * 8))

        # Selector 0x08: code descriptor
        fp.write(create_code_data_entry(0, 0xFFFFF, 0,
                 FLAGS_GRAN, ACCESS_EX | ACCESS_RW))

        # Selector 0x10: data descriptor
        fp.write(create_code_data_entry(0, 0xFFFFF, 0,
                 FLAGS_GRAN, ACCESS_RW))

if __name__ == "__main__":
    main()

