#!/usr/bin/env python3
#
# Copyright (c) 2018 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

"""
This script will relocate .text, .rodata, .data and .bss sections from required files
and places it in the required memory region. This memory region and file
are given to this python script in the form of a string.

Example of such a string would be::

   SRAM2:COPY:/home/xyz/zephyr/samples/hello_world/src/main.c,\
   SRAM1:COPY:/home/xyz/zephyr/samples/hello_world/src/main2.c, \
   FLASH2:NOCOPY:/home/xyz/zephyr/samples/hello_world/src/main3.c

To invoke this script::

   python3 gen_relocate_app.py -i input_string -o generated_linker -c generated_code

Configuration that needs to be sent to the python script.

- If the memory is like SRAM1/SRAM2/CCD/AON then place full object in
  the sections
- If the memory type is appended with _DATA / _TEXT/ _RODATA/ _BSS only the
  selected memory is placed in the required memory region. Others are
  ignored.
- COPY/NOCOPY defines whether the script should generate the relocation code in
  code_relocation.c or not

Multiple regions can be appended together like SRAM2_DATA_BSS
this will place data and bss inside SRAM2.
"""


import sys
import argparse
import os
import glob
import warnings
from elftools.elf.elffile import ELFFile

# This script will create linker commands for text,rodata data, bss section relocation

PRINT_TEMPLATE = """
                KEEP(*({0}))
"""

SECTION_LOAD_MEMORY_SEQ = """
        __{0}_{1}_rom_start = LOADADDR(_{2}_{3}_SECTION_NAME);
"""

LOAD_ADDRESS_LOCATION_FLASH = """
#ifdef CONFIG_XIP
GROUP_DATA_LINK_IN({0}, FLASH)
#else
GROUP_DATA_LINK_IN({0}, {0})
#endif
"""

LOAD_ADDRESS_LOCATION_FLASH_NOCOPY = """
GROUP_LINK_IN({0})
"""

LOAD_ADDRESS_LOCATION_BSS = "GROUP_LINK_IN({0})"

MPU_RO_REGION_START = """

     _{0}_mpu_ro_region_start = ORIGIN({1});

"""

MPU_RO_REGION_END = """

    _{0}_mpu_ro_region_end = .;

"""

# generic section creation format
LINKER_SECTION_SEQ = """

/* Linker section for memory region {2} for  {3} section  */

	SECTION_PROLOGUE(_{2}_{3}_SECTION_NAME,,)
        {{
                . = ALIGN(4);
                {4}
                . = ALIGN(4);
	}} {5}
        __{0}_{1}_end = .;
        __{0}_{1}_start = ADDR(_{2}_{3}_SECTION_NAME);
        __{0}_{1}_size = SIZEOF(_{2}_{3}_SECTION_NAME);
"""

LINKER_SECTION_SEQ_MPU = """

/* Linker section for memory region {2} for {3} section  */

	SECTION_PROLOGUE(_{2}_{3}_SECTION_NAME,,)
        {{
                __{0}_{1}_start = .;
                {4}
#if {6}
                . = ALIGN({6});
#else
                MPU_ALIGN(__{0}_{1}_size);
#endif
                __{0}_{1}_end = .;
	}} {5}
        __{0}_{1}_size = __{0}_{1}_end - __{0}_{1}_start;
"""

SOURCE_CODE_INCLUDES = """
/* Auto generated code. Do not modify.*/
#include <zephyr/zephyr.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
"""

EXTERN_LINKER_VAR_DECLARATION = """
extern char __{0}_{1}_start[];
extern char __{0}_{1}_rom_start[];
extern char __{0}_{1}_size[];
"""


DATA_COPY_FUNCTION = """
void data_copy_xip_relocation(void)
{{
{0}
}}
"""

BSS_ZEROING_FUNCTION = """
void bss_zeroing_relocation(void)
{{
{0}
}}
"""

MEMCPY_TEMPLATE = """
	z_early_memcpy(&__{0}_{1}_start, &__{0}_{1}_rom_start,
		           (size_t) &__{0}_{1}_size);

"""

MEMSET_TEMPLATE = """
 	z_early_memset(&__{0}_bss_start, 0,
		           (size_t) &__{0}_bss_size);
"""


def find_sections(filename, full_list_of_sections):
    with open(filename, 'rb') as obj_file_desc:
        full_lib = ELFFile(obj_file_desc)
        if not full_lib:
            sys.exit("Error parsing file: " + filename)

        sections = [x for x in full_lib.iter_sections()]

        for section in sections:

            if ".text." in section.name:
                full_list_of_sections["text"].append(section.name)

            if ".rodata." in section.name:
                full_list_of_sections["rodata"].append(section.name)

            if ".data." in section.name:
                full_list_of_sections["data"].append(section.name)

            if ".bss." in section.name:
                full_list_of_sections["bss"].append(section.name)

            # Common variables will be placed in the .bss section
            # only after linking in the final executable. This "if" finds
            # common symbols and warns the user of the problem.
            # The solution to which is simply assigning a 0 to
            # bss variable and it will go to the required place.
            if ".symtab" in section.name:
                symbols = [x for x in section.iter_symbols()]
                for symbol in symbols:
                    if symbol.entry["st_shndx"] == 'SHN_COMMON':
                        warnings.warn("Common variable found. Move "+
                                      symbol.name + " to bss by assigning it to 0/NULL")

    return full_list_of_sections


def assign_to_correct_mem_region(memory_type,
                                 full_list_of_sections, complete_list_of_sections):
    all_regions = False
    iteration_sections = {"text": False, "rodata": False, "data": False, "bss": False}
    if "_TEXT" in memory_type:
        iteration_sections["text"] = True
        memory_type = memory_type.replace("_TEXT", "")
    if "_RODATA" in memory_type:
        iteration_sections["rodata"] = True
        memory_type = memory_type.replace("_RODATA", "")
    if "_DATA" in memory_type:
        iteration_sections["data"] = True
        memory_type = memory_type.replace("_DATA", "")
    if "_BSS" in memory_type:
        iteration_sections["bss"] = True
        memory_type = memory_type.replace("_BSS", "")
    if not (iteration_sections["data"] or iteration_sections["bss"] or
            iteration_sections["text"] or iteration_sections["rodata"]):
        all_regions = True

    pos = memory_type.find('_')
    if pos in range(len(memory_type)):
        align_size = int(memory_type[pos+1:])
        memory_type = memory_type[:pos]
        mpu_align[memory_type] = align_size

    if memory_type in complete_list_of_sections:
        for iter_sec in ["text", "rodata", "data", "bss"]:
            if ((iteration_sections[iter_sec] or all_regions) and
                    full_list_of_sections[iter_sec] != []):
                complete_list_of_sections[memory_type][iter_sec] += (
                    full_list_of_sections[iter_sec])
    else:
        # new memory type was found. in which case just assign the
        # full_list_of_sections to the memorytype dict
        tmp_list = {"text": [], "rodata": [], "data": [], "bss": []}
        for iter_sec in ["text", "rodata", "data", "bss"]:
            if ((iteration_sections[iter_sec] or all_regions) and
                    full_list_of_sections[iter_sec] != []):
                tmp_list[iter_sec] = full_list_of_sections[iter_sec]

        complete_list_of_sections[memory_type] = tmp_list

    return complete_list_of_sections


def print_linker_sections(list_sections):
    print_string = ''
    for section in sorted(list_sections):
        print_string += PRINT_TEMPLATE.format(section)
    return print_string


def string_create_helper(region, memory_type,
                         full_list_of_sections, load_address_in_flash, is_copy):
    linker_string = ''
    if load_address_in_flash:
        if is_copy:
            load_address_string = LOAD_ADDRESS_LOCATION_FLASH.format(memory_type)
        else:
            load_address_string = LOAD_ADDRESS_LOCATION_FLASH_NOCOPY.format(memory_type)
    else:
        load_address_string = LOAD_ADDRESS_LOCATION_BSS.format(memory_type)
    if full_list_of_sections[region]:
        # Create a complete list of funcs/ variables that goes in for this
        # memory type
        tmp = print_linker_sections(full_list_of_sections[region])
        if memory_type == 'SRAM' and region in {'data', 'bss'}:
            linker_string += tmp
        else:
            if memory_type != 'SRAM' and region == 'rodata':
                align_size = 0
                if memory_type in mpu_align:
                    align_size = mpu_align[memory_type]

                linker_string += LINKER_SECTION_SEQ_MPU.format(memory_type.lower(), region, memory_type.upper(),
                                                               region.upper(), tmp, load_address_string, align_size)
            else:
                if memory_type == 'SRAM' and region == 'text':
                    align_size = 0
                    linker_string += LINKER_SECTION_SEQ_MPU.format(memory_type.lower(), region, memory_type.upper(),
                                                                   region.upper(), tmp, load_address_string, align_size)
                else:
                    linker_string += LINKER_SECTION_SEQ.format(memory_type.lower(), region, memory_type.upper(),
                                                               region.upper(), tmp, load_address_string)
            if load_address_in_flash:
                linker_string += SECTION_LOAD_MEMORY_SEQ.format(memory_type.lower(), region, memory_type.upper(),
                                                                region.upper())
    return linker_string


def generate_linker_script(linker_file, sram_data_linker_file, sram_bss_linker_file, complete_list_of_sections):
    gen_string = ''
    gen_string_sram_data = ''
    gen_string_sram_bss = ''

    for memory_type, full_list_of_sections in \
            sorted(complete_list_of_sections.items()):

        is_copy = bool("|COPY" in memory_type)
        memory_type = memory_type.split("|", 1)[0]

        if memory_type != "SRAM" and is_copy:
            gen_string += MPU_RO_REGION_START.format(memory_type.lower(), memory_type.upper())

        gen_string += string_create_helper("text", memory_type, full_list_of_sections, 1, is_copy)
        gen_string += string_create_helper("rodata", memory_type, full_list_of_sections, 1, is_copy)

        if memory_type != "SRAM" and is_copy:
            gen_string += MPU_RO_REGION_END.format(memory_type.lower())

        if memory_type == 'SRAM':
            gen_string_sram_data += string_create_helper("data", memory_type, full_list_of_sections, 1, 1)
            gen_string_sram_bss += string_create_helper("bss", memory_type, full_list_of_sections, 0, 1)
        else:
            gen_string += string_create_helper("data", memory_type, full_list_of_sections, 1, 1)
            gen_string += string_create_helper("bss", memory_type, full_list_of_sections, 0, 1)

    # finally writing to the linker file
    with open(linker_file, "w") as file_desc:
        file_desc.write(gen_string)

    with open(sram_data_linker_file, "w") as file_desc:
        file_desc.write(gen_string_sram_data)

    with open(sram_bss_linker_file, "w") as file_desc:
        file_desc.write(gen_string_sram_bss)


def generate_memcpy_code(memory_type, full_list_of_sections, code_generation):
    all_sections = True
    generate_section = {"text": False, "rodata": False, "data": False, "bss": False}
    for section_name in ["_TEXT", "_RODATA", "_DATA", "_BSS"]:
        if section_name in memory_type:
            generate_section[section_name.lower()[1:]] = True
            memory_type = memory_type.replace(section_name, "")
            all_sections = False

    if all_sections:
        generate_section["text"] = True
        generate_section["rodata"] = True
        generate_section["data"] = True
        generate_section["bss"] = True

    # add all the regions that needs to be copied on boot up
    for mtype in ["text", "rodata", "data"]:
        if memory_type == "SRAM" and mtype == "data":
            continue

        if full_list_of_sections[mtype] and generate_section[mtype]:
            code_generation["copy_code"] += MEMCPY_TEMPLATE.format(memory_type.lower(), mtype)
            code_generation["extern"] += EXTERN_LINKER_VAR_DECLARATION.format(
                memory_type.lower(), mtype)

    # add for all the bss data that needs to be zeroed on boot up
    if full_list_of_sections["bss"] and generate_section["bss"] and memory_type != "SRAM":
        code_generation["zero_code"] += MEMSET_TEMPLATE.format(memory_type.lower())
        code_generation["extern"] += EXTERN_LINKER_VAR_DECLARATION.format(
            memory_type.lower(), "bss")

    return code_generation


def dump_header_file(header_file, code_generation):
    code_string = ''
    # create a dummy void function if there is no code to generate for
    # bss/data/text regions

    code_string += code_generation["extern"]

    if code_generation["copy_code"]:
        code_string += DATA_COPY_FUNCTION.format(code_generation["copy_code"])
    else:
        code_string += DATA_COPY_FUNCTION.format("void;")
    if code_generation["zero_code"]:
        code_string += BSS_ZEROING_FUNCTION.format(code_generation["zero_code"])
    else:
        code_string += BSS_ZEROING_FUNCTION.format("return;")

    with open(header_file, "w") as header_file_desc:
        header_file_desc.write(SOURCE_CODE_INCLUDES)
        header_file_desc.write(code_string)


def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-d", "--directory", required=True,
                        help="obj file's directory")
    parser.add_argument("-i", "--input_rel_dict", required=True,
                        help="input src:memory type(sram2 or ccm or aon etc) string")
    parser.add_argument("-o", "--output", required=False, help="Output ld file")
    parser.add_argument("-s", "--output_sram_data", required=False,
                        help="Output sram data ld file")
    parser.add_argument("-b", "--output_sram_bss", required=False,
                        help="Output sram bss ld file")
    parser.add_argument("-c", "--output_code", required=False,
                        help="Output relocation code header file")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Verbose Output")
    args = parser.parse_args()


# return the absolute path for the object file.
def get_obj_filename(searchpath, filename):
    # get the object file name which is almost always pended with .obj
    obj_filename = filename.split("/")[-1] + ".obj"

    for dirpath, _, files in os.walk(searchpath):
        for filename1 in files:
            if filename1 == obj_filename:
                if filename.split("/")[-2] in dirpath.split("/")[-1]:
                    fullname = os.path.join(dirpath, filename1)
                    return fullname


# Create a dict with key as memory type and files as a list of values.
def create_dict_wrt_mem():
    # need to support wild card *
    rel_dict = dict()
    if args.input_rel_dict == '':
        sys.exit("Disable CONFIG_CODE_DATA_RELOCATION if no file needs relocation")
    for line in args.input_rel_dict.split(';'):
        if ':' not in line:
            continue

        mem_region, copy_flag, file_name = line.split(':', 2)

        file_name_list = glob.glob(file_name)
        if not file_name_list:
            warnings.warn("File: "+file_name+" Not found")
            continue
        if mem_region == '':
            continue
        if args.verbose:
            print("Memory region ", mem_region, " Selected for file:", file_name_list)

        mem_region = "|".join((mem_region, copy_flag))

        if mem_region in rel_dict:
            rel_dict[mem_region].extend(file_name_list)
        else:
            rel_dict[mem_region] = file_name_list

    return rel_dict


def main():
    global mpu_align
    mpu_align = {}
    parse_args()
    searchpath = args.directory
    linker_file = args.output
    sram_data_linker_file = args.output_sram_data
    sram_bss_linker_file = args.output_sram_bss
    rel_dict = create_dict_wrt_mem()
    complete_list_of_sections = {}

    # Create/or truncate file contents if it already exists
    # raw = open(linker_file, "w")

    # for each memory_type, create text/rodata/data/bss sections for all obj files
    for memory_type, files in rel_dict.items():
        full_list_of_sections = {"text": [], "rodata": [], "data": [], "bss": []}

        for filename in files:
            obj_filename = get_obj_filename(searchpath, filename)
            # the obj file wasn't found. Probably not compiled.
            if not obj_filename:
                continue

            full_list_of_sections = find_sections(obj_filename, full_list_of_sections)

        # cleanup and attach the sections to the memory type after cleanup.
        complete_list_of_sections = assign_to_correct_mem_region(memory_type,
                                                                 full_list_of_sections,
                                                                 complete_list_of_sections)

    generate_linker_script(linker_file, sram_data_linker_file,
                           sram_bss_linker_file, complete_list_of_sections)

    code_generation = {"copy_code": '', "zero_code": '', "extern": ''}
    for mem_type, list_of_sections in sorted(complete_list_of_sections.items()):

        if "|COPY" in mem_type:
            mem_type = mem_type.split("|", 1)[0]
            code_generation = generate_memcpy_code(mem_type,
                                               list_of_sections, code_generation)

    dump_header_file(args.output_code, code_generation)


if __name__ == '__main__':
    main()
