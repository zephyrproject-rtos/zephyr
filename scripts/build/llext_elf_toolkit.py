#!/usr/bin/env python3
#
# Copyright (c) 2025 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

"""
ELF utilities shared by LLEXT scripts
"""

import struct

import elftools.elf.elffile
import elftools.elf.sections


def get_target_specific_structure(struct_desc: str, ptr_size: int, endianness: str):
    """
    Obtain target-specific 'struct.Struct' from structure description string.
    Internally, pointer-sized elements ('P') is replaced with an integer field
    of the proper size, and the endianness indicator is prepended automatically.

    Parameters:
        struct_template:
            Python 'struct' descriptor, with 'P' for pointer-sized fields
            and without an endianness indicator

        ptr_size: Pointer size of target, in bytes (4/8)
        endianness: Endianness of target ('little'/'big')
    """
    assert ptr_size in (4, 8)
    assert endianness in ['little', 'big']

    endspec = "<" if endianness == 'little' else ">"
    if ptr_size == 4:
        ptrspec = "I"
    elif ptr_size == 8:
        ptrspec = "Q"

    return struct.Struct(endspec + struct_desc.replace("P", ptrspec))


# ELF Shdr flag applied to the export table section, to indicate
# the section has already been prepared by this script. This is
# mostly a security measure to prevent the script from running
# twice on the same ELF file, which can result in catastrophic
# failures if SLID-based linking is enabled (in this case, the
# preparation process is destructive).
#
# This flag is part of the SHF_MASKOS mask, of which all bits
# are "reserved for operating system-specific semantics".
# See: https://refspecs.linuxbase.org/elf/gabi4+/ch4.sheader.html
SHF_LLEXT_PREPARATION_DONE = 0x08000000


class SectionDescriptor:
    """ELF Section descriptor

    This is a wrapper class around pyelftools' "Section" object.

    Attributes:
        name: Section name
        size: Section size
        flags: Section flags
        offset: File offset to section
        shdr_index: Section header index
        shdr_offset: File offset to section header
        section: pyelftools Section object
    """

    def __init__(self, elffile: elftools.elf.elffile.ELFFile, section_name: str):
        self.name = section_name
        self.section = elffile.get_section_by_name(section_name)
        if not isinstance(self.section, elftools.elf.sections.Section):
            raise KeyError(f"section {section_name} not found")

        self.shdr_index = elffile.get_section_index(section_name)
        self.shdr_offset = elffile['e_shoff'] + self.shdr_index * elffile['e_shentsize']
        self.size = self.section['sh_size']
        self.flags = self.section['sh_flags']
        self.offset = self.section['sh_offset']
