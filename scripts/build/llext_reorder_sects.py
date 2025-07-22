#!/usr/bin/env python3
#
# Copyright (c) 2025 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''Reorder sections in an ELF file.
The LLEXT loader groups sections into regions. If sections are
not ordered correctly, regions may overlap, causing the loader to fail.
This script reorders sections in an ELF file to ensure regions will not overlap.
'''

import argparse
import logging
import os
import shutil
import sys
from enum import Enum

from elftools.elf.constants import SH_FLAGS
from elftools.elf.elffile import ELFFile

logger = logging.getLogger('reorder')
LOGGING_FORMAT = '[%(levelname)s][%(name)s] %(message)s'


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument('file', help='Object file')
    parser.add_argument('--debug', action='store_true', help='Print extra debugging information')

    return parser.parse_args()


def get_field_size(struct, name):
    for field in struct.subcons:
        if field.name == name:
            return field.sizeof()

    raise ValueError(f"Unknown field name: {name}")


def get_field_offset(struct, name):
    offset = 0

    for field in struct.subcons:
        if field.name == name:
            break
        offset += field.sizeof()

    if offset < struct.sizeof():
        return offset

    raise ValueError(f"Unknown field name: {name}")


def write_field_in_struct_bytearr(elf, bytearr, name, num, ent_idx=0):
    """Write a field in an ELF struct represented by a bytearray.

    Given a bytearray read from an ELF file containing one or more entries
    from a table (ELF header entry, section header entry, or symbol), edit
    fields inline. User will write this bytearray back to the file.
    """
    if name[0] == 'e':
        header = elf.structs.Elf_Ehdr
    elif name[0:2] == 'sh':
        header = elf.structs.Elf_Shdr
    elif name[0:2] == 'st':
        header = elf.structs.Elf_Sym
    else:
        raise ValueError(f"Unable to identify struct for {name}")

    size = get_field_size(header, name)

    # ent_idx is 0 for ELF header
    offset = (ent_idx * header.sizeof()) + get_field_offset(header, name)

    if elf.little_endian:
        field = num.to_bytes(size, 'little')
    else:
        field = num.to_bytes(size, 'big')

    bytearr[offset : offset + size] = field


class LlextMem(Enum):
    LLEXT_MEM_TEXT = 0
    LLEXT_MEM_DATA = 1
    LLEXT_MEM_RODATA = 2
    LLEXT_MEM_BSS = 3
    LLEXT_MEM_EXPORT = 4
    LLEXT_MEM_SYMTAB = 5
    LLEXT_MEM_STRTAB = 6
    LLEXT_MEM_SHSTRTAB = 7
    LLEXT_MEM_PREINIT = 8
    LLEXT_MEM_INIT = 9
    LLEXT_MEM_FINI = 10

    LLEXT_MEM_COUNT = 11


class LlextRegion:
    def __init__(self, mem_idx):
        self.mem_idx = mem_idx
        self.sections = []
        self.sh_type = None
        self.sh_flags = 0
        self.sh_addr = 0
        self.sh_offset = 0
        self.sh_size = 0
        self.sh_info = None
        self.sh_addralign = 0

    def add_section(self, section):
        # Causes unexpected reordering if detached section doesn't have name of type ".detach.*"
        if section.name.startswith('.detach'):
            logger.debug(f'Section {section.name} is detached, not expanding region')
            return

        if not self.sections:  # First section in the region
            self.sh_type = section['sh_type']
            self.sh_flags = section['sh_flags']
            self.sh_addr = section['sh_addr']
            self.sh_offset = section['sh_offset']
            self.sh_size = section['sh_size']
            self.sh_info = section['sh_info']
            self.sh_addralign = section['sh_addralign']
        else:
            # See llext_map_sections
            self.sh_addr = min(self.sh_addr, section['sh_addr'])
            bottom_of_region = min(self.sh_offset, section['sh_offset'])
            top_of_region = max(
                self.sh_offset + self.sh_size, section['sh_offset'] + section['sh_size']
            )
            self.sh_offset = bottom_of_region
            self.sh_size = top_of_region - bottom_of_region
            self.sh_addralign = max(self.sh_addralign, section['sh_addralign'])
        self.sections.append(section)

    def bottom(self, field):
        return getattr(self, field) + self.sh_info

    def top(self, field):
        return getattr(self, field) + self.sh_size - 1

    def overlaps(self, field, other):
        return (
            self.bottom(field) <= other.top(field) and self.top(field) >= other.bottom(field)
        ) or (other.bottom(field) <= self.top(field) and other.top(field) >= self.bottom(field))


def get_region_for_section(elf, idx, section):
    mem_idx = LlextMem.LLEXT_MEM_COUNT.value

    # llext_find_tables
    if (
        section['sh_type'] == 'SHT_SYMTAB'
        and elf.header['e_type'] == 'ET_REL'
        or section['sh_type'] == 'SHT_DYNSYM'
        and elf.header['e_type'] == 'ET_DYN'
    ):
        mem_idx = LlextMem.LLEXT_MEM_SYMTAB.value
    elif section['sh_type'] == 'SHT_STRTAB':
        if elf.header['e_shstrndx'] == idx:
            mem_idx = LlextMem.LLEXT_MEM_SHSTRTAB.value
        else:
            mem_idx = LlextMem.LLEXT_MEM_STRTAB.value

    # llext_map_sections
    if mem_idx == LlextMem.LLEXT_MEM_COUNT.value:
        if section['sh_type'] == 'SHT_NOBITS':
            mem_idx = LlextMem.LLEXT_MEM_BSS.value
        elif section['sh_type'] == 'SHT_PROGBITS':
            if section['sh_flags'] & SH_FLAGS.SHF_EXECINSTR:
                mem_idx = LlextMem.LLEXT_MEM_TEXT.value
            elif section['sh_flags'] & SH_FLAGS.SHF_WRITE:
                mem_idx = LlextMem.LLEXT_MEM_DATA.value
            else:
                mem_idx = LlextMem.LLEXT_MEM_RODATA.value
        elif section['sh_type'] == 'SHT_PREINIT_ARRAY':
            mem_idx = LlextMem.LLEXT_MEM_PREINIT.value
        elif section['sh_type'] == 'SHT_INIT_ARRAY':
            mem_idx = LlextMem.LLEXT_MEM_INIT.value
        elif section['sh_type'] == 'SHT_FINI_ARRAY':
            mem_idx = LlextMem.LLEXT_MEM_FINI.value

        if section.name == '.exported_sym':
            mem_idx = LlextMem.LLEXT_MEM_EXPORT.value

        if not (section['sh_flags'] & SH_FLAGS.SHF_ALLOC) or section['sh_size'] == 0:
            mem_idx = LlextMem.LLEXT_MEM_COUNT.value

    if mem_idx == LlextMem.LLEXT_MEM_COUNT.value:
        logger.debug(f'Section {idx} name {section.name} not assigned to any region')
    else:
        logger.debug(f'Section {idx} name {section.name} maps to region {mem_idx}')

    return mem_idx


# llext_map_sections
def needs_reordering(elf, region_for_sects):
    x = None
    y = None
    needs_reorder = False

    for i in range(LlextMem.LLEXT_MEM_COUNT.value):
        x = region_for_sects[i]

        if x.sh_type == 'SHT_NULL' or x.sh_size == 0:
            continue

        prepad = x.sh_offset & (x.sh_addralign - 1)

        if elf.header['e_type'] == 'ET_DYN':
            x.sh_addr -= prepad

        x.sh_offset -= prepad
        x.sh_size += prepad
        x.sh_info = prepad

    for i in range(LlextMem.LLEXT_MEM_COUNT.value):
        for j in range(i + 1, LlextMem.LLEXT_MEM_COUNT.value):
            x = region_for_sects[i]
            y = region_for_sects[j]

            if (
                x.sh_type == 'SHT_NULL'
                or x.sh_size == 0
                or y.sh_type == 'SHT_NULL'
                or y.sh_size == 0
            ):
                continue

            if (
                i in (LlextMem.LLEXT_MEM_DATA.value, LlextMem.LLEXT_MEM_RODATA.value)
            ) and j == LlextMem.LLEXT_MEM_EXPORT.value:
                continue

            if LlextMem.LLEXT_MEM_EXPORT.value in (i, j):
                continue

            if (
                (elf.header['e_type'] == 'ET_DYN')
                and (x.sh_flags & SH_FLAGS.SHF_ALLOC)
                and (y.sh_flags & SH_FLAGS.SHF_ALLOC)
                and x.overlaps('sh_addr', y)
            ):
                logger.debug(
                    'Region {} VMA range ({}-{}) overlaps with {} ({}-{})'.format(
                        i,
                        hex(x.bottom('sh_addr')),
                        hex(x.top('sh_addr')),
                        j,
                        hex(y.bottom('sh_addr')),
                        hex(y.top('sh_addr')),
                    )
                )
                needs_reorder = True
                break

            if LlextMem.LLEXT_MEM_BSS.value in (i, j):
                continue

            if x.overlaps('sh_offset', y):
                logger.debug(
                    'Region {} ({}-{}) overlaps with {} ({}-{})'.format(
                        i,
                        hex(x.bottom('sh_offset')),
                        hex(x.top('sh_offset')),
                        j,
                        hex(y.bottom('sh_offset')),
                        hex(y.top('sh_offset')),
                    )
                )
                needs_reorder = True
                break

    return needs_reorder


def reorder_sections(f, bak):
    reorder = False

    elf = ELFFile(bak)

    if not (elf.header['e_type'] == 'ET_REL' or elf.header['e_type'] == 'ET_DYN'):
        logger.warning('Script not applicable to {}'.format(elf.header['e_type']))
        return reorder

    elfh = bytearray()

    e_shentsize = elf.header['e_shentsize']

    e_shstrndx = 0
    e_shoff = 0

    sht = bytearray()

    # Read in and write out ELF header (to move file pointer forward)
    bak.seek(0)
    elfh += bak.read(elf.header['e_ehsize'])
    f.write(elfh)

    # Read in program header table (if present) and write it out
    if elf.header['e_phnum'] > 0:
        bak.seek(elf.header['e_phoff'])
        phdrs = bak.read(elf.header['e_phnum'] * elf.header['e_phentsize'])
        f.write(phdrs)

    # Read in section header table
    bak.seek(elf.header['e_shoff'])
    bak_sht = bak.read(elf.header['e_shnum'] * elf.header['e_shentsize'])

    # Get list of sections ordered by region
    region_for_sects = []
    for i in range(LlextMem.LLEXT_MEM_COUNT.value + 1):
        region_for_sects.append(LlextRegion(i))

    sections = list(elf.iter_sections())
    for i, section in enumerate(sections[1:], start=1):
        region_idx = get_region_for_section(elf, i, section)
        region_for_sects[region_idx].add_section(section)

    reorder = needs_reordering(elf, region_for_sects)

    if not reorder:
        logger.info('No reordering needed')
        return reorder

    if elf.header['e_type'] == 'ET_DYN':
        logger.warning(
            'Extension section reordering needed. Give compiler a linker script'
            + 'to control memory layout for ET_DYN ELF files'
        )
        return not reorder

    logger.info('Reordering sections...')

    # Create ordered list of sections
    ordered_sections = [elf.get_section(0)]
    for mem_idx in range(LlextMem.LLEXT_MEM_COUNT.value + 1):
        ordered_sections.extend(region_for_sects[mem_idx].sections)

    # Map unordered section indices to ordered indices (for sh_link)
    index_mapping = [0] * (elf.header['e_shnum'] + 1)
    for i, section in enumerate(ordered_sections):
        index_mapping[elf.get_section_index(section.name)] = i

    # Write out sections and build updated section header table
    for i, section in enumerate(ordered_sections):
        # Accommodate for alignment
        sh_addralign = section['sh_addralign']
        # How many bytes we overshot alignment by
        past_by = f.tell() % sh_addralign if sh_addralign != 0 else 0
        if past_by > 0:
            f.seek(f.tell() + (sh_addralign - past_by))
            logger.debug(f'Adjusting section offset for sh_addralign {sh_addralign}')
        logger.debug(f'Section {i} at 0x{f.tell():x} with name {section.name}')

        if section.name == '.shstrtab':
            e_shstrndx = i
            logger.debug(f'\te_shstrndx = {e_shstrndx}')

        # Add and update section header to the section header table
        bak_sht_idx = elf.get_section_index(section.name)
        sht += bak_sht[bak_sht_idx * e_shentsize : (bak_sht_idx + 1) * e_shentsize]
        write_field_in_struct_bytearr(elf, sht, 'sh_offset', f.tell(), i)

        if (
            section['sh_type'] == 'SHT_REL'
            or section['sh_type'] == 'SHT_RELA'
            or section['sh_type'] == 'SHT_SYMTAB'
        ):
            sh_link = index_mapping[section['sh_link']]
            write_field_in_struct_bytearr(elf, sht, 'sh_link', sh_link, i)
            logger.debug(f'\tsh_link updated from {section["sh_link"]} to {sh_link}')

            if section['sh_type'] != 'SHT_SYMTAB':
                sh_info = index_mapping[section['sh_info']]
                write_field_in_struct_bytearr(elf, sht, 'sh_info', sh_info, i)
                logger.debug(f'\tsh_info updated from {section["sh_info"]} to {sh_info}')

        # Read in section data
        bak.seek(section['sh_offset'])
        data = bytearray() + bak.read(section['sh_size'])

        # Correct symbol table
        if section['sh_type'] == 'SHT_SYMTAB':
            symtab = elf._make_symbol_table_section(section.header, section.name)
            for j in range(section['sh_size'] // section['sh_entsize']):
                st_shndx = symtab.get_symbol(j).entry['st_shndx']
                if st_shndx not in ('SHN_UNDEF', 'SHN_COMMON', 'SHN_ABS'):
                    logger.debug(
                        f'\tsym {j} st_shndx updated from {st_shndx} to {index_mapping[st_shndx]}'
                    )
                    st_shndx = index_mapping[st_shndx]
                    write_field_in_struct_bytearr(elf, data, 'st_shndx', st_shndx, j)

        # Write out section data
        f.write(data)

    # Write out section header table
    e_shoff = f.tell()
    logger.debug(f'e_shoff = 0x{e_shoff:x}')

    f.write(sht)

    # Update and rewrite out ELF header
    write_field_in_struct_bytearr(elf, elfh, 'e_shoff', e_shoff)
    write_field_in_struct_bytearr(elf, elfh, 'e_shstrndx', e_shstrndx)

    f.seek(0)
    f.write(elfh)

    return reorder


def main():
    args = parse_args()

    logging.basicConfig(format=LOGGING_FORMAT)

    if args.debug:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.WARNING)

    if not os.path.isfile(args.file):
        logger.error(f'Cannot find file {args.file}, exiting...')
        sys.exit(1)

    with open(args.file, 'rb') as f:
        try:
            ELFFile(f)
        except Exception:
            logger.error(f'File {args.file} is not a valid ELF file, exiting...')
            sys.exit(1)

    try:
        # Back up extension.llext
        shutil.copy(args.file, f'{args.file}.bak')

        reordered = False
        # Read from extension.llext.bak, write out to extension.llext
        with open(f'{args.file}.bak', 'rb') as bak, open(args.file, 'wb') as f:
            reordered = reorder_sections(f, bak)

        if not reordered:
            logger.info('Exiting without reordering...')
            os.remove(f'{args.file}')
            os.rename(f'{args.file}.bak', args.file)
        else:
            logger.warning(f'Extension sections reordered. Backup saved at {args.file}.bak')
    except Exception as e:
        logger.error(f'An error occurred while processing the file: {e}')
        sys.exit(1)


if __name__ == "__main__":
    main()
