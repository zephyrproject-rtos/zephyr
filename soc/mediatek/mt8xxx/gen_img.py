#!/usr/bin/env python3
# Copyright 2023 The ChromiumOS Authors
# SPDX-License-Identifier: Apache-2.0
import struct
import sys

import elftools.elf.elffile
import elftools.elf.sections

# Converts a zephyr.elf file into an extremely simple "image format"
# for device loading.  Really we should just load the ELF file
# directly, but the python ChromeOS test image lacks elftools.  Longer
# term we should probably just use rimage, but that's significantly
# harder to parse.
#
# Format:
#
# 1. Three LE 32 bit words: MagicNumber, SRAM len, BootAddress
# 2. Two byte arrays: SRAM (length specified), and DRAM (remainder of file)
#
# No padding or uninterpreted bytes.

FILE_MAGIC = 0xE463BE95

elf_file = sys.argv[1]
out_file = sys.argv[2]

sram = bytearray()
dram = bytearray()

# Returns the offset of a segment within the sram region, or -1 if it
# doesn't appear to be SRAM.  SRAM is mapped differently for different
# SOCs, but it's always a <=1M region in 0x4xxxxxxx.  Just use what we
# get, but validate that it fits.
sram_block = 0


def sram_off(addr):
    global sram_block
    if addr < 0x40000000 or addr >= 0x50000000:
        return -1
    block = addr & ~0xFFFFF
    assert sram_block in (0, block)

    sram_block = block
    off = addr - sram_block
    assert off < 0x100000
    return off


# Similar heuristics: current platforms put DRAM either at 0x60000000
# or 0x90000000 with no more than 16M of range
def dram_off(addr):
    if (addr >> 28 not in [6, 9]) or (addr & 0x0F000000 != 0):
        return -1
    return addr & 0xFFFFFF


def read_elf(efile):
    ef = elftools.elf.elffile.ELFFile(efile)

    for seg in ef.iter_segments():
        h = seg.header
        if h.p_type == "PT_LOAD":
            soff = sram_off(h.p_paddr)
            doff = dram_off(h.p_paddr)
            if soff >= 0:
                buf = sram
                off = soff
            elif doff >= 0:
                buf = dram
                off = doff
            else:
                print(f"Invalid PT_LOAD address {h.p_paddr:x}")
                sys.exit(1)

            dat = seg.data()
            end = off + len(dat)
            if end > len(buf):
                buf.extend(b'\x00' * (end - len(buf)))

            # pylint: disable=consider-using-enumerate
            for i in range(len(dat)):
                buf[i + off] = dat[i]

    for sec in ef.iter_sections():
        if isinstance(sec, elftools.elf.sections.SymbolTableSection):
            for sym in sec.iter_symbols():
                if sym.name == "mtk_adsp_boot_entry":
                    boot_vector = sym.entry['st_value']

    with open(out_file, "wb") as of:
        of.write(struct.pack("<III", FILE_MAGIC, len(sram), boot_vector))
        of.write(sram)
        of.write(dram)


with open(elf_file, "rb") as f:
    read_elf(f)
