#!/usr/bin/env python3
#
# Copyright (c) 2026 Aerlync Labs Inc.
#
# SPDX-License-Identifier: Apache-2.0
"""Print the CRC-32 of a linked Zephyr image's text region.

Used by the sample's two-pass build. The first pass links with
CONFIG_SAMPLE_IMAGE_CRC=0; this script reads the resulting ELF and prints the
checksum, which the second pass compiles in.

The region is .text alone, bounded by __text_region_start and
__text_region_end. The expected value lives in .rodata, outside that region,
so the second pass cannot change the bytes the checksum covers -- the two
passes converge without iteration and nothing is patched after linking.
"""

import argparse
import sys
import zlib

from elftools.elf.elffile import ELFFile

START_SYMBOL = "__text_region_start"
END_SYMBOL = "__text_region_end"


def symbol_value(elf, name):
    """Address of a symbol, from any symbol table in the ELF."""
    for section in elf.iter_sections():
        if section.header["sh_type"] != "SHT_SYMTAB":
            continue
        matches = section.get_symbol_by_name(name)
        if matches:
            return matches[0]["st_value"]
    raise SystemExit(f"symbol '{name}' not found; is the ELF stripped?")


def file_offset(elf, vaddr, size):
    """File offset of a virtual address range inside one PT_LOAD segment."""
    for segment in elf.iter_segments():
        if segment["p_type"] != "PT_LOAD":
            continue
        base = segment["p_vaddr"]
        if base <= vaddr and vaddr + size <= base + segment["p_filesz"]:
            return segment["p_offset"] + (vaddr - base)
    raise SystemExit(
        f"0x{vaddr:x}..0x{vaddr + size:x} is not contained in a single loadable segment"
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--elf", required=True, help="linked ELF to read")
    parser.add_argument(
        "--verbose", action="store_true", help="also report the region bounds on stderr"
    )
    args = parser.parse_args()

    with open(args.elf, "rb") as stream:
        elf = ELFFile(stream)
        start = symbol_value(elf, START_SYMBOL)
        end = symbol_value(elf, END_SYMBOL)

        if end <= start:
            raise SystemExit(f"empty text region: 0x{start:x}..0x{end:x}")

        stream.seek(file_offset(elf, start, end - start))
        text = stream.read(end - start)

    if len(text) != end - start:
        raise SystemExit("short read of the text region")

    if args.verbose:
        print(
            f"text region 0x{start:x}..0x{end:x} ({end - start} bytes)",
            file=sys.stderr,
        )

    print(f"0x{zlib.crc32(text) & 0xFFFFFFFF:08x}")


if __name__ == "__main__":
    main()
