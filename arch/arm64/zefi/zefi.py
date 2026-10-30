#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
# Copyright (c) 2026 KylinSoft Corporation
# SPDX-License-Identifier: Apache-2.0
"""Build zephyr.efi from zephyr.elf using the AArch64 zefi stub."""

import argparse
import os
import subprocess
import sys

import elftools.elf.elffile

ENTRY_SYM = "__start"

args = None


def verbose(msg):
    if args.verbose:
        print(msg)


def build_elf(elf_file, include_dirs):
    source_file = os.path.abspath(args.source_file)
    linker_script = os.path.abspath(args.linker_script)
    output_file = os.path.abspath(args.output)
    stub_elf = os.path.abspath(args.stub_elf)
    data_file = os.path.abspath(args.data_file)
    segments_header = os.path.abspath(args.segments_header)

    assert os.path.isfile(source_file)
    assert os.path.isfile(linker_script)

    with open(elf_file, "rb") as fp:
        ef = elftools.elf.elffile.ELFFile(fp)
        symtab = ef.get_section_by_name(".symtab")
        entry_addr = symtab.get_symbol_by_name(ENTRY_SYM)[0].entry.st_value

    verbose(f"Entry point address (symbol: {ENTRY_SYM}) 0x{entry_addr:x}")

    data_blob = b""
    data_segs = []
    zero_segs = []

    with open(elf_file, "rb") as fp:
        ef = elftools.elf.elffile.ELFFile(fp)
        for seg in ef.iter_segments():
            h = seg.header
            if h.p_type != "PT_LOAD":
                continue

            assert h.p_memsz >= h.p_filesz
            assert len(seg.data()) == h.p_filesz

            if h.p_filesz > 0:
                sd = seg.data()
                verbose(f"{len(sd)} bytes of data at 0x{h.p_vaddr:x}, data offset {len(data_blob)}")
                data_segs.append((h.p_vaddr, len(sd), len(data_blob)))
                data_blob = data_blob + sd

            if h.p_memsz > h.p_filesz:
                bytesz = h.p_memsz - h.p_filesz
                addr = h.p_vaddr + h.p_filesz
                verbose(f"{bytesz} bytes of zero-fill at 0x{addr:x}")
                zero_segs.append((addr, bytesz))

    verbose(f"{len(data_blob)} bytes of data to include in image")

    with open(segments_header, "w") as cf:
        cf.write("/* GENERATED CODE.  DO NOT EDIT. */\n\n")
        cf.write("struct data_seg { uint64_t addr; uint32_t sz; uint32_t off; };\n\n")
        cf.write("static struct data_seg zefi_dsegs[] = {\n")
        for s in data_segs:
            cf.write(f"    {{ 0x{s[0]:x}, {s[1]}, {s[2]} }},\n")
        cf.write("};\n\n")
        cf.write("struct zero_seg { uint64_t addr; uint32_t sz; };\n\n")
        cf.write("static struct zero_seg zefi_zsegs[] = {\n")
        for s in zero_segs:
            cf.write(f"    {{ 0x{s[0]:x}, {s[1]} }},\n")
        cf.write("};\n\n")
        cf.write(f"static uintptr_t zefi_entry = 0x{entry_addr:x}UL;\n")

    verbose("Metadata header generated.")

    includes = []
    for include_dir in include_dirs:
        includes.extend(["-I", include_dir])
    includes.extend(
        [
            "-I",
            os.path.dirname(source_file),
            "-I",
            os.path.dirname(segments_header),
        ]
    )

    # Zephyr SDK aarch64-gcc pulls crt0 with -shared; link freestanding instead.
    cmd = (
        [
            args.compiler,
            "-nostdlib",
            "-nostartfiles",
            "-static",
            "-Wl,-e,efi_entry",
            "-Wall",
            "-Werror",
            "-I.",
        ]
        + includes
        + [
            "-fno-stack-protector",
            "-fpic",
            "-fshort-wchar",
            "-mgeneral-regs-only",
            "-ffreestanding",
            "-Wl,-nostdlib",
            "-T",
            linker_script,
            "-o",
            stub_elf,
            source_file,
        ]
    )
    verbose(" ".join(cmd))
    subprocess.run(cmd, check=True)

    cmd = [args.objcopy, "-O", "binary", "-j", ".data", stub_elf, data_file]
    verbose(" ".join(cmd))
    subprocess.run(cmd, check=True)

    assert (os.stat(data_file).st_size % 8) == 0
    with open(data_file, "ab") as df:
        df.write(data_blob)

    subprocess.run([args.objcopy, "--update-section", f".data={data_file}", stub_elf], check=True)

    cmd = [
        args.objcopy,
        "--target=efi-app-aarch64",
        "-j",
        ".text",
        "-j",
        ".reloc",
        "-j",
        ".data",
        stub_elf,
        output_file,
    ]
    verbose(" ".join(cmd))
    subprocess.run(cmd, check=True)

    verbose("Build complete; zephyr.efi wrapper binary is ready")


def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument("-c", "--compiler", required=True, help="Compiler to be used")
    parser.add_argument("-o", "--objcopy", required=True, help="objcopy to be used")
    parser.add_argument("-f", "--elf-file", required=True, help="Input zephyr.elf")
    parser.add_argument("--source-file", required=True, help="AArch64 EFI stub source")
    parser.add_argument("--linker-script", required=True, help="EFI stub linker script")
    parser.add_argument("--output", required=True, help="Output EFI application")
    parser.add_argument("--stub-elf", required=True, help="Intermediate EFI stub ELF")
    parser.add_argument("--data-file", required=True, help="Intermediate EFI data image")
    parser.add_argument(
        "--segments-header", required=True, help="Generated Zephyr segment metadata header"
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    parser.add_argument(
        "-i", "--includes", required=True, nargs="+", help="Zephyr include directories"
    )

    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    verbose(f"Working on {args.elf_file} ...")
    try:
        build_elf(args.elf_file, args.includes)
    except Exception as exc:
        print(exc, file=sys.stderr)
        sys.exit(1)
