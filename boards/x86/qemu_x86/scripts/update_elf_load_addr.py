#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""Update ELF Load Address

This updates the entry and physical load addresses in ELF file
so that QEMU is able to load and run the Zephyr kernel even
though the kernel is linked in virtual address space.

When the Zephyr kernel is linked in virtual address space,
the ELF file only contains virtual addresses which may result
in QEMU loading code and data into non-existent physical memory
if both physical and virtual address space do not start at
the same address. This script modifies the physical addresses
of the load segments so QEMU will place code and data in
physical memory. This also updates the entry address to physical
address so QEMU can jump to it to start the Zephyr kernel.
"""

import argparse
import os
import sys
import struct

from ctypes import create_string_buffer
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection


# ELF Header, load entry offset
ELF_HDR_ENTRY_OFFSET = 0x18


def log(text):
    """Output log text if --verbose is used"""
    if args.verbose:
        sys.stdout.write(os.path.basename(sys.argv[0]) + ": " + text + "\n")


def error(text):
    """Output error message"""
    sys.exit(os.path.basename(sys.argv[0]) + ": " + text)


def extract_all_symbols_from_elf(obj):
    """Get all symbols from the ELF file"""
    all_syms = {}
    for section in obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            for sym in section.iter_symbols():
                all_syms[sym.name] = sym.entry.st_value

    if len(all_syms) == 0:
        raise LookupError("Could not find symbol table")

    return all_syms


def parse_args():
    """Parse command line arguments"""
    global args

    parser = argparse.ArgumentParser()

    parser.add_argument("-k", "--kernel", required=True,
                        help="Zephyr ELF binary")
    parser.add_argument("-o", "--output", required=True,
                        help="Output path")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Print extra debugging information")
    args = parser.parse_args()


def main():
    """Main function"""
    global args
    parse_args()

    if not os.path.exists(args.kernel):
        error("{0} does not exist.".format(args.kernel))

    elf_fd = open(args.kernel, "rb")

    # Create a modifiable byte stream
    raw_elf = elf_fd.read()
    output = create_string_buffer(raw_elf)

    elf = ELFFile(elf_fd)

    if not elf.has_dwarf_info():
        error("ELF file has no DWARF information")

    if elf.num_segments() == 0:
        error("ELF file has no program header table")

    syms = extract_all_symbols_from_elf(elf)

    vm_base = syms["CONFIG_KERNEL_VM_BASE"]
    vm_size = syms["CONFIG_KERNEL_VM_SIZE"]

    sram_base = syms["CONFIG_SRAM_BASE_ADDRESS"]
    sram_size = syms["CONFIG_SRAM_SIZE"] * 1024

    vm_offset = syms["CONFIG_KERNEL_VM_OFFSET"]
    sram_offset = syms.get("CONFIG_SRAM_OFFSET", 0)

    #
    # Calculate virtual-to-physical address translation
    #
    virt_to_phys_offset = (sram_base + sram_offset) - (vm_base + vm_offset)

    log("Virtual address space: 0x%x - 0x%x size 0x%x (offset 0x%x)" %
        (vm_base, vm_base + vm_size, vm_size, vm_offset))
    log("Physical address space: 0x%x - 0x%x size 0x%x (offset 0x%x)" %
        (sram_base, sram_base + sram_size, sram_size, sram_offset))

    #
    # Update the entry address in header
    #
    if elf.elfclass == 32:
        load_entry_type = "I"
    else:
        load_entry_type = "Q"

    entry_virt = struct.unpack_from(load_entry_type, output, ELF_HDR_ENTRY_OFFSET)[0]
    entry_phys = entry_virt + virt_to_phys_offset
    struct.pack_into(load_entry_type, output, ELF_HDR_ENTRY_OFFSET, entry_phys)

    log("Entry Address: 0x%x -> 0x%x" % (entry_virt, entry_phys))

    #
    # Update load address in program header segments
    #

    # Program header segment offset from beginning of file
    ph_off = elf.header['e_phoff']

    # ph_seg_type: segment type and other fields before virtual address
    # ph_seg_addr: virtual and phyiscal addresses
    # ph_seg_whole: whole segment
    if elf.elfclass == 32:
        ph_seg_type = "II"
        ph_seg_addr = "II"
        ph_seg_whole = "IIIIIIII"
    else:
        ph_seg_type = "IIQ"
        ph_seg_addr = "QQ"
        ph_seg_whole = "IIQQQQQQ"

    # Go through all segments
    for ph_idx in range(elf.num_segments()):
        seg_off = ph_off + struct.calcsize(ph_seg_whole) * ph_idx

        seg_type, _ = struct.unpack_from(ph_seg_type, output, seg_off)

        # Only process LOAD segments
        if seg_type != 0x01:
            continue

        # Add offset to get to the addresses
        addr_off = seg_off + struct.calcsize(ph_seg_type)

        # Grab virtual and physical addresses
        seg_vaddr, seg_paddr = struct.unpack_from(ph_seg_addr, output, addr_off)

        # Apply virt-to-phys offset so it will load into
        # physical address
        seg_paddr_new = seg_vaddr + virt_to_phys_offset

        log("Segment %d: physical address 0x%x -> 0x%x" % (ph_idx, seg_paddr, seg_paddr_new))

        # Put the addresses back
        struct.pack_into(ph_seg_addr, output, addr_off, seg_vaddr, seg_paddr_new)

    out_fd = open(args.output, "wb")
    out_fd.write(output)
    out_fd.close()

    elf_fd.close()

if __name__ == "__main__":
    main()
