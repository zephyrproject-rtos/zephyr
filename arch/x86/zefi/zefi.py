#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
import sys
import os.path
import subprocess
import elftools.elf.elffile

ZEPHYR_ELF = sys.argv[1]
ENTRY_SYM = "__start64"
GCC = "gcc"
OBJCOPY = "objcopy"

dir = os.path.dirname(os.path.abspath(__file__))
cfile = dir + "/zefi.c"
ldscript = dir + "/efi.ld"
assert os.path.isfile(cfile)
assert os.path.isfile(ldscript)

#
# Open the ELF file up and find our entry point
#
ef = elftools.elf.elffile.ELFFile(open(ZEPHYR_ELF, "rb"))

symtab = ef.get_section_by_name(".symtab")
entry_addr = symtab.get_symbol_by_name(ENTRY_SYM)[0].entry.st_value

print("Entry point address (symbol: %s) 0x%x" % (ENTRY_SYM, entry_addr))

#
# Parse the ELF file and extract segment data
#

data_blob = b''
data_segs = []
zero_segs = []

for seg in ef.iter_segments():
    h = seg.header
    if h.p_type != "PT_LOAD":
        continue

    assert h.p_memsz >= h.p_filesz
    assert (h.p_vaddr % 8) == 0
    assert (h.p_filesz % 8) == 0
    assert len(seg.data()) == h.p_filesz

    if h.p_filesz > 0:
        sd = seg.data()
        print("%d bytes of data at 0x%x, data offset %d"
              % (len(sd), h.p_vaddr, len(data_blob)))
        data_segs.append((h.p_vaddr, len(sd) / 8, len(data_blob) / 8))
        data_blob = data_blob + sd

    if h.p_memsz > h.p_filesz:
        bytesz = h.p_memsz - h.p_filesz
        if bytesz % 8:
            bytesz += 8 - (bytesz % 8)
        addr = h.p_vaddr + h.p_filesz
        print("%d bytes of zero-fill at 0x%x" % (bytesz, addr))
        zero_segs.append((addr, bytesz / 8))

print(len(data_blob), "bytes of data to include in image")

#
# Emit a C header containing the metadata
#
cf = open("zefi-segments.h", "w")

cf.write("/* GENERATED CODE.  DO NOT EDIT. */\n\n")

cf.write("/* Sizes and offsets specified in 8-byte units.\n")
cf.write(" * All addresses 8-byte aligned.\n")
cf.write(" */\n")

cf.write("struct data_seg { uint64_t addr; uint32_t sz; uint32_t off; };\n\n")

cf.write("static struct data_seg zefi_dsegs[] = {\n")
for s in data_segs:
    cf.write("    { 0x%x, %d, %d },\n"
             % (s[0], s[1], s[2]))
cf.write("};\n\n")

cf.write("struct zero_seg { uint64_t addr; uint32_t sz; };\n\n")

cf.write("static struct zero_seg zefi_zsegs[] = {\n")
for s in zero_segs:
    cf.write("    { 0x%x, %d },\n"
             % (s[0], s[1]))
cf.write("};\n\n")

cf.write("static uintptr_t zefi_entry = 0x%xUL;\n" % (entry_addr))

cf.close()

print("\nMetadata header generated.\n")

#
# Build
#

# First stage ELF binary.  Flag notes:
#  + Stack protector is default on some distros and needs library support
#  + We need pic to enforce that the linker adds no relocations
#  + UEFI can take interrupts on our stack, so no red zone
#  + UEFI API assumes 16-bit wchar_t
cmd = [GCC, "-shared", "-Wall", "-Werror", "-I.",
       "-fno-stack-protector", "-fpic", "-mno-red-zone", "-fshort-wchar",
       "-Wl,-nostdlib", "-T", ldscript, "-o", "zefi.elf", cfile]
print(" ".join(cmd))
subprocess.run(cmd, check = True)

# Extract the .data segment and append our extra blob
cmd = [OBJCOPY, "-O", "binary", "-j", ".data", "zefi.elf", "data.dat"]
print(" ".join(cmd))
subprocess.run(cmd, check = True)

assert (os.stat("data.dat").st_size % 8) == 0
df = open("data.dat", "ab")
df.write(data_blob)
df.close()

# FIXME: this generates warnings about our unused trash section having to be moved to make room.  Set its address far away...
subprocess.run([OBJCOPY, "--update-section", ".data=data.dat",
                "zefi.elf"], check = True)

# Convert it to a PE-COFF DLL.
cmd = [OBJCOPY, "--target=efi-app-x86_64",
       "-j", ".text", "-j", ".reloc", "-j", ".data",
       "zefi.elf", "zephyr.efi"]
print(" ".join(cmd))
subprocess.run(cmd, check = True)

print("\nBuild complete; zephyr.efi wrapper binary is ready")
