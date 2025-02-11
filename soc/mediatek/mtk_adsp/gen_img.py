#!/usr/bin/env python3
# Copyright 2023 The ChromiumOS Authors
# SPDX-License-Identifier: Apache-2.0
import sys
import struct
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

FILE_MAGIC = 0xe463be95

SRAM_START = 0x40000000
SRAM_END   = 0x40040000
DRAM_START = 0x60000000
DRAM_END   = 0x61100000

elf_file = sys.argv[1]
out_file = sys.argv[2]

ef = elftools.elf.elffile.ELFFile(open(elf_file, "rb"))

sram = bytearray()
dram = bytearray()

for seg in ef.iter_segments():
    h = seg.header
    if h.p_type == "PT_LOAD":
        if h.p_paddr in range(SRAM_START, SRAM_END):
            buf = sram
            off = h.p_paddr - SRAM_START
        elif h.p_paddr in range(DRAM_START, DRAM_END):
            buf = dram
            off = h.p_paddr - DRAM_START
        else:
            print(f"Invalid PT_LOAD address {h.p_paddr:x}")
            sys.exit(1)

        dat = seg.data()
        end = off + len(dat)
        if end > len(buf):
            buf.extend(b'\x00' * (end - len(buf)))

        for i in range(len(dat)):
            buf[i + off] = dat[i]

for sec in ef.iter_sections():
    if isinstance(sec, elftools.elf.sections.SymbolTableSection):
        for sym in sec.iter_symbols():
            if sym.name == "mtk_adsp_boot_entry":
                boot_vector = sym.entry['st_value']

assert len(sram) < SRAM_END - SRAM_START
assert len(dram) < DRAM_END - DRAM_START
assert (SRAM_START <= boot_vector < SRAM_END) or (DRAM_START <= boot_vector < DRAM_END)

of = open(out_file, "wb")
of.write(struct.pack("<III", FILE_MAGIC, len(sram), boot_vector))
of.write(sram)
of.write(dram)
