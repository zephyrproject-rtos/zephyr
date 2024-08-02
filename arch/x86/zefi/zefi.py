#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import os.path
import subprocess
import elftools.elf.elffile
import argparse

ENTRY_SYM = "__start64"

def verbose(msg):
    if args.verbose:
        print(msg)

def build_elf(elf_file, include_dirs):
    base_dir = os.path.dirname(os.path.abspath(__file__))

    cfile = os.path.join(base_dir, "zefi.c")
    ldscript = os.path.join(base_dir, "efi.ld")

    assert os.path.isfile(cfile)
    assert os.path.isfile(ldscript)

    #
    # Open the ELF file up and find our entry point
    #
    fp = open(elf_file, "rb")
    ef = elftools.elf.elffile.ELFFile(fp)

    symtab = ef.get_section_by_name(".symtab")
    entry_addr = symtab.get_symbol_by_name(ENTRY_SYM)[0].entry.st_value

    verbose("Entry point address (symbol: %s) 0x%x" % (ENTRY_SYM, entry_addr))

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
        assert len(seg.data()) == h.p_filesz

        if h.p_filesz > 0:
            sd = seg.data()
            verbose("%d bytes of data at 0x%x, data offset %d"
                % (len(sd), h.p_vaddr, len(data_blob)))
            data_segs.append((h.p_vaddr, len(sd), len(data_blob)))
            data_blob = data_blob + sd

        if h.p_memsz > h.p_filesz:
            bytesz = h.p_memsz - h.p_filesz
            addr = h.p_vaddr + h.p_filesz
            verbose("%d bytes of zero-fill at 0x%x" % (bytesz, addr))
            zero_segs.append((addr, bytesz))

    verbose(f"{len(data_blob)} bytes of data to include in image")

    #
    # Emit a C header containing the metadata
    #
    cf = open("zefi-segments.h", "w")

    cf.write("/* GENERATED CODE.  DO NOT EDIT. */\n\n")

    cf.write("/* Sizes and offsets specified in 4-byte units.\n")
    cf.write(" * All addresses 4-byte aligned.\n")
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

    verbose("Metadata header generated.")

    #
    # Build
    #

    # First stage ELF binary.  Flag notes:
    #  + Stack protector is default on some distros and needs library support
    #  + We need pic to enforce that the linker adds no relocations
    #  + UEFI can take interrupts on our stack, so no red zone
    #  + UEFI API assumes 16-bit wchar_t
    includes = []
    for include_dir in include_dirs:
        includes.extend(["-I", include_dir])
    cmd = ([args.compiler, "-shared", "-Wall", "-Werror", "-I."] + includes +
           ["-fno-stack-protector", "-fpic", "-mno-red-zone", "-fshort-wchar",
            "-Wl,-nostdlib", "-T", ldscript, "-o", "zefi.elf", cfile])
    verbose(" ".join(cmd))
    subprocess.run(cmd, check = True)

    # Extract the .data segment and append our extra blob
    cmd = [args.objcopy, "-O", "binary", "-j", ".data", "zefi.elf", "data.dat"]
    verbose(" ".join(cmd))
    subprocess.run(cmd, check = True)

    assert (os.stat("data.dat").st_size % 8) == 0
    df = open("data.dat", "ab")
    df.write(data_blob)
    df.close()

    # FIXME: this generates warnings about our unused trash section having to be moved to make room.  Set its address far away...
    subprocess.run([args.objcopy, "--update-section", ".data=data.dat",
                    "zefi.elf"], check = True)

    # Convert it to a PE-COFF DLL.
    cmd = [args.objcopy, "--target=efi-app-x86_64",
        "-j", ".text", "-j", ".reloc", "-j", ".data",
        "zefi.elf", "zephyr.efi"]
    verbose(" ".join(cmd))
    subprocess.run(cmd, check = True)

    verbose("Build complete; zephyr.efi wrapper binary is ready")


def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument("-c", "--compiler", required=True, help="Compiler to be used")
    parser.add_argument("-o", "--objcopy", required=True, help="objcopy to be used")
    parser.add_argument("-f", "--elf-file", required=True, help="Input file")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    parser.add_argument("-i", "--includes", required=True, nargs="+",
                        help="Zephyr base include directories")

    return parser.parse_args()

if __name__ == "__main__":

    args = parse_args()
    verbose(f"Working on {args.elf_file} with {args.includes}...")
    build_elf(args.elf_file, args.includes)
