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

One can also specify the program header for a given memory region:

   SRAM2\\ :phdr0:COPY:/home/xyz/zephyr/samples/hello_world/src/main.c

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
- NOKEEP will suppress the default behavior of marking every relocated symbol
  with KEEP() in the generated linker script.

Multiple regions can be appended together like SRAM2_DATA_BSS
this will place data and bss inside SRAM2.
"""


import sys
import argparse
import os
import glob
import warnings
from collections import defaultdict
from enum import Enum
from pathlib import Path
from typing import NamedTuple
from typing import NewType
from typing import Tuple

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

MemoryRegion = NewType('MemoryRegion', str)


class SectionKind(Enum):
    TEXT = "text"
    RODATA = "rodata"
    DATA = "data"
    BSS = "bss"
    LITERAL = "literal"

    def __str__(self):
        return self.name

    @classmethod
    def for_section_named(cls, name: str):
        """
        Return the kind of section that includes a section with the given name.

        >>> SectionKind.for_section_with_name(".rodata.str1.4")
        <SectionKind.RODATA: 'rodata'>
        >>> SectionKind.for_section_with_name(".device_deps")
        None
        """
        if ".text." in name:
            return cls.TEXT
        elif ".rodata." in name:
            return cls.RODATA
        elif ".data." in name:
            return cls.DATA
        elif ".bss." in name:
            return cls.BSS
        elif ".literal." in name:
            return cls.LITERAL
        else:
            return None


class OutputSection(NamedTuple):
    obj_file_name: str
    section_name: str
    keep: bool = True


PRINT_TEMPLATE = """
                KEEP(*{obj_file_name}({section_name}))
"""

PRINT_TEMPLATE_NOKEEP = """
                *{obj_file_name}({section_name})
"""

SECTION_LOAD_MEMORY_SEQ = """
        __{0}_{1}_rom_start = LOADADDR(.{0}_{1}_reloc);
"""

LOAD_ADDRESS_LOCATION_FLASH = """
#ifdef CONFIG_XIP
GROUP_DATA_LINK_IN({0}, ROMABLE_REGION)
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

	SECTION_PROLOGUE(.{0}_{1}_reloc,,)
        {{
                . = ALIGN(4);
                {4}
                . = ALIGN(4);
	}} {5}
        __{0}_{1}_reloc_end = .;
        __{0}_{1}_reloc_start = ADDR(.{0}_{1}_reloc);
        __{0}_{1}_reloc_size = __{0}_{1}_reloc_end - __{0}_{1}_reloc_start;
"""

LINKER_SECTION_SEQ_MPU = """

/* Linker section for memory region {2} for {3} section  */

	SECTION_PROLOGUE(.{0}_{1}_reloc,,)
        {{
                __{0}_{1}_reloc_start = .;
                {4}
#if {6}
                . = ALIGN({6});
#else
                MPU_ALIGN(__{0}_{1}_reloc_size);
#endif
                __{0}_{1}_reloc_end = .;
	}} {5}
        __{0}_{1}_reloc_size = __{0}_{1}_reloc_end - __{0}_{1}_reloc_start;
"""

SOURCE_CODE_INCLUDES = """
/* Auto generated code. Do not modify.*/
#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
"""

EXTERN_LINKER_VAR_DECLARATION = """
extern char __{0}_{1}_reloc_start[];
extern char __{0}_{1}_rom_start[];
extern char __{0}_{1}_reloc_size[];
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
	z_early_memcpy(&__{0}_{1}_reloc_start, &__{0}_{1}_rom_start,
		           (size_t) &__{0}_{1}_reloc_size);

"""

MEMSET_TEMPLATE = """
	z_early_memset(&__{0}_bss_reloc_start, 0,
		           (size_t) &__{0}_bss_reloc_size);
"""


def region_is_default_ram(region_name: str) -> bool:
    """
    Test whether a memory region with the given name is the system's default
    RAM region or not.

    This is used to determine whether some items need to be omitted from
    custom regions and instead be placed in the default. In particular, mutable
    data placed in the default RAM section is ignored and is allowed to be
    handled normally by the linker because it is placed in that region anyway.
    """
    return region_name == args.default_ram_region


def find_sections(filename: str) -> 'dict[SectionKind, list[OutputSection]]':
    """
    Locate relocatable sections in the given object file.

    The output value maps categories of sections to the list of actual sections
    located in the object file that fit in that category.
    """
    obj_file_path = Path(filename)

    with open(obj_file_path, 'rb') as obj_file_desc:
        full_lib = ELFFile(obj_file_desc)
        if not full_lib:
            sys.exit("Error parsing file: " + filename)

        sections = [x for x in full_lib.iter_sections()]
        out = defaultdict(list)

        for section in sections:
            section_kind = SectionKind.for_section_named(section.name)
            if section_kind is None:
                continue

            out[section_kind].append(
                OutputSection(obj_file_path.name, section.name)
            )

            # Common variables will be placed in the .bss section
            # only after linking in the final executable. This "if" finds
            # common symbols and warns the user of the problem.
            # The solution to which is simply assigning a 0 to
            # bss variable and it will go to the required place.
            if isinstance(section, SymbolTableSection):
                def is_common_symbol(s):
                    return s.entry["st_shndx"] == "SHN_COMMON"

                for symbol in filter(is_common_symbol, section.iter_symbols()):
                    warnings.warn("Common variable found. Move "+
                                  symbol.name + " to bss by assigning it to 0/NULL")

    return out


def assign_to_correct_mem_region(
    memory_region: str,
    full_list_of_sections: 'dict[SectionKind, list[OutputSection]]'
) -> 'dict[MemoryRegion, dict[SectionKind, list[OutputSection]]]':
    """
    Generate a mapping of memory region to collection of output sections to be
    placed in each region.
    """
    use_section_kinds, memory_region = section_kinds_from_memory_region(memory_region)

    memory_region, _, align_size = memory_region.partition('_')
    if align_size:
        mpu_align[memory_region] = int(align_size)

    keep_sections = '|NOKEEP' not in memory_region
    memory_region = memory_region.replace('|NOKEEP', '')

    output_sections = {}
    for used_kind in use_section_kinds:
        # Pass through section kinds that go into this memory region
        output_sections[used_kind] = [
            section._replace(keep=keep_sections)
            for section in full_list_of_sections[used_kind]
        ]

    return {MemoryRegion(memory_region): output_sections}


def section_kinds_from_memory_region(memory_region: str) -> 'Tuple[set[SectionKind], str]':
    """
    Get the section kinds requested by the given memory region name.

    Region names can be like RAM_RODATA_TEXT or just RAM; a section kind may
    follow the region name. If no kinds are specified all are assumed.

    In addition to the parsed kinds, the input region minus specifiers for those
    kinds is returned.

    >>> section_kinds_from_memory_region('SRAM2_TEXT')
    ({<SectionKind.TEXT: 'text'>}, 'SRAM2')
    """
    out = set()
    for kind in SectionKind:
        specifier = f"_{kind}"
        if specifier in memory_region:
            out.add(kind)
            memory_region = memory_region.replace(specifier, "")
    if not out:
        # No listed kinds implies all of the kinds
        out = set(SectionKind)
    return (out, memory_region)


def print_linker_sections(list_sections: 'list[OutputSection]'):
    out = ''
    for section in sorted(list_sections):
        template = PRINT_TEMPLATE if section.keep else PRINT_TEMPLATE_NOKEEP
        out += template.format(obj_file_name=section.obj_file_name,
                               section_name=section.section_name)
    return out

def add_phdr(memory_type, phdrs):
    return f'{memory_type} {phdrs[memory_type] if memory_type in phdrs else ""}'


def string_create_helper(
    kind: SectionKind,
    memory_type,
    full_list_of_sections: 'dict[SectionKind, list[OutputSection]]',
    load_address_in_flash,
    is_copy,
    phdrs
):
    linker_string = ''
    if load_address_in_flash:
        if is_copy:
            load_address_string = LOAD_ADDRESS_LOCATION_FLASH.format(add_phdr(memory_type, phdrs))
        else:
            load_address_string = LOAD_ADDRESS_LOCATION_FLASH_NOCOPY.format(add_phdr(memory_type, phdrs))
    else:
        load_address_string = LOAD_ADDRESS_LOCATION_BSS.format(add_phdr(memory_type, phdrs))
    if full_list_of_sections[kind]:
        # Create a complete list of funcs/ variables that goes in for this
        # memory type
        tmp = print_linker_sections(full_list_of_sections[kind])
        if region_is_default_ram(memory_type) and kind in (SectionKind.DATA, SectionKind.BSS):
            linker_string += tmp
        else:
            if not region_is_default_ram(memory_type) and kind is SectionKind.RODATA:
                align_size = 0
                if memory_type in mpu_align:
                    align_size = mpu_align[memory_type]

                linker_string += LINKER_SECTION_SEQ_MPU.format(memory_type.lower(), kind.value, memory_type.upper(),
                                                               kind, tmp, load_address_string, align_size)
            else:
                if region_is_default_ram(memory_type) and kind in (SectionKind.TEXT, SectionKind.LITERAL):
                    align_size = 0
                    linker_string += LINKER_SECTION_SEQ_MPU.format(memory_type.lower(), kind.value, memory_type.upper(),
                                                                   kind, tmp, load_address_string, align_size)
                else:
                    linker_string += LINKER_SECTION_SEQ.format(memory_type.lower(), kind.value, memory_type.upper(),
                                                               kind, tmp, load_address_string)
            if load_address_in_flash:
                linker_string += SECTION_LOAD_MEMORY_SEQ.format(memory_type.lower(), kind.value, memory_type.upper(),
                                                                kind)
    return linker_string


def generate_linker_script(linker_file, sram_data_linker_file, sram_bss_linker_file,
                           complete_list_of_sections, phdrs):
    gen_string = ''
    gen_string_sram_data = ''
    gen_string_sram_bss = ''

    for memory_type, full_list_of_sections in \
            sorted(complete_list_of_sections.items()):

        is_copy = bool("|COPY" in memory_type)
        memory_type = memory_type.split("|", 1)[0]

        if region_is_default_ram(memory_type) and is_copy:
            gen_string += MPU_RO_REGION_START.format(memory_type.lower(), memory_type.upper())

        gen_string += string_create_helper(SectionKind.LITERAL, memory_type, full_list_of_sections, 1, is_copy, phdrs)
        gen_string += string_create_helper(SectionKind.TEXT, memory_type, full_list_of_sections, 1, is_copy, phdrs)
        gen_string += string_create_helper(SectionKind.RODATA, memory_type, full_list_of_sections, 1, is_copy, phdrs)

        if region_is_default_ram(memory_type) and is_copy:
            gen_string += MPU_RO_REGION_END.format(memory_type.lower())

        if region_is_default_ram(memory_type):
            gen_string_sram_data += string_create_helper(SectionKind.DATA, memory_type, full_list_of_sections, 1, 1, phdrs)
            gen_string_sram_bss += string_create_helper(SectionKind.BSS, memory_type, full_list_of_sections, 0, 1, phdrs)
        else:
            gen_string += string_create_helper(SectionKind.DATA, memory_type, full_list_of_sections, 1, 1, phdrs)
            gen_string += string_create_helper(SectionKind.BSS, memory_type, full_list_of_sections, 0, 1, phdrs)

    # finally writing to the linker file
    with open(linker_file, "w") as file_desc:
        file_desc.write(gen_string)

    with open(sram_data_linker_file, "w") as file_desc:
        file_desc.write(gen_string_sram_data)

    with open(sram_bss_linker_file, "w") as file_desc:
        file_desc.write(gen_string_sram_bss)


def generate_memcpy_code(memory_type, full_list_of_sections, code_generation):
    generate_sections, memory_type = section_kinds_from_memory_region(memory_type)

    # Non-BSS sections get copied to the destination memory, except data in
    # main memory which gets copied automatically.
    for kind in (SectionKind.TEXT, SectionKind.RODATA, SectionKind.DATA):
        if region_is_default_ram(memory_type) and kind is SectionKind.DATA:
            continue

        if kind in generate_sections and full_list_of_sections[kind]:
            code_generation["copy_code"] += MEMCPY_TEMPLATE.format(memory_type.lower(), kind.value)
            code_generation["extern"] += EXTERN_LINKER_VAR_DECLARATION.format(
                memory_type.lower(), kind.value)

    # BSS sections in main memory are automatically zeroed; others need to have
    # zeroing code generated.
    if (SectionKind.BSS in generate_sections
        and full_list_of_sections[SectionKind.BSS]
        and not region_is_default_ram(memory_type)
    ):
        code_generation["zero_code"] += MEMSET_TEMPLATE.format(memory_type.lower())
        code_generation["extern"] += EXTERN_LINKER_VAR_DECLARATION.format(
            memory_type.lower(), SectionKind.BSS.value)

    return code_generation


def dump_header_file(header_file, code_generation):
    code_string = ''
    # create a dummy void function if there is no code to generate for
    # bss/data/text regions

    code_string += code_generation["extern"]

    if code_generation["copy_code"]:
        code_string += DATA_COPY_FUNCTION.format(code_generation["copy_code"])
    else:
        code_string += DATA_COPY_FUNCTION.format("return;")
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
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)
    parser.add_argument("-d", "--directory", required=True,
                        help="obj file's directory")
    parser.add_argument("-i", "--input_rel_dict", required=True, type=argparse.FileType('r'),
                        help="input file with dict src:memory type(sram2 or ccm or aon etc)")
    parser.add_argument("-o", "--output", required=False, help="Output ld file")
    parser.add_argument("-s", "--output_sram_data", required=False,
                        help="Output sram data ld file")
    parser.add_argument("-b", "--output_sram_bss", required=False,
                        help="Output sram bss ld file")
    parser.add_argument("-c", "--output_code", required=False,
                        help="Output relocation code header file")
    parser.add_argument("-R", "--default_ram_region", default='SRAM',
                        help="Name of default RAM memory region for system")
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


# Extracts all possible components for the input string:
# <mem_region>[\ :program_header]:<flag_1>[;<flag_2>...]:<file_1>[;<file_2>...]
# Returns a 4-tuple with them: (mem_region, program_header, flags, files)
# If no `program_header` is defined, returns an empty string
def parse_input_string(line):
    # Be careful when splitting by : to avoid breaking absolute paths on Windows
    mem_region, rest = line.split(':', 1)

    phdr = ''
    if mem_region.endswith(' '):
        mem_region = mem_region.rstrip()
        phdr, rest = rest.split(':', 1)

    # Split lists by semicolons, in part to support generator expressions
    flag_list, file_list = (lst.split(';') for lst in rest.split(':', 1))

    return mem_region, phdr, flag_list, file_list


# Create a dict with key as memory type and files as a list of values.
# Also, return another dict with program headers for memory regions
def create_dict_wrt_mem():
    # need to support wild card *
    rel_dict = dict()
    phdrs = dict()

    input_rel_dict = args.input_rel_dict.read().splitlines()
    if not input_rel_dict:
        sys.exit("Disable CONFIG_CODE_DATA_RELOCATION if no file needs relocation")
    for line in input_rel_dict:
        if ':' not in line:
            continue

        mem_region, phdr, flag_list, file_list = parse_input_string(line)

        # Handle any program header
        if phdr != '':
            phdrs[mem_region] = f':{phdr}'

        file_name_list = []
        # Use glob matching on each file in the list
        for file_glob in file_list:
            glob_results = glob.glob(file_glob)
            if not glob_results:
                warnings.warn("File: "+file_glob+" Not found")
                continue
            elif len(glob_results) > 1:
                warnings.warn("Regex in file lists is deprecated, please use file(GLOB) instead")
            file_name_list.extend(glob_results)
        if len(file_name_list) == 0:
            continue
        if mem_region == '':
            continue
        if args.verbose:
            print("Memory region ", mem_region, " Selected for files:", file_name_list)

        mem_region = "|".join((mem_region, *flag_list))

        if mem_region in rel_dict:
            rel_dict[mem_region].extend(file_name_list)
        else:
            rel_dict[mem_region] = file_name_list

    return rel_dict, phdrs


def main():
    global mpu_align
    mpu_align = {}
    parse_args()
    searchpath = args.directory
    linker_file = args.output
    sram_data_linker_file = args.output_sram_data
    sram_bss_linker_file = args.output_sram_bss
    rel_dict, phdrs = create_dict_wrt_mem()
    complete_list_of_sections: 'dict[MemoryRegion, dict[SectionKind, list[OutputSection]]]' \
        = defaultdict(lambda: defaultdict(list))

    # Create/or truncate file contents if it already exists
    # raw = open(linker_file, "w")

    # for each memory_type, create text/rodata/data/bss sections for all obj files
    for memory_type, files in rel_dict.items():
        full_list_of_sections: 'dict[SectionKind, list[OutputSection]]' = defaultdict(list)

        for filename in files:
            obj_filename = get_obj_filename(searchpath, filename)
            # the obj file wasn't found. Probably not compiled.
            if not obj_filename:
                continue

            file_sections = find_sections(obj_filename)
            # Merge sections from file into collection of sections for all files
            for category, sections in file_sections.items():
                full_list_of_sections[category].extend(sections)

        # cleanup and attach the sections to the memory type after cleanup.
        sections_by_category = assign_to_correct_mem_region(memory_type, full_list_of_sections)
        for (region, section_category_map) in sections_by_category.items():
            for (category, sections) in section_category_map.items():
                complete_list_of_sections[region][category].extend(sections)

    generate_linker_script(linker_file, sram_data_linker_file,
                           sram_bss_linker_file, complete_list_of_sections, phdrs)

    code_generation = {"copy_code": '', "zero_code": '', "extern": ''}
    for mem_type, list_of_sections in sorted(complete_list_of_sections.items()):

        if "|COPY" in mem_type:
            mem_type = mem_type.split("|", 1)[0]
            code_generation = generate_memcpy_code(mem_type,
                                               list_of_sections, code_generation)

    dump_header_file(args.output_code, code_generation)


if __name__ == '__main__':
    main()
