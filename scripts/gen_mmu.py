#!/usr/bin/env python3

import os
import sys
import struct
import parser
from collections import namedtuple
import ctypes
import argparse

############# global variables
pd_complete = ''
inputfile = ''
outputfile = ''
list_of_pde = {}
num_of_regions = 0
read_buff=''

struct_mmu_regions_tuple = {"start_addr","size","permissions"}
mmu_region_details = namedtuple("mmu_region_details", "pde_index page_entries_info")

valid_pages_inside_pde = namedtuple("valid_pages_inside_pde","start_addr size \
                                pte_valid_addr_start \
                                pte_valid_addr_end \
                                permissions")

page_tables_list = []
pd_start_addr = 0
validation_issue_memory_overlap = [False, 0, -1]
output_offset = 0
print_string_pde_list = ''
pde_pte_string = {}

#############

#return the page directory number for the give address
def get_pde_number(value):
    return( (value >> 22 ) & 0x3FF)

#return the page table number for the given address
def get_pte_number(value):
    return( (value >> 12 ) & 0x3FF)


# update the tuple values for the memory regions needed
def set_pde_pte_values(pde_index, address, mem_size,
                       pte_valid_addr_start, pte_valid_addr_end, perm):

    pages_tuple = valid_pages_inside_pde(
        start_addr = address,
        size = mem_size,
        pte_valid_addr_start = pte_valid_addr_start,
        pte_valid_addr_end = pte_valid_addr_end,
        permissions = perm)

    mem_region_values = mmu_region_details(pde_index = pde_index,
                                           page_entries_info = [])

    mem_region_values.page_entries_info.append(pages_tuple)

    if pde_index in list_of_pde.keys():
        # this step adds the new page info to the exsisting pages info
        list_of_pde[pde_index].page_entries_info.append(pages_tuple)
    else:
        list_of_pde[pde_index] = mem_region_values



# read the binary from the input file and populate a dict for
# start address of mem region
# size of the region - so page tables entries will be created with this
# read write permissions
def read_mmu_list_marshal_param():

    global read_buff
    global page_tables_list
    global pd_start_addr
    global validation_issue_memory_overlap
    read_buff = input_file.read()
    input_file.close()
    raw_info=[]

    # read contents of the binary file first 2 values read are
    # num_of_regions and page directory start address both calculated and
    # populated by the linker
    num_of_regions, pd_start_addr = struct.unpack_from(header_values_format,read_buff,0);

    # a offset used to remember next location to read in the binary
    size_read_from_binary = struct.calcsize(header_values_format);

    # for each of the regions mentioned in the binary loop and populate all the
    # required parameters
    for region in range(num_of_regions):
        basic_mem_region_values = struct.unpack_from(struct_mmu_regions_format,
                                               read_buff,
                                               size_read_from_binary);
        size_read_from_binary += struct.calcsize(struct_mmu_regions_format);

        #validate for memory overlap here
        for i in raw_info:
            start_location = basic_mem_region_values[0]
            end_location = basic_mem_region_values[0] + basic_mem_region_values[1]

            overlap_occurred = ( (start_location >= i[0]) and \
             (start_location <= (i[0]+i[1]))) and \
            ((end_location >= i[0]) and \
              (end_location <= i[0]+i[1]))

            if overlap_occurred:
                validation_issue_memory_overlap = [True,
                 start_location,
                 get_pde_number(start_location)]
                return

        # add the retrived info another list
        raw_info.append(basic_mem_region_values)

    for region in raw_info:
        pde_index = get_pde_number(region[0])
        pte_valid_addr_start = get_pte_number(region[0])

        # Get the end of the page table entries
        # Since a memory region can take up only a few entries in the Page
        # table, this helps us get the last valid PTE.
        pte_valid_addr_end = get_pte_number(region[0] +
                                            region[1])

        mem_size = region[1]

        # In-case the start address aligns with a page table entry other than zero
        # and the mem_size is greater than (1024*4096)
        # in case where it overflows the currenty PDE's range then limit the
        # PTE to 1024 and so make the mem_size reflect the actual size taken up
        # in the current PDE
        if (region[1] + (pte_valid_addr_start * 4096) ) > (1024*4096):
            pte_valid_addr_end = 1024
            mem_size = ( (pte_valid_addr_end - pte_valid_addr_start)*4096)

        set_pde_pte_values(pde_index, region[0], mem_size,
                           pte_valid_addr_start, pte_valid_addr_end, region[2])


        if pde_index not in page_tables_list:
                page_tables_list.append(pde_index)

        # IF the current pde couldn't fit the entire requested region size then
        # there is a need to create new PDEs to match the size.
        # Here the overflow_size represents the size that couldn't be fit inside
        # the current PDE, this is will now to used to create a new PDE/PDEs
        # so the size remaining will be
        # requested size - allocated size(in the current PDE)

        overflow_size = region[1] - \
                        ((pte_valid_addr_end -
                          pte_valid_addr_start) * 4096)

        # create all the extra PDEs needed to fit the requested size
        # this loop starts from the current pde till the last pde that is needed
        # the last pde is calcualted as the (start_addr + size) >> 22
        for extra_pde in range(pde_index+1, get_pde_number(
                region[0] + region[1])+1):

            # new pde's start address
            # each page directory entry has a addr range of (1024 *4096)
            # thus the new PDE start address is a multiple of that number
            extra_pde_start_address = extra_pde*(4096*1024)

            # the start address of and extra pde will always be 0
            # and the end address is calculated with the new pde's start address
            # and the overflow_size
            extra_pte_valid_addr_end = get_pte_number(extra_pde_start_address
                                                      + overflow_size)

            # if the overflow_size couldn't be fit inside this new pde then
            # need another pde and so we now need to limit the end of the PTE
            # to 1024 and set the size of this new region to the max possible
            extra_region_size = overflow_size
            if overflow_size > (1024*4096):
                extra_region_size = 1024*4096
                extra_pte_valid_addr_end =  1024

            # load the new PDE's details

            set_pde_pte_values(extra_pde, extra_pde_start_address,
                               extra_region_size,
                               0, extra_pte_valid_addr_end, region[2] )


            # for the next iteration of the loop the size needs to decreased
            overflow_size -= (extra_pte_valid_addr_end) * 4096

            if extra_pde not in page_tables_list:
                page_tables_list.append(extra_pde)
    page_tables_list.sort()


def validate_pde_regions():
    #validation for correct page alignment of the regions
    for key, value in list_of_pde.items():
        for pages_inside_pde in value.page_entries_info:
            if pages_inside_pde.start_addr & (0xFFF) != 0:
                print("Memory Regions are not page aligned",
                      hex(pages_inside_pde.start_addr))
                sys.exit(2)

            #validation for correct page alignment of the regions
            if pages_inside_pde.size & (0xFFF) != 0:
                print("Memory Regions size is not page aligned",
                      hex(pages_inside_pde.size))
                sys.exit(2)

    #validation for spiling of the regions across various
    if validation_issue_memory_overlap[0] == True:
        print("Memory Regions are overlapping at memory address " +
              str(hex(validation_issue_memory_overlap[1]))+
              " with Page directory Entry number " +
              str(validation_issue_memory_overlap[2]))
        sys.exit(2)





# the return value will have the page address and it is assumed to be a 4096 boundary
# hence the output of this API will be a 20bit address of the page table
def address_of_page_table(page_table_number):
    global pd_start_addr

    # location from where the Page tables will be written
    PT_start_addr = pd_start_addr + 4096
    return ( (PT_start_addr + (page_tables_list.index(page_table_number)*4096) >>12))

#     union x86_mmu_pde_pt {
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

def page_directory_create_binary_file():
    global output_buffer
    global output_offset
    for pde in range(1024):
        binary_value = 0 # the page directory entry is not valid

        # if i have a valid entry to populate
        if pde in sorted(list_of_pde.keys()):
            value = list_of_pde[pde]

            present = 1 << 0;
            read_write = ( ( value.page_entries_info[0].permissions >> 1) & 0x1) << 1;
            user_mode = ( ( value.page_entries_info[0].permissions >> 2) & 0x1) << 2;
            pwt = 0 << 3;
            pcd = 0 << 4;
            a = 0 << 5; # this is a read only field
            ps = 0 << 7; # this is a read only field
            page_table = address_of_page_table(value.pde_index) << 12;
            binary_value = (present | read_write | user_mode | pwt | pcd | a | ps | page_table)
            pde_verbose_output(pde, binary_value)

        struct.pack_into(write_4byte_bin,output_buffer, output_offset, binary_value)
        output_offset += struct.calcsize(write_4byte_bin)


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

def page_table_create_binary_file():
    global output_buffer
    global output_offset

    for key, value in sorted(list_of_pde.items()):
        for pte in range(1024):
            binary_value = 0 # the page directory entry is not valid

            valid_pte = 0
            for i in value.page_entries_info:
                temp_value = ((pte >= i.pte_valid_addr_start) and (pte <= i.pte_valid_addr_end))
                if temp_value:
                    perm_for_pte = i.permissions
                valid_pte |= temp_value

            # if i have a valid entry to populate
            if valid_pte:
                present = 1 << 0;
                read_write = ( ( perm_for_pte >> 1) & 0x1) << 1;
                user_mode = ( ( perm_for_pte >> 2) & 0x1) << 2;
                pwt = 0 << 3;
                pcd = 0 << 4;
                a = 0 << 5; # this is a read only field
                d = 0 << 6; # this is a read only field
                pat = 0 << 7
                g = 0<< 8
                alloc = 1 << 9
                custom = 0 <<10

                # This points to the actual memory in the HW
                # totally 20 bits to rep the phy address
                # first 10 is the number got from pde and next 10 is pte
                page_table = ((value.pde_index <<10) |pte) << 12;

                binary_value = (present | read_write | user_mode |
                                pwt | pcd | a | d | pat | g | alloc | custom |
                                page_table)

                pte_verbose_output(key,pte,binary_value)

            struct.pack_into(write_4byte_bin, output_buffer, output_offset, binary_value)
            output_offset += struct.calcsize(write_4byte_bin)


# Read the parameters passed to the file
def parse_args():
    global args

    parser = argparse.ArgumentParser(description = __doc__,
                                     formatter_class = argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-e", "--big-endian", action="store_true",
                        help="Target encodes data in big-endian format"
                        "(little endian is the default)")

    parser.add_argument("-i", "--input",
                        help="Input file from which MMU regions are read.")
    parser.add_argument("-o", "--output",
                        help="Output file into which the page tables are written.")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Lists all the relavent data generated.")
    args = parser.parse_args()

# the format for writing in the binary file would be decided by the
# endian selected
def set_struct_endian_format():
    endian_string = "<"
    if args.big_endian == True:
        endian_string = ">"
    global struct_mmu_regions_format
    global header_values_format
    global write_4byte_bin

    struct_mmu_regions_format = endian_string + "III"
    header_values_format = endian_string + "II"
    write_4byte_bin = endian_string + "I"


def format_string(input_str):
    output_str = '{0: <5}'.format(str(input_str))
    return(output_str)

def pde_verbose_output(pde, binary_value):
    if args.verbose == False:
        return

    global print_string_pde_list

    present = format_string(binary_value & 0x1 )
    read_write = format_string((binary_value >> 1 ) & 0x1 )
    user_mode = format_string((binary_value >> 2 ) & 0x1 )
    pwt = format_string((binary_value >> 3 ) & 0x1 )
    pcd = format_string((binary_value >> 4 ) & 0x1 )
    a = format_string((binary_value >> 5 ) & 0x1 )
    ignored1 = format_string(0)
    ps = format_string((binary_value >> 7 ) & 0x1 )
    ignored2 = format_string(0000)
    page_table_addr = format_string(hex((binary_value >> 12 ) & 0xFFFFF) )

    print_string_pde_list += ( format_string(str(pde))+" | "+(present)+ " | "+\
          (read_write)+ " | "+\
          (user_mode)+ " | "+\
          (pwt)+ " | "+\
          (pcd)+ " | "+\
          (a)+ " | "+\
          (ps)+ " | "+
          page_table_addr +"\n"
    )

def pde_print_elements():
    global print_string_pde_list
    print("PAGE DIRECTORY ")
    print(format_string("PDE")+" | "+ \
          format_string('P')  +" | "+  \
          format_string('rw')   +" | "+  \
          format_string('us')  +" | "+  \
          format_string('pwt')  +" | "+  \
          format_string('pcd')  +" | "+  \
          format_string('a')  +" | "+   \
          format_string('ps')  +" | "+  \
          format_string('Addr page table'))
    print(print_string_pde_list)
    print("END OF PAGE DIRECTORY")


def pte_verbose_output(pde, pte, binary_value):
    global pde_pte_string

    present    = format_string( str((binary_value >> 0) & 0x1))
    read_write = format_string( str((binary_value >> 1) & 0x1))
    user_mode  = format_string( str((binary_value >> 2) & 0x1))
    pwt = format_string( str((binary_value >> 3) & 0x1))
    pcd = format_string( str((binary_value >> 4) & 0x1))
    a   = format_string( str((binary_value >> 5) & 0x1))
    d   = format_string( str((binary_value >> 6) & 0x1))
    pat = format_string( str((binary_value >> 7) & 0x1))
    g   = format_string( str((binary_value >> 8) & 0x1))
    alloc  = format_string( str((binary_value >> 9) & 0x1))
    custom = format_string( str((binary_value >> 10) & 0x3))
    page_table_addr = format_string( str(hex((binary_value >> 12) & 0xFFFFF)))

    print_string_list = ( format_string(str(pte))+" | "+(present)+ " | "+\
          (read_write)+ " | "+\
          (user_mode)+ " | "+\
          (pwt)+ " | "+\
          (pcd)+ " | "+\
          (a)+ " | "+\
          (d)+ " | "+\
          (pat)+ " | "+\
          (g)+ " | "+\
          (alloc)+ " | "+\
          (custom)+ " | "+\
          page_table_addr +"\n"
    )

    if pde in pde_pte_string.keys():
        pde_pte_string[pde] += (print_string_list)
    else:
        pde_pte_string[pde] = print_string_list


def pte_print_elements():
    global pde_pte_string

    for pde,print_string in sorted(pde_pte_string.items()):
        print("\nPAGE TABLE "+str(pde))

        print(format_string("PTE")+" | "+ \
              format_string('P')  +" | "+  \
              format_string('rw')   +" | "+  \
              format_string('us')  +" | "+  \
              format_string('pwt')  +" | "+  \
              format_string('pcd')  +" | "+  \
              format_string('a')  +" | "+   \
              format_string('d')  +" | "+   \
              format_string('pat')  +" | "+   \
              format_string('g')  +" | "+   \
              format_string('alloc')  +" | "+   \
              format_string('custom')  +" | "+   \
              format_string('page addr'))
        print(print_string)
        print("END OF PAGE TABLE "+ str(pde))

def verbose_output():
    if args.verbose == False:
        return

    print("\nTotal Page directory entries " + str(len(list_of_pde.keys())))
    count =0
    for key, value in list_of_pde.items():
        for i in value.page_entries_info:
            count+=1
            print("Memory Region "+str(count) +" start address = "+
                  str(hex(i.start_addr)))

    pde_print_elements()
    pte_print_elements()

def main():
    global output_buffer
    parse_args()

    set_struct_endian_format()

    global input_file
    input_file = open(args.input, 'rb')

    global binary_output_file
    binary_output_file = open(args.output, 'wb')

    # inputfile= file_name
    read_mmu_list_marshal_param()

    #validate the inputs
    validate_pde_regions()

    # The size of the output buffer has to match the number of bytes we write
    # this corresponds to the number of page tables gets created.
    output_buffer = ctypes.create_string_buffer((4096)+
                                                (len(list_of_pde.keys()) *
                                                 4096))

    page_directory_create_binary_file()
    page_table_create_binary_file()

    #write the binary data into the file
    binary_output_file.write(output_buffer);
    binary_output_file.close()

    # verbose output needed by the build system
    verbose_output()

if __name__ == "__main__":
     main()
