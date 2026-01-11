#!/usr/bin/env python3
#
# Copyright (c) 2025 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import struct

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection


def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument("-k", "--kernel", required=False, help="Zephyr kernel image")
    parser.add_argument("-o", "--header-output", required=False, help="Header output file")
    parser.add_argument("-c", "--config", required=False, help="Configuration file (.config)")
    parser.add_argument(
        "-a",
        "--arch",
        required=False,
        help="Architecture to generate shadow stack for",
        choices=["x86", "x86_64"],
        default="x86",
    )
    args = parser.parse_args()


def get_symbols(obj):
    for section in obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            return {sym.name: sym for sym in section.iter_symbols()}

    raise LookupError("Could not find symbol table")


shstk_irq_top_fmt = "<Q"


def generate_initialized_irq_array64(sym, section_data, section_addr, isr_depth):
    offset = sym["st_value"] - section_addr
    # First four bytes have the number of members of the array
    nmemb = int.from_bytes(section_data[offset : offset + 4], "little") * isr_depth
    stack_size = (int)(sym["st_size"] / nmemb)

    # Top of shadow stack is on the form:
    # [-1] = shadow stack supervisor token

    output = bytearray(sym["st_size"])
    for i in range(nmemb):
        token = sym["st_value"] + stack_size * (i + 1) - 8

        struct.pack_into(shstk_irq_top_fmt, output, stack_size * (i + 1) - 8, token)

    return output


shstk_top_fmt = "<QQQQ"


def generate_initialized_array64(sym, section_data, section_addr, thread_entry):
    offset = sym["st_value"] - section_addr
    # First four bytes have the number of members of the array
    nmemb = int.from_bytes(section_data[offset : offset + 4], "little")
    if nmemb == 0:
        # CONFIG_DYNAMIC_THREAD_POOL_SIZE can be zero - in which case isn't
        # used. To allow the static initialization, we make the corresponding shadow stack
        # have one item. Here, we recognize it by having 0 members. But we already
        # wasted the space for this. Sigh...
        return bytearray(sym["st_size"])
    stack_size = (int)(sym["st_size"] / nmemb)

    # Top of shadow stack is on the form:
    # [-5] = shadow stack token, pointing to [-4]
    # [-4] = previous SSP, pointing to [-1]
    # [-3] = z_thread_entry
    # [-2] = X86_KERNEL_CS

    output = bytearray(sym["st_size"])
    for i in range(nmemb):
        end = sym["st_value"] + stack_size * (i + 1)
        token = end - 8 * 4 + 1
        prev_ssp = end - 8
        cs = 0x18  # X86_KERNEL_CS

        struct.pack_into(
            shstk_top_fmt,
            output,
            stack_size * (i + 1) - 8 * 5,
            token,
            prev_ssp,
            thread_entry["st_value"],
            cs,
        )

    return output


shstk_top_fmt32 = "<QI"


def generate_initialized_array32(sym, section_data, section_addr, thread_entry):
    offset = sym["st_value"] - section_addr
    # First four bytes have the number of members of the array
    nmemb = int.from_bytes(section_data[offset : offset + 4], "little")
    if nmemb == 0:
        # See comment on generate_initialized_array64
        return bytearray(sym["st_size"])
    stack_size = (int)(sym["st_size"] / nmemb)

    # Top of shadow stack is on the form:
    # [-4] = shadow stack token, pointing to [-2]
    # [-3] = 0 - high order bits of token
    # [-2] = z_thread_entry/z_x86_thread_entry_wrapper

    output = bytearray(sym["st_size"])
    for i in range(nmemb):
        end = sym["st_value"] + stack_size * (i + 1)
        token = end - 8

        struct.pack_into(
            shstk_top_fmt32,
            output,
            stack_size * (i + 1) - 4 * 4,
            token,
            thread_entry["st_value"],
        )

    return output


def generate_initialized_array(sym, section_data, section_addr, thread_entry):
    if args.arch == "x86":
        return generate_initialized_array32(sym, section_data, section_addr, thread_entry)

    return generate_initialized_array64(sym, section_data, section_addr, thread_entry)


def patch_elf():
    with open(args.kernel, "r+b") as elf_fp:
        kernel = ELFFile(elf_fp)
        section = kernel.get_section_by_name(".x86shadowstack.arr")
        syms = get_symbols(kernel)
        thread_entry = syms["z_thread_entry"]
        if args.arch == "x86" and syms["CONFIG_X86_DEBUG_INFO"].entry.st_value:
            thread_entry = syms["z_x86_thread_entry_wrapper"]

        updated_section = bytearray()
        shstk_arr_syms = [
            sym[1]
            for sym in syms.items()
            if sym[0].startswith("__") and sym[0].endswith("_shstk_arr")
        ]
        shstk_arr_syms.sort(key=lambda x: x["st_value"])
        section_data = section.data()

        for sym in shstk_arr_syms:
            if sym.name == "__z_interrupt_stacks_shstk_arr" and args.arch == "x86_64":
                isr_depth = syms["CONFIG_ISR_DEPTH"].entry.st_value
                out = generate_initialized_irq_array64(
                    sym, section_data, section["sh_addr"], isr_depth
                )
            else:
                out = generate_initialized_array(
                    sym, section_data, section["sh_addr"], thread_entry
                )

            updated_section += out

        elf_fp.seek(section["sh_offset"])
        elf_fp.write(updated_section)


def generate_header():
    if args.config is None:
        raise ValueError("Configuration file is required to generate header")

    isr_depth = stack_size = alignment = hw_stack_percentage = hw_stack_min_size = None

    with open(args.config) as config_fp:
        config_lines = config_fp.readlines()

    for line in config_lines:
        if line.startswith("CONFIG_ISR_DEPTH="):
            isr_depth = int(line.split("=")[1].strip())
        if line.startswith("CONFIG_ISR_STACK_SIZE="):
            stack_size = int(line.split("=")[1].strip())
        if line.startswith("CONFIG_X86_CET_SHADOW_STACK_ALIGNMENT="):
            alignment = int(line.split("=")[1].strip())
        if line.startswith("CONFIG_HW_SHADOW_STACK_PERCENTAGE_SIZE="):
            hw_stack_percentage = int(line.split("=")[1].strip())
        if line.startswith("CONFIG_HW_SHADOW_STACK_MIN_SIZE="):
            hw_stack_min_size = int(line.split("=")[1].strip())

    if isr_depth is None:
        raise ValueError("Missing CONFIG_ISR_DEPTH in configuration file")
    if stack_size is None:
        raise ValueError("Missing CONFIG_ISR_STACK_SIZE in configuration file")
    if alignment is None:
        raise ValueError("Missing CONFIG_X86_CET_SHADOW_STACK_ALIGNMENT in configuration file")
    if hw_stack_percentage is None:
        raise ValueError("Missing CONFIG_HW_SHADOW_STACK_PERCENTAGE_SIZE in configuration file")
    if hw_stack_min_size is None:
        raise ValueError("Missing CONFIG_HW_SHADOW_STACK_MIN_SIZE in configuration file")

    stack_size = int(stack_size * (hw_stack_percentage / 100))
    stack_size = int((stack_size + alignment - 1) / alignment) * alignment
    stack_size = max(stack_size, hw_stack_min_size)
    stack_size = int(stack_size / isr_depth)

    with open(args.header_output, "w") as header_fp:
        header_fp.write(f"#define X86_CET_IRQ_SHADOW_SUBSTACK_SIZE {stack_size}\n")


def main():
    parse_args()

    if args.kernel is not None:
        patch_elf()

    if args.header_output is not None:
        generate_header()


if __name__ == "__main__":
    main()
