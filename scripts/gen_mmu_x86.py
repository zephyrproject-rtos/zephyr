#!/usr/bin/env python3

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

#  global variables
pd_complete = ''
inputfile = ''
outputfile = ''
list_of_pde = {}
num_of_regions = 0
read_buff = ''
raw_info = []

mmu_region_details = namedtuple("mmu_region_details",
                                "pde_index page_entries_info")

valid_pages_inside_pde = namedtuple("valid_pages_inside_pde", "start_addr size \
                                pte_valid_addr_start \
                                pte_valid_addr_end \
                                permissions")

mmu_region_details_pdpt = namedtuple("mmu_region_details_pdpt",
                                     "pdpte_index pd_entries")

page_tables_list = []
pd_tables_list = []
pd_start_addr = 0
validation_issue_memory_overlap = [False, 0, -1]
output_offset = 0
print_string_pde_list = ''
pde_pte_string = {}
FourMB = (1024 * 4096)  # In Bytes

#  Constants
PAGE_ENTRY_PRESENT = 1
PAGE_ENTRY_READ_WRITE = 1 << 1
PAGE_ENTRY_USER_SUPERVISOR = 1 << 2
PAGE_ENTRY_PWT = 0 << 3
PAGE_ENTRY_PCD = 0 << 4
PAGE_ENTRY_ACCESSED = 0 << 5  # this is a read only field
PAGE_ENTRY_DIRTY = 0 << 6  # this is a read only field
PAGE_ENTRY_PAT = 0 << 7
PAGE_ENTRY_GLOBAL = 0 << 8
PAGE_ENTRY_ALLOC = 1 << 9
PAGE_ENTRY_CUSTOM = 0 << 10
#############

#*****************************************************************************#
# class for 4Kb Mode


class PageMode_4kb:
    total_pages = 1023
    write_page_entry_bin = "I"
    size_addressed_per_pde = (1024 * 4096)  # 4MB In Bytes

    # return the page directory number for the give address
    def get_pde_number(self, value):
        return (value >> 22) & 0x3FF

    # return the page table number for the given address
    def get_pte_number(self, value):
        return (value >> 12) & 0x3FF

    # get the total number of pd available
    def get_number_of_pd(self):
        return len(list_of_pde.keys())

    # the return value will have the page address and it is assumed
    # to be a 4096 boundary
    # hence the output of this API will be a 20bit address of the page table
    def address_of_page_table(self, page_table_number):
        global pd_start_addr

        # location from where the Page tables will be written
        PT_start_addr = pd_start_addr + 4096
        return ((PT_start_addr +
                 (page_tables_list.index(page_table_number) * 4096) >> 12))

    #   union x86_mmu_pde_pt {
    # 	u32_t  value;
    # 	struct {
    # 		u32_t p:1;
    # 		u32_t rw:1;
    # 		u32_t us:1;
    # 		u32_t pwt:1;
    # 		u32_t pcd:1;
    # 		u32_t a:1;
    # 		u32_t ignored1:1;
    # 		u32_t ps:1;
    # 		u32_t ignored2:4;
    # 		u32_t page_table:20;
    # 	};
    # };
    def get_binary_pde_value(self, value):
        perms = value.page_entries_info[0].permissions

        present = PAGE_ENTRY_PRESENT
        read_write = check_bits(perms, [1, 29]) << 1
        user_mode = check_bits(perms, [2, 28]) << 2

        pwt = PAGE_ENTRY_PWT
        pcd = PAGE_ENTRY_PCD
        a = PAGE_ENTRY_ACCESSED
        ps = 0 << 7  # this is a read only field
        page_table = self.address_of_page_table(value.pde_index) << 12
        return (present |
                read_write |
                user_mode |
                pwt |
                pcd |
                a |
                ps |
                page_table)

    # union x86_mmu_pte {
    # 	u32_t  value;
    # 	struct {
    # 		u32_t p:1;
    # 		u32_t rw:1;
    # 		u32_t us:1;
    # 		u32_t pwt:1;
    # 		u32_t pcd:1;
    # 		u32_t a:1;
    # 		u32_t d:1;
    # 		u32_t pat:1;
    # 		u32_t g:1;
    # 		u32_t alloc:1;
    # 		u32_t custom:2;
    # 		u32_t page:20;
    # 	};
    # };
    def get_binary_pte_value(self, value, pte, perm_for_pte):
        present = PAGE_ENTRY_PRESENT
        read_write = ((perm_for_pte >> 1) & 0x1) << 1
        user_mode = ((perm_for_pte >> 2) & 0x1) << 2
        pwt = PAGE_ENTRY_PWT
        pcd = PAGE_ENTRY_PCD
        a = PAGE_ENTRY_ACCESSED
        d = PAGE_ENTRY_DIRTY
        pat = PAGE_ENTRY_PAT
        g = PAGE_ENTRY_GLOBAL
        alloc = PAGE_ENTRY_ALLOC
        custom = PAGE_ENTRY_CUSTOM

        # This points to the actual memory in the HW
        # totally 20 bits to rep the phy address
        # first 10 is the number got from pde and next 10 is pte
        page_table = ((value.pde_index << 10) | pte) << 12

        binary_value = (present | read_write | user_mode |
                        pwt | pcd | a | d | pat | g | alloc | custom |
                        page_table)
        return binary_value

    def populate_required_structs(self):
        for region in raw_info:
            pde_index = self.get_pde_number(region[0])
            pte_valid_addr_start = self.get_pte_number(region[0])

            # Get the end of the page table entries
            # Since a memory region can take up only a few entries in the Page
            # table, this helps us get the last valid PTE.
            pte_valid_addr_end = self.get_pte_number(region[0] +
                                                     region[1] - 1)

            mem_size = region[1]

            # In-case the start address aligns with a page table entry other
            # than zero and the mem_size is greater than (1024*4096) i.e 4MB
            # in case where it overflows the currenty PDE's range then limit the
            # PTE to 1024 and so make the mem_size reflect the actual size taken
            # up in the current PDE
            if (region[1] + (pte_valid_addr_start * 4096)) >= \
               (self.size_addressed_per_pde):

                pte_valid_addr_end = self.total_pages
                mem_size = (((self.total_pages + 1) -
                             pte_valid_addr_start) * 4096)

            self.set_pde_pte_values(pde_index, region[0], mem_size,
                                    pte_valid_addr_start,
                                    pte_valid_addr_end,
                                    region[2])

            if pde_index not in page_tables_list:
                page_tables_list.append(pde_index)

            # IF the current pde couldn't fit the entire requested region size
            # then there is a need to create new PDEs to match the size.
            # Here the overflow_size represents the size that couldn't be fit
            # inside the current PDE, this is will now to used to create a
            # new PDE/PDEs so the size remaining will be
            # requested size - allocated size(in the current PDE)

            overflow_size = region[1] - mem_size

            # create all the extra PDEs needed to fit the requested size
            # this loop starts from the current pde till the last pde that is
            # needed the last pde is calcualted as the (start_addr + size) >>
            # 22
            if overflow_size != 0:
                for extra_pde in range(pde_index + 1, self.get_pde_number(
                        region[0] + region[1]) + 1):

                    # new pde's start address
                    # each page directory entry has a addr range of (1024 *4096)
                    # thus the new PDE start address is a multiple of that
                    # number
                    extra_pde_start_address = (extra_pde *
                    (self.size_addressed_per_pde))

                    # the start address of and extra pde will always be 0
                    # and the end address is calculated with the new pde's start
                    # address and the overflow_size
                    extra_pte_valid_addr_end = self.get_pte_number(
                        extra_pde_start_address + overflow_size - 1)

                    # if the overflow_size couldn't be fit inside this new pde
                    # then need another pde and so we now need to limit the end
                    # of the PTE to 1024 and set the size of this new region to
                    # the max possible
                    extra_region_size = overflow_size
                    if overflow_size >= (self.size_addressed_per_pde):
                        extra_region_size = self.size_addressed_per_pde
                        extra_pte_valid_addr_end = self.total_pages

                    # load the new PDE's details

                    self.set_pde_pte_values(extra_pde,
                                            extra_pde_start_address,
                                            extra_region_size,
                                            0,
                                            extra_pte_valid_addr_end,
                                            region[2])

                    # for the next iteration of the loop the size needs to
                    # decreased.
                    overflow_size -= extra_region_size

                    # print(hex_32(overflow_size),extra_pde)
                    if extra_pde not in page_tables_list:
                        page_tables_list.append(extra_pde)

                    if overflow_size == 0:
                        break

        page_tables_list.sort()

    # update the tuple values for the memory regions needed
    def set_pde_pte_values(self, pde_index, address, mem_size,
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

        if pde_index in list_of_pde.keys():
            # this step adds the new page info to the exsisting pages info
            list_of_pde[pde_index].page_entries_info.append(pages_tuple)
        else:
            list_of_pde[pde_index] = mem_region_values

    def page_directory_create_binary_file(self):
        global output_buffer
        global output_offset
        for pde in range(self.total_pages + 1):
            binary_value = 0  # the page directory entry is not valid

            # if i have a valid entry to populate
            if pde in sorted(list_of_pde.keys()):
                value = list_of_pde[pde]
                binary_value = self.get_binary_pde_value(value)
                self.pde_verbose_output(pde, binary_value)

            struct.pack_into(self.write_page_entry_bin,
                             output_buffer,
                             output_offset,
                             binary_value)

            output_offset += struct.calcsize(self.write_page_entry_bin)

    def page_table_create_binary_file(self):
        global output_buffer
        global output_offset

        for key, value in sorted(list_of_pde.items()):
            for pte in range(self.total_pages + 1):
                binary_value = 0  # the page directory entry is not valid

                valid_pte = 0
                for i in value.page_entries_info:
                    temp_value = ((pte >= i.pte_valid_addr_start) and
                                  (pte <= i.pte_valid_addr_end))
                    if temp_value:
                        perm_for_pte = i.permissions
                    valid_pte |= temp_value

                # if i have a valid entry to populate
                if valid_pte:
                    binary_value = self.get_binary_pte_value(value,
                                                             pte,
                                                             perm_for_pte)
                    self.pte_verbose_output(key, pte, binary_value)

                struct.pack_into(self.write_page_entry_bin,
                                 output_buffer,
                                 output_offset,
                                 binary_value)
                output_offset += struct.calcsize(self.write_page_entry_bin)

    # To populate the binary file the module struct needs a buffer of the
    # excat size. This returns the size needed for the given set of page
    # tables.
    def set_binary_file_size(self):
        binary_size = ctypes.create_string_buffer((4096) +
                                                  (len(list_of_pde.keys()) *
                                                   4096))
        return binary_size

    # prints the details of the pde
    def verbose_output(self):

        print("\nTotal Page directory entries " + str(self.get_number_of_pd()))
        count = 0
        for key, value in list_of_pde.items():
            for i in value.page_entries_info:
                count += 1
                print("In Page directory entry " +
                      format_string(value.pde_index) +
                      ": valid start address = " +
                      hex_32(i.start_addr) + ", end address = " +
                      hex_32((i.pte_valid_addr_end + 1) * 4096 - 1 +
                             (value.pde_index * (FourMB))))

    # print all the tables for a given page table mode
    def print_all_page_table_info(self):
        self.pde_print_elements()
        self.pte_print_elements()

    def pde_verbose_output(self, pde, binary_value):
        if args.verbose < 2:
            return

        global print_string_pde_list

        present = format_string(binary_value & 0x1)
        read_write = format_string((binary_value >> 1) & 0x1)
        user_mode = format_string((binary_value >> 2) & 0x1)
        pwt = format_string((binary_value >> 3) & 0x1)
        pcd = format_string((binary_value >> 4) & 0x1)
        a = format_string((binary_value >> 5) & 0x1)
        ignored1 = format_string(0)
        ps = format_string((binary_value >> 7) & 0x1)
        ignored2 = format_string(0000)
        page_table_addr = format_string(hex((binary_value >> 12) & 0xFFFFF))

        print_string_pde_list += (format_string(str(pde)) +
                                  " | " +
                                  (present) +
                                  " | " +
                                  (read_write) + " | " +
                                  (user_mode) + " | " +
                                  (pwt) + " | " +
                                  (pcd) + " | " +
                                  (a) + " | " +
                                  (ps) + " | " +
                                  page_table_addr + "\n"
                                  )

    def pde_print_elements(self):
        global print_string_pde_list
        print("PAGE DIRECTORY ")
        print(format_string("PDE") + " | " +
              format_string('P') + " | " +
              format_string('rw') + " | " +
              format_string('us') + " | " +
              format_string('pwt') + " | " +
              format_string('pcd') + " | " +
              format_string('a') + " | " +
              format_string('ps') + " | " +
              format_string('Addr page table'))
        print(print_string_pde_list)
        print("END OF PAGE DIRECTORY")

    def pte_verbose_output(self, pde, pte, binary_value):
        global pde_pte_string

        present = format_string((binary_value >> 0) & 0x1)
        read_write = format_string((binary_value >> 1) & 0x1)
        user_mode = format_string((binary_value >> 2) & 0x1)
        pwt = format_string((binary_value >> 3) & 0x1)
        pcd = format_string((binary_value >> 4) & 0x1)
        a = format_string((binary_value >> 5) & 0x1)
        d = format_string((binary_value >> 6) & 0x1)
        pat = format_string((binary_value >> 7) & 0x1)
        g = format_string((binary_value >> 8) & 0x1)
        alloc = format_string((binary_value >> 9) & 0x1)
        custom = format_string((binary_value >> 10) & 0x3)
        page_table_addr = hex_20((binary_value >> 12) & 0xFFFFF)

        print_string_list = (format_string(str(pte)) + " | " +
                             (present) + " | " +
                             (read_write) + " | " +
                             (user_mode) + " | " +
                             (pwt) + " | " +
                             (pcd) + " | " +
                             (a) + " | " +
                             (d) + " | " +
                             (pat) + " | " +
                             (g) + " | " +
                             (alloc) + " | " +
                             (custom) + " | " +
                             page_table_addr + "\n"
                             )

        if pde in pde_pte_string.keys():
            pde_pte_string[pde] += (print_string_list)
        else:
            pde_pte_string[pde] = print_string_list

    def pte_print_elements(self):
        global pde_pte_string

        for pde, print_string in sorted(pde_pte_string.items()):
            print("\nPAGE TABLE " + str(pde))

            print(format_string("PTE") + " | " +
                  format_string('P') + " | " +
                  format_string('rw') + " | " +
                  format_string('us') + " | " +
                  format_string('pwt') + " | " +
                  format_string('pcd') + " | " +
                  format_string('a') + " | " +
                  format_string('d') + " | " +
                  format_string('pat') + " | " +
                  format_string('g') + " | " +
                  format_string('alloc') + " | " +
                  format_string('custom') + " | " +
                  format_string('page addr'))
            print(print_string)
            print("END OF PAGE TABLE " + str(pde))


#*****************************************************************************#
# class for PAE 4KB Mode
class PageMode_PAE:
    total_pages = 511
    write_page_entry_bin = "Q"
    size_addressed_per_pde = (512 * 4096)  # 2MB In Bytes
    size_addressed_per_pdpte = (512 * size_addressed_per_pde)  # In Bytes
    list_of_pdpte = {}
    pdpte_print_string = {}
    print_string_pdpte_list = ''

    # TODO enable all page tables on just a flag

    def __init__(self):
        for i in range(4):
            self.list_of_pdpte[i] = mmu_region_details_pdpt(pdpte_index=i,
                                                            pd_entries={})

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
        return list({temp[0] for temp in pd_tables_list})

    # the return value will have the page address and it is assumed to be a 4096
    # boundary.hence the output of this API will be a 20bit address of the page
    # table
    def address_of_page_table(self, pdpte, page_table_number):
        global pd_start_addr

        # first page given to page directory pointer
        # and 2nd page till 5th page are used for storing the page directories.

        # set the max pdpte used. this tells how many pd are needed after
        # that we start keeping the pt
        PT_start_addr = self.get_number_of_pd() * 4096 +\
            pd_start_addr + 4096
        return (PT_start_addr +
                (pd_tables_list.index([pdpte, page_table_number]) *
                 4096) >> 12)

    #   union x86_mmu_pae_pde {
    # 	u64_t  value;
    # 	struct {
    # 		u64_t p:1;
    # 		u64_t rw:1;
    # 		u64_t us:1;
    # 		u64_t pwt:1;
    # 		u64_t pcd:1;
    # 		u64_t a:1;
    # 		u64_t ignored1:1;
    # 		u64_t ps:1;
    # 		u64_t ignored2:4;
    # 		u64_t page_table:20;
    # 		u64_t igonred3:29;
    # 		u64_t xd:1;
    # 	};
    # };

    def get_binary_pde_value(self, pdpte, value):
        perms = value.page_entries_info[0].permissions

        present = PAGE_ENTRY_PRESENT
        read_write = check_bits(perms, [1, 29]) << 1
        user_mode = check_bits(perms, [2, 28]) << 2

        pwt = PAGE_ENTRY_PWT
        pcd = PAGE_ENTRY_PCD
        a = PAGE_ENTRY_ACCESSED
        ps = 0 << 7  # set to make sure that the phy page is 4KB
        page_table = self.address_of_page_table(pdpte, value.pde_index) << 12
        xd = 0
        return (present |
                read_write |
                user_mode |
                pwt |
                pcd |
                a |
                ps |
                page_table |
                xd)

    # union x86_mmu_pae_pte {
    # 	u64_t  value;
    # 	struct {
    # 		u64_t p:1;
    # 		u64_t rw:1;
    # 		u64_t us:1;
    # 		u64_t pwt:1;
    # 		u64_t pcd:1;
    # 		u64_t a:1;
    # 		u64_t d:1;
    # 		u64_t pat:1;
    # 		u64_t g:1;
    # 		u64_t ignore:3;
    # 		u64_t page:20;
    # 		u64_t igonred3:29;
    # 		u64_t xd:1;
    # 	};
    # };
    def get_binary_pte_value(self, value, pde, pte, perm_for_pte):
        present = PAGE_ENTRY_PRESENT
        read_write = perm_for_pte & PAGE_ENTRY_READ_WRITE
        user_mode = perm_for_pte & PAGE_ENTRY_USER_SUPERVISOR
        pwt = PAGE_ENTRY_PWT
        pcd = PAGE_ENTRY_PCD
        a   = PAGE_ENTRY_ALLOC
        d   = PAGE_ENTRY_DIRTY
        pat = PAGE_ENTRY_PAT
        g   = PAGE_ENTRY_GLOBAL

        # This points to the actual memory in the HW
        # totally 20 bits to rep the phy address
        # first 2bits is from pdpte then 9bits is the number got from pde and
        # next 9bits is pte
        page_table = ((value.pdpte_index << 18) | (pde << 9) | pte) << 12

        xd = ((perm_for_pte >> 63) & 0x1) << 63

        binary_value = (present | read_write | user_mode |
                        pwt | pcd | a | d | pat | g |
                        page_table | xd)
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
        for region in raw_info:
            pdpte_index = self.get_pdpte_number(region[0])
            pde_index = self.get_pde_number(region[0])
            pte_valid_addr_start = self.get_pte_number(region[0])

            # Get the end of the page table entries
            # Since a memory region can take up only a few entries in the Page
            # table, this helps us get the last valid PTE.
            pte_valid_addr_end = self.get_pte_number(region[0] +
                                                     region[1] - 1)

            mem_size = region[1]

            # In-case the start address aligns with a page table entry other
            # than zero and the mem_size is greater than (1024*4096) i.e 4MB
            # in case where it overflows the currenty PDE's range then limit the
            # PTE to 1024 and so make the mem_size reflect the actual size
            # taken up in the current PDE
            if (region[1] + (pte_valid_addr_start * 4096)) >= \
               (self.size_addressed_per_pde):

                pte_valid_addr_end = self.total_pages
                mem_size = (((self.total_pages + 1) -
                             pte_valid_addr_start) * 4096)

            self.set_pde_pte_values(pdpte_index,
                                    pde_index,
                                    region[0],
                                    mem_size,
                                    pte_valid_addr_start,
                                    pte_valid_addr_end,
                                    region[2])

            if [pdpte_index, pde_index] not in pd_tables_list:
                pd_tables_list.append([pdpte_index, pde_index])

            # IF the current pde couldn't fit the entire requested region
            # size then there is a need to create new PDEs to match the size.
            # Here the overflow_size represents the size that couldn't be fit
            # inside the current PDE, this is will now to used to create a new
            # PDE/PDEs so the size remaining will be
            # requested size - allocated size(in the current PDE)

            overflow_size = region[1] - mem_size

            # create all the extra PDEs needed to fit the requested size
            # this loop starts from the current pde till the last pde that is
            # needed the last pde is calcualted as the (start_addr + size) >>
            # 22
            if overflow_size != 0:
                for extra_pdpte in range(pdpte_index,
                                         self.get_pdpte_number(region[0] +
                                                               region[1]) + 1):
                    for extra_pde in range(pde_index + 1, self.get_pde_number(
                            region[0] + region[1]) + 1):

                        # new pde's start address
                        # each page directory entry has a addr range of
                        # (1024 *4096) thus the new PDE start address is a
                        # multiple of that number
                        extra_pde_start_address = (extra_pde *
                        (self.size_addressed_per_pde))

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
                                                region[2])

                        # for the next iteration of the loop the size needs
                        # to decreased
                        overflow_size -= extra_region_size

                        if [extra_pdpte, extra_pde] not in pd_tables_list:
                            pd_tables_list.append([extra_pdpte, extra_pde])

                        if overflow_size == 0:
                            break

        pd_tables_list.sort()
        self.clean_up_unused_pdpte()

    def pdpte_create_binary_file(self):
        global output_buffer
        global output_offset
        global pd_start_addr

        # pae needs a pdpte at 32byte aligned address

        # Even though we have only 4 entries in the pdpte we need to move
        # the output_offset variable to the next page to start pushing
        # the pd contents
        for pdpte in range(self.total_pages + 1):
            if pdpte in self.get_pdpte_list():
                present = 1 << 0
                pwt = 0 << 3
                pcd = 0 << 4
                addr_of_pd = (((pd_start_addr + 4096) +
                               self.get_pdpte_list().index(pdpte) *
                               4096) >> 12) << 12
                binary_value = (present | pwt | pcd | addr_of_pd)
                self.pdpte_verbose_output(pdpte, binary_value)
            else:
                binary_value = 0

            struct.pack_into(self.write_page_entry_bin,
                             output_buffer,
                             output_offset,
                             binary_value)

            output_offset += struct.calcsize(self.write_page_entry_bin)

    def page_directory_create_binary_file(self):
        global output_buffer
        global output_offset
        pdpte_number_count = 0
        for pdpte, pde_info in self.list_of_pdpte.items():

            pde_number_count = 0
            for pde in range(self.total_pages + 1):
                binary_value = 0  # the page directory entry is not valid

                # if i have a valid entry to populate
                # if pde in sorted(list_of_pde.keys()):
                if pde in sorted(pde_info.pd_entries.keys()):
                    value = pde_info.pd_entries[pde]
                    binary_value = self.get_binary_pde_value(pdpte, value)
                    self.pde_verbose_output(pdpte, pde, binary_value)

                pde_number_count += 1
                struct.pack_into(self.write_page_entry_bin,
                                 output_buffer,
                                 output_offset,
                                 binary_value)

                output_offset += struct.calcsize(self.write_page_entry_bin)

    def page_table_create_binary_file(self):
        global output_buffer
        global output_offset

        pdpte_number_count = 0
        for pdpte, pde_info in sorted(self.list_of_pdpte.items()):
            pdpte_number_count += 1
            for pde, pte_info in sorted(pde_info.pd_entries.items()):
                pte_number_count = 0
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
                        pte_number_count += 1
                        self.pte_verbose_output(pdpte, pde, pte, binary_value)

                    # print(binary_value, (self.write_page_entry_bin))

                    struct.pack_into(self.write_page_entry_bin,
                                     output_buffer,
                                     output_offset,
                                     binary_value)
                    output_offset += struct.calcsize(self.write_page_entry_bin)


    # To populate the binary file the module struct needs a buffer of the
    # excat size This returns the size needed for the given set of page tables.
    def set_binary_file_size(self):
        pages_for_pdpte = 1
        pages_for_pd = self.get_number_of_pd()
        pages_for_pt = len(pd_tables_list)
        binary_size = ctypes.create_string_buffer((pages_for_pdpte +
                                                   pages_for_pd +
                                                   pages_for_pt) * 4096)
        return binary_size

    # prints the details of the pde
    def verbose_output(self):
        print("\nTotal Page directory Page pointer entries " +
              str(self.get_number_of_pd()))
        count = 0
        for pdpte, pde_info in sorted(self.list_of_pdpte.items()):
            print(
                "In page directory page table pointer " +
                format_string(pdpte))

            for pde, pte_info in sorted(pde_info.pd_entries.items()):
                for pte in pte_info.page_entries_info:
                    count += 1
                    print("    In Page directory entry " + format_string(pde) +
                          ": valid start address = " +
                          hex_32(pte.start_addr) + ", end address = " +
                          hex_32((pte.pte_valid_addr_end + 1) * 4096 - 1 +
                                 (pde * (self.size_addressed_per_pde)) +
                                 (pdpte * self.size_addressed_per_pdpte)))

    def pdpte_verbose_output(self, pdpte, binary_value):
        if args.verbose < 2:
            return

        present = format_string(binary_value & 0x1)
        pwt = format_string((binary_value >> 3) & 0x1)
        pcd = format_string((binary_value >> 4) & 0x1)
        page_table_addr = format_string(hex((binary_value >> 12) & 0xFFFFF))

        self.print_string_pdpte_list += (format_string(str(pdpte)) +
                                         " | " + (present) + " | " +
                                         (pwt) + " | " +
                                         (pcd) + " | " +
                                         page_table_addr + "\n")

    def pdpte_print_elements(self):
        print("\nPAGE DIRECTORIES POINTER ")
        print(format_string("PDPTE") + " | " +
              format_string('P') + " | " +
              format_string('pwt') + " | " +
              format_string('pcd') + " | " +
              format_string('Addr'))
        print(self.print_string_pdpte_list)
        print("END OF PAGE DIRECTORY POINTER")

    def pde_verbose_output(self, pdpte, pde, binary_value):
        if args.verbose < 2:
            return

        global print_string_pde_list

        present = format_string(binary_value & 0x1)
        read_write = format_string((binary_value >> 1) & 0x1)
        user_mode = format_string((binary_value >> 2) & 0x1)
        pwt = format_string((binary_value >> 3) & 0x1)
        pcd = format_string((binary_value >> 4) & 0x1)
        a = format_string((binary_value >> 5) & 0x1)
        ignored1 = format_string(0)
        ps = format_string((binary_value >> 7) & 0x1)
        ignored2 = format_string(0000)
        page_table_addr = format_string(hex((binary_value >> 12) & 0xFFFFF))
        xd = format_string((binary_value >> 63) & 0x1)

        print_string_pde_list = (format_string(str(pde)) + " | " +
                                 (present) + " | " +
                                 (read_write) + " | " +
                                 (user_mode) + " | " +
                                 (pwt) + " | " +
                                 (pcd) + " | " +
                                 (a) + " | " +
                                 (ps) + " | " +
                                 page_table_addr + " | " +
                                 (xd) + "\n")

        if pdpte in self.pdpte_print_string.keys():
            self.pdpte_print_string[pdpte] += (print_string_pde_list)
        else:
            self.pdpte_print_string[pdpte] = print_string_pde_list

    # print all the tables for a given page table mode
    def print_all_page_table_info(self):
        self.pdpte_print_elements()
        self.pde_print_elements()
        self.pte_print_elements()

    def pde_print_elements(self):
        global print_string_pde_list

        for pdpte, print_string in sorted(self.pdpte_print_string.items()):
            print("\n PAGE DIRECTORIES for PDPT " + str(pdpte))
            print(format_string("PDE") + " | " +
                  format_string('P') + " | " +
                  format_string('rw') + " | " +
                  format_string('us') + " | " +
                  format_string('pwt') + " | " +
                  format_string('pcd') + " | " +
                  format_string('a') + " | " +
                  format_string('ps') + " | " +
                  format_string('Addr') + " | " +
                  format_string('xd'))
            print(print_string)
            print("END OF PAGE DIRECTORIES for PDPT " + str(pdpte))

    def pte_verbose_output(self, pdpte, pde, pte, binary_value):
        global pde_pte_string

        present = format_string((binary_value >> 0) & 0x1)
        read_write = format_string((binary_value >> 1) & 0x1)
        user_mode = format_string((binary_value >> 2) & 0x1)
        pwt = format_string((binary_value >> 3) & 0x1)
        pcd = format_string((binary_value >> 4) & 0x1)
        a = format_string((binary_value >> 5) & 0x1)
        d = format_string((binary_value >> 6) & 0x1)
        pat = format_string((binary_value >> 7) & 0x1)
        g = format_string((binary_value >> 8) & 0x1)
        page_table_addr = hex_20((binary_value >> 12) & 0xFFFFF)
        xd = format_string((binary_value >> 63) & 0x1)

        print_string_list = (format_string(str(pte)) + " | " +
                             (present) + " | " +
                             (read_write) + " | " +
                             (user_mode) + " | " +
                             (pwt) + " | " +
                             (pcd) + " | " +
                             (a) + " | " +
                             (d) + " | " +
                             (pat) + " | " +
                             (g) + " | " +
                             page_table_addr + " | " +
                             (xd) + "\n"
                             )

        if (pdpte, pde) in pde_pte_string.keys():
            pde_pte_string[(pdpte, pde)] += (print_string_list)
        else:
            pde_pte_string[(pdpte, pde)] = print_string_list

    def pte_print_elements(self):
        global pde_pte_string

        for (pdpte, pde), print_string in sorted(pde_pte_string.items()):
            print(
                "\nPAGE TABLE for PDPTE = " +
                str(pdpte) +
                " and PDE = " +
                str(pde))

            print(format_string("PTE") + " | " +
                  format_string('P') + " | " +
                  format_string('rw') + " | " +
                  format_string('us') + " | " +
                  format_string('pwt') + " | " +
                  format_string('pcd') + " | " +
                  format_string('a') + " | " +
                  format_string('d') + " | " +
                  format_string('pat') + " | " +
                  format_string('g') + " | " +
                  format_string('Page Addr') + " | " +
                  format_string('xd'))
            print(print_string)
            print("END OF PAGE TABLE " + str(pde))


#*****************************************************************************#


def print_list_of_pde(list_of_pde):
    for key, value in list_of_pde.items():
        print(key, value)
        print('\n')


# read the binary from the input file and populate a dict for
# start address of mem region
# size of the region - so page tables entries will be created with this
# read write permissions

def read_mmu_list_marshal_param(page_mode):

    global read_buff
    global page_tables_list
    global pd_start_addr
    global validation_issue_memory_overlap
    read_buff = input_file.read()
    input_file.close()

    # read contents of the binary file first 2 values read are
    # num_of_regions and page directory start address both calculated and
    # populated by the linker
    num_of_regions, pd_start_addr = struct.unpack_from(
        header_values_format, read_buff, 0)

    # a offset used to remember next location to read in the binary
    size_read_from_binary = struct.calcsize(header_values_format)

    # for each of the regions mentioned in the binary loop and populate all the
    # required parameters
    for region in range(num_of_regions):
        basic_mem_region_values = struct.unpack_from(struct_mmu_regions_format,
                                                     read_buff,
                                                     size_read_from_binary)
        size_read_from_binary += struct.calcsize(struct_mmu_regions_format)

        # ignore zero sized memory regions
        if basic_mem_region_values[1] == 0:
            continue

        # validate for memory overlap here
        for i in raw_info:
            start_location = basic_mem_region_values[0]
            end_location = basic_mem_region_values[0] + \
                basic_mem_region_values[1]

            overlap_occurred = ((start_location >= i[0]) and
                                (start_location <= (i[0] + i[1]))) and \
                ((end_location >= i[0]) and
                 (end_location <= i[0] + i[1]))

            if overlap_occurred:
                validation_issue_memory_overlap = [
                    True,
                    start_location,
                    page_mode.get_pde_number(start_location)]
                return

        # add the retrived info another list
        raw_info.append(basic_mem_region_values)


def validate_pde_regions():
    # validation for correct page alignment of the regions
    for key, value in list_of_pde.items():
        for pages_inside_pde in value.page_entries_info:
            if pages_inside_pde.start_addr & (0xFFF) != 0:
                print("Memory Regions are not page aligned",
                      hex(pages_inside_pde.start_addr))
                sys.exit(2)

            # validation for correct page alignment of the regions
            if pages_inside_pde.size & (0xFFF) != 0:
                print("Memory Regions size is not page aligned",
                      hex(pages_inside_pde.size))
                sys.exit(2)

    # validation for spiling of the regions across various
    if validation_issue_memory_overlap[0] == True:
        print("Memory Regions are overlapping at memory address " +
              str(hex(validation_issue_memory_overlap[1])) +
              " with Page directory Entry number " +
              str(validation_issue_memory_overlap[2]))
        sys.exit(2)


def check_bits(val, bits):
    for b in bits:
        if val & (1 << b):
            return 1
    return 0


# Read the parameters passed to the file
def parse_args():
    global args

    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-e", "--big-endian", action="store_true",
                        help="Target encodes data in big-endian format"
                        "(little endian is the default)")

    parser.add_argument("-i", "--input",
                        help="Input file from which MMU regions are read.")
    parser.add_argument("-k", "--kernel",
                        help="Zephyr kernel image")
    parser.add_argument("-o", "--output",
                        help="Output file into which the page tables are written.")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Print debugging information. Multiple "
                             "invocations increase verbosity")
    args = parser.parse_args()


# the format for writing in the binary file would be decided by the
# endian selected
def set_struct_endian_format(page_mode):
    endian_string = "<"
    if args.big_endian is True:
        endian_string = ">"
    global struct_mmu_regions_format
    global header_values_format

    struct_mmu_regions_format = endian_string + "IIQ"
    header_values_format = endian_string + "II"
    page_mode.write_page_entry_bin = (endian_string +
                                      page_mode.write_page_entry_bin)


def format_string(input_str):
    output_str = '{0: <5}'.format(str(input_str))
    return output_str

# format for 32bit hex value
def hex_32(input_value):
    output_value = "{0:#0{1}x}".format(input_value, 10)
    return output_value

# format for 20bit hex value
def hex_20(input_value):
    output_value = "{0:#0{1}x}".format(input_value, 7)
    return output_value


def verbose_output(page_mode):
    if args.verbose == 0:
        return

    print("\nMemory Regions as defined:")
    for info in raw_info:
        print("Memory region start address = " + hex_32(info[0]) +
              ", Memory size = " + hex_32(info[1]) +
              ", Permission = " + hex(info[2]))

    page_mode.verbose_output()

    if args.verbose > 1:
        page_mode.print_all_page_table_info()

# build sym table
def get_symbols(obj):
    for section in obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            return {sym.name: sym.entry.st_value
                    for sym in section.iter_symbols()}

    raise LookupError("Could not find symbol table")


# determine which paging mode was selected
def get_page_mode():
    with open(args.kernel, "rb") as fp:
        kernel = ELFFile(fp)
        sym = get_symbols(kernel)
    try:
        return sym["CONFIG_X86_PAE_MODE"]
    except BaseException:
        return 0


def main():
    global output_buffer
    parse_args()

    # select the page table needed
    if get_page_mode():
        page_mode = PageMode_PAE()
    else:
        page_mode = PageMode_4kb()

    set_struct_endian_format(page_mode)

    global input_file
    input_file = open(args.input, 'rb')

    global binary_output_file
    binary_output_file = open(args.output, 'wb')

    # inputfile= file_name
    read_mmu_list_marshal_param(page_mode)

    # populate the required structs
    page_mode.populate_required_structs()

    # validate the inputs
    validate_pde_regions()

    # The size of the output buffer has to match the number of bytes we write
    # this corresponds to the number of page tables gets created.
    output_buffer = page_mode.set_binary_file_size()

    try:
        page_mode.pdpte_create_binary_file()
    except BaseException:
        pass
    page_mode.page_directory_create_binary_file()
    page_mode.page_table_create_binary_file()

    # write the binary data into the file
    binary_output_file.write(output_buffer)
    binary_output_file.close()

    # verbose output needed by the build system
    verbose_output(page_mode)


if __name__ == "__main__":
    main()
