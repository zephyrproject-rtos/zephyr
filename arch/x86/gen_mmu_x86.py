#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""Generate MMU page tables for x86 CPUs.

This script generates 64-bit PAE style MMU page tables for x86.
Even though x86 is a 32-bit target, we use this type of page table
to support the No-Execute (NX) bit. Please consult the IA
Architecture SW Developer Manual, volume 3, chapter 4 for more
details on this data structure.

The script takes as input the zephyr_prebuilt.elf kernel binary,
which is a link of the Zephyr kernel without various build-time
generated data structures (such as the MMU tables) inserted into it.
The build cannot easily predict how large these tables will be,
so it is important that these MMU tables be inserted at the very
end of memory.

Of particular interest is the "mmulist" section, which is a
table of memory region access policies set in code by instances
of MMU_BOOT_REGION() macros. The set of regions defined here
specifies the boot-time configuration of the page tables.

The output of this script is a linked set of page tables, page
directories, and a page directory pointer table, which gets linked
into the final Zephyr binary, reflecting the access policies
read in the "mmulist" section. Any memory ranges not specified
in "mmulist" are marked non-present.

If Kernel Page Table Isolation (CONFIG_X86_KPTI) is enabled, this
script additionally outputs a second set of page tables intended
for use by user threads running in Ring 3. These tables have the
same policy as the kernel's set of page tables with one crucial
difference: any pages not accessible to user mode threads are not
marked 'present', preventing Meltdown-style side channel attacks
from reading their contents.
"""

import os
import sys
import struct
import parser
from collections import namedtuple
import ctypes
import argparse
import re
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

mmu_region_details = namedtuple("mmu_region_details",
                                "pde_index page_entries_info")

valid_pages_inside_pde = namedtuple("valid_pages_inside_pde", "start_addr size \
                                pte_valid_addr_start \
                                pte_valid_addr_end \
                                permissions")

mmu_region_details_pdpt = namedtuple("mmu_region_details_pdpt",
                                     "pdpte_index pd_entries")

# Constants
PAGE_ENTRY_PRESENT = 1
PAGE_ENTRY_READ_WRITE = 1 << 1
PAGE_ENTRY_USER_SUPERVISOR = 1 << 2
PAGE_ENTRY_XD = 1 << 63

# Struct formatters
struct_mmu_regions_format = "<IIQ"
header_values_format = "<II"
page_entry_format = "<Q"

entry_counter = 0
def print_code(val):
    global entry_counter

    if not val & PAGE_ENTRY_PRESENT:
        ret = '.'
    else:
        if (val & PAGE_ENTRY_READ_WRITE):
            # Writable page
            if (val & PAGE_ENTRY_XD):
                # Readable, writeable, not executable
                ret = 'w'
            else:
                # Readable, writable, executable
                ret = 'a'
        else:
            # Read-only
            if (val & PAGE_ENTRY_XD):
                # Read-only
                ret = 'r'
            else:
                # Readable, executable
                ret = 'x'

        if (val & PAGE_ENTRY_USER_SUPERVISOR):
            # User accessible pages are capital letters
            ret = ret.upper()

    sys.stdout.write(ret)
    entry_counter = entry_counter + 1
    if (entry_counter == 128):
        sys.stdout.write("\n")
        entry_counter = 0

class PageMode_PAE:
    total_pages = 511

    size_addressed_per_pde = (512 * 4096)  # 2MB In Bytes
    size_addressed_per_pdpte = (512 * size_addressed_per_pde)  # In Bytes
    list_of_pdpte = {}

    def __init__(self, pd_start_addr, mem_regions, syms, kpti):
        self.pd_start_addr = pd_start_addr
        self.mem_regions = mem_regions
        self.pd_tables_list = []
        self.output_offset = 0
        self.kpti = kpti
        self.syms = syms

        for i in range(4):
            self.list_of_pdpte[i] = mmu_region_details_pdpt(pdpte_index=i,
                                                            pd_entries={})
        self.populate_required_structs()
        self.pdpte_create_binary_file()
        self.page_directory_create_binary_file()
        self.page_table_create_binary_file()

    # return the pdpte number for the give address
    def get_pdpte_number(self, value):
        return (value >> 30) & 0x3

    # return the page directory number for the give address
    def get_pde_number(self, value):
        return (value >> 21) & 0x1FF

    # return the page table number for the given address
    def get_pte_number(self, value):
        return (value >> 12) & 0x1FF

    def get_number_of_pd(self):
        return len(self.get_pdpte_list())

    def get_pdpte_list(self):
        return list({temp[0] for temp in self.pd_tables_list})

    # the return value will have the page address and it is assumed to be a 4096
    # boundary.hence the output of this API will be a 20bit address of the page
    # table
    def address_of_page_table(self, pdpte, page_table_number):
        # first page given to page directory pointer
        # and 2nd page till 5th page are used for storing the page directories.

        # set the max pdpte used. this tells how many pd are needed after
        # that we start keeping the pt
        PT_start_addr = self.get_number_of_pd() * 4096 +\
            self.pd_start_addr + 4096
        return (PT_start_addr +
                (self.pd_tables_list.index([pdpte, page_table_number]) *
                 4096) >> 12)

    def get_binary_pde_value(self, pdpte, value):
        perms = value.page_entries_info[0].permissions

        present = PAGE_ENTRY_PRESENT

        read_write = check_bits(perms, [1, 29]) << 1
        user_mode = check_bits(perms, [2, 28]) << 2

        page_table = self.address_of_page_table(pdpte, value.pde_index) << 12
        return present | read_write | user_mode | page_table

    def get_binary_pte_value(self, value, pde, pte, perm_for_pte):
        read_write = perm_for_pte & PAGE_ENTRY_READ_WRITE
        user_mode = perm_for_pte & PAGE_ENTRY_USER_SUPERVISOR
        xd = perm_for_pte & PAGE_ENTRY_XD

        # This points to the actual memory in the HW
        # totally 20 bits to rep the phy address
        # first 2bits is from pdpte then 9bits is the number got from pde and
        # next 9bits is pte
        page_table = ((value.pdpte_index << 18) | (pde << 9) | pte) << 12

        if self.kpti:
            if user_mode:
                present = PAGE_ENTRY_PRESENT
            else:
                if page_table == self.syms['z_shared_kernel_page_start']:
                    present = PAGE_ENTRY_PRESENT
                else:
                    present = 0
        else:
            present = PAGE_ENTRY_PRESENT

        binary_value = (present | read_write | user_mode | xd)

        # L1TF mitigation: map non-present pages to the NULL page
        if present:
            binary_value |= page_table;

        return binary_value

    def clean_up_unused_pdpte(self):
        self.list_of_pdpte = {key: value for key, value in
                              self.list_of_pdpte.items()
                              if value.pd_entries != {}}

    # update the tuple values for the memory regions needed
    def set_pde_pte_values(self, pdpte, pde_index, address, mem_size,
                           pte_valid_addr_start, pte_valid_addr_end, perm):

        pages_tuple = valid_pages_inside_pde(
            start_addr=address,
            size=mem_size,
            pte_valid_addr_start=pte_valid_addr_start,
            pte_valid_addr_end=pte_valid_addr_end,
            permissions=perm)

        mem_region_values = mmu_region_details(pde_index=pde_index,
                                               page_entries_info=[])

        mem_region_values.page_entries_info.append(pages_tuple)

        if pde_index in self.list_of_pdpte[pdpte].pd_entries.keys():
            # this step adds the new page info to the exsisting pages info
            self.list_of_pdpte[pdpte].pd_entries[pde_index].\
                page_entries_info.append(pages_tuple)
        else:
            self.list_of_pdpte[pdpte].pd_entries[pde_index] = mem_region_values

    def populate_required_structs(self):
        for start, size, flags in self.mem_regions:
            pdpte_index = self.get_pdpte_number(start)
            pde_index = self.get_pde_number(start)
            pte_valid_addr_start = self.get_pte_number(start)

            # Get the end of the page table entries
            # Since a memory region can take up only a few entries in the Page
            # table, this helps us get the last valid PTE.
            pte_valid_addr_end = self.get_pte_number(start +
                                                     size - 1)

            mem_size = size

            # In-case the start address aligns with a page table entry other
            # than zero and the mem_size is greater than (1024*4096) i.e 4MB
            # in case where it overflows the currenty PDE's range then limit the
            # PTE to 1024 and so make the mem_size reflect the actual size
            # taken up in the current PDE
            if (size + (pte_valid_addr_start * 4096)) >= \
               (self.size_addressed_per_pde):

                pte_valid_addr_end = self.total_pages
                mem_size = (((self.total_pages + 1) -
                             pte_valid_addr_start) * 4096)

            self.set_pde_pte_values(pdpte_index,
                                    pde_index,
                                    start,
                                    mem_size,
                                    pte_valid_addr_start,
                                    pte_valid_addr_end,
                                    flags)

            if [pdpte_index, pde_index] not in self.pd_tables_list:
                self.pd_tables_list.append([pdpte_index, pde_index])

            # IF the current pde couldn't fit the entire requested region
            # size then there is a need to create new PDEs to match the size.
            # Here the overflow_size represents the size that couldn't be fit
            # inside the current PDE, this is will now to used to create a new
            # PDE/PDEs so the size remaining will be
            # requested size - allocated size(in the current PDE)

            overflow_size = size - mem_size

            # create all the extra PDEs needed to fit the requested size
            # this loop starts from the current pde till the last pde that is
            # needed the last pde is calcualted as the (start_addr + size) >>
            # 22
            if overflow_size != 0:
                for extra_pdpte in range(pdpte_index,
                                         self.get_pdpte_number(start +
                                                               size) + 1):
                    for extra_pde in range(pde_index + 1, self.get_pde_number(
                            start + size) + 1):

                        # new pde's start address
                        # each page directory entry has a addr range of
                        # (1024 *4096) thus the new PDE start address is a
                        # multiple of that number
                        extra_pde_start_address = (
                            extra_pde * (self.size_addressed_per_pde))

                        # the start address of and extra pde will always be 0
                        # and the end address is calculated with the new
                        # pde's start address and the overflow_size
                        extra_pte_valid_addr_end = (
                            self.get_pte_number(extra_pde_start_address +
                                                overflow_size - 1))

                        # if the overflow_size couldn't be fit inside this new
                        # pde then need another pde and so we now need to limit
                        # the end of the PTE to 1024 and set the size of this
                        # new region to the max possible
                        extra_region_size = overflow_size
                        if overflow_size >= (self.size_addressed_per_pde):
                            extra_region_size = self.size_addressed_per_pde
                            extra_pte_valid_addr_end = self.total_pages

                        # load the new PDE's details

                        self.set_pde_pte_values(extra_pdpte,
                                                extra_pde,
                                                extra_pde_start_address,
                                                extra_region_size,
                                                0,
                                                extra_pte_valid_addr_end,
                                                flags)

                        # for the next iteration of the loop the size needs
                        # to decreased
                        overflow_size -= extra_region_size

                        if [extra_pdpte, extra_pde] not in self.pd_tables_list:
                            self.pd_tables_list.append([extra_pdpte, extra_pde])

                        if overflow_size == 0:
                            break

        self.pd_tables_list.sort()
        self.clean_up_unused_pdpte()


        pages_for_pdpte = 1
        pages_for_pd = self.get_number_of_pd()
        pages_for_pt = len(self.pd_tables_list)
        self.output_buffer = ctypes.create_string_buffer((pages_for_pdpte +
                                                   pages_for_pd +
                                                   pages_for_pt) * 4096)

    def pdpte_create_binary_file(self):
        # pae needs a pdpte at 32byte aligned address

        # Even though we have only 4 entries in the pdpte we need to move
        # the self.output_offset variable to the next page to start pushing
        # the pd contents
        #
        # FIXME: This wastes a ton of RAM!!
        if (args.verbose):
            print("PDPTE at 0x%x" % self.pd_start_addr)

        for pdpte in range(self.total_pages + 1):
            if pdpte in self.get_pdpte_list():
                present = 1 << 0
                addr_of_pd = (((self.pd_start_addr + 4096) +
                               self.get_pdpte_list().index(pdpte) *
                               4096) >> 12) << 12
                binary_value = (present | addr_of_pd)
            else:
                binary_value = 0

            struct.pack_into(page_entry_format,
                             self.output_buffer,
                             self.output_offset,
                             binary_value)

            self.output_offset += struct.calcsize(page_entry_format)


    def page_directory_create_binary_file(self):
        for pdpte, pde_info in self.list_of_pdpte.items():
            if (args.verbose):
                print("Page directory %d at 0x%x" % (pde_info.pdpte_index,
                        self.pd_start_addr + self.output_offset))
            for pde in range(self.total_pages + 1):
                binary_value = 0  # the page directory entry is not valid

                # if i have a valid entry to populate
                if pde in pde_info.pd_entries.keys():
                    value = pde_info.pd_entries[pde]
                    binary_value = self.get_binary_pde_value(pdpte, value)

                struct.pack_into(page_entry_format,
                                 self.output_buffer,
                                 self.output_offset,
                                 binary_value)
                if (args.verbose):
                    print_code(binary_value)

                self.output_offset += struct.calcsize(page_entry_format)

    def page_table_create_binary_file(self):
        for pdpte, pde_info in sorted(self.list_of_pdpte.items()):
            for pde, pte_info in sorted(pde_info.pd_entries.items()):
                pe_info = pte_info.page_entries_info[0]
                start_addr = pe_info.start_addr & ~0x1FFFFF
                end_addr = start_addr + 0x1FFFFF
                if (args.verbose):
                    print("Page table for 0x%08x - 0x%08x at 0x%08x" %
                            (start_addr, end_addr,
                                self.pd_start_addr + self.output_offset))
                for pte in range(self.total_pages + 1):
                    binary_value = 0  # the page directory entry is not valid

                    valid_pte = 0
                    # go through all the valid pages inside the pde to
                    # figure out if we need to populate this pte
                    for i in pte_info.page_entries_info:
                        temp_value = ((pte >= i.pte_valid_addr_start) and
                                      (pte <= i.pte_valid_addr_end))
                        if temp_value:
                            perm_for_pte = i.permissions
                        valid_pte |= temp_value

                    # if i have a valid entry to populate
                    if valid_pte:
                        binary_value = self.get_binary_pte_value(pde_info,
                                                                 pde,
                                                                 pte,
                                                                 perm_for_pte)

                    if (args.verbose):
                        print_code(binary_value)
                    struct.pack_into(page_entry_format,
                                     self.output_buffer,
                                     self.output_offset,
                                     binary_value)
                    self.output_offset += struct.calcsize(page_entry_format)



#*****************************************************************************#

def read_mmu_list(mmu_list_data):
    regions = []

    # Read mmu_list header data
    num_of_regions, pd_start_addr = struct.unpack_from(
        header_values_format, mmu_list_data, 0)

    # a offset used to remember next location to read in the binary
    size_read_from_binary = struct.calcsize(header_values_format)

    if (args.verbose):
        print("Start address of page tables: 0x%08x" % pd_start_addr)
        print("Build-time memory regions:")

    # Read all the region definitions
    for region_index in range(num_of_regions):
        addr, size, flags = struct.unpack_from(struct_mmu_regions_format,
                                                     mmu_list_data,
                                                     size_read_from_binary)
        size_read_from_binary += struct.calcsize(struct_mmu_regions_format)

        if (args.verbose):
            print("    Region %03d: 0x%08x - 0x%08x (0x%016x)" %
                    (region_index, addr, addr + size - 1, flags))

        # ignore zero sized memory regions
        if size == 0:
            continue

        if (addr & 0xFFF) != 0:
            print("Memory region %d start address %x is not page-aligned" %
                    (region_index, addr))
            sys.exit(2)

        if (size & 0xFFF) != 0:
            print("Memory region %d size %zu is not page-aligned" %
                    (region_index, size))
            sys.exit(2)

        # validate for memory overlap here
        for other_region_index in range(len(regions)):
            other_addr, other_size, _ = regions[other_region_index]

            end_addr = addr + size
            other_end_addr = other_addr + other_size

            overlap = ((addr <= other_addr and end_addr > other_addr) or
                       (other_addr <= addr and other_end_addr > addr))

            if (overlap):
                print("Memory region %d (%x:%x) overlaps memory region %d (%x:%x)" %
                        (region_index, addr, end_addr, other_region_index,
                            other_addr, other_end_addr))
                sys.exit(2)

        # add the retrived info another list
        regions.append((addr, size, flags))

    return (pd_start_addr, regions)


def check_bits(val, bits):
    for b in bits:
        if val & (1 << b):
            return 1
    return 0

def get_symbols(obj):
    for section in obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            return {sym.name: sym.entry.st_value
                    for sym in section.iter_symbols()}

    raise LookupError("Could not find symbol table")

# Read the parameters passed to the file
def parse_args():
    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-k", "--kernel",
                        help="Zephyr kernel image")
    parser.add_argument("-o", "--output",
                        help="Output file into which the page tables are "
                             "written.")
    parser.add_argument("-u", "--user-output",
                        help="User mode page tables for KPTI")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Print debugging information. Multiple "
                             "invocations increase verbosity")
    args = parser.parse_args()
    if "VERBOSE" in os.environ:
        args.verbose = 1

def main():
    parse_args()

    with open(args.kernel, "rb") as fp:
        kernel = ELFFile(fp)
        syms = get_symbols(kernel)
        irq_data = kernel.get_section_by_name("mmulist").data()

    pd_start_addr, regions = read_mmu_list(irq_data)

    # select the page table needed
    page_table = PageMode_PAE(pd_start_addr, regions, syms, False)

    # write the binary data into the file
    with open(args.output, 'wb') as fp:
        fp.write(page_table.output_buffer)

    if "CONFIG_X86_KPTI" in syms:
        pd_start_addr += page_table.output_offset

        user_page_table = PageMode_PAE(pd_start_addr, regions, syms, True)
        with open(args.user_output, 'wb') as fp:
            fp.write(user_page_table.output_buffer)

if __name__ == "__main__":
    main()
