#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import logging
import struct

import elftools
from elftools.elf.elffile import ELFFile
from enum import IntEnum


# ELF section flags
SHF_WRITE = 0x1
SHF_ALLOC = 0x2
SHF_EXEC = 0x4
SHF_WRITE_ALLOC = SHF_WRITE | SHF_ALLOC
SHF_ALLOC_EXEC = SHF_ALLOC | SHF_EXEC

# Must match enum in thread_info.c
class ThreadInfoOffset(IntEnum):
    THREAD_INFO_OFFSET_VERSION = 0
    THREAD_INFO_OFFSET_K_CURR_THREAD = 1
    THREAD_INFO_OFFSET_K_THREADS = 2
    THREAD_INFO_OFFSET_T_ENTRY = 3
    THREAD_INFO_OFFSET_T_NEXT_THREAD = 4
    THREAD_INFO_OFFSET_T_STATE = 5
    THREAD_INFO_OFFSET_T_USER_OPTIONS = 6
    THREAD_INFO_OFFSET_T_PRIO = 7
    THREAD_INFO_OFFSET_T_STACK_PTR = 8
    THREAD_INFO_OFFSET_T_NAME = 9
    THREAD_INFO_OFFSET_T_ARCH = 10
    THREAD_INFO_OFFSET_T_PREEMPT_FLOAT = 11
    THREAD_INFO_OFFSET_T_COOP_FLOAT = 12
    THREAD_INFO_OFFSET_T_ARM_EXC_RETURN = 13
    THREAD_INFO_OFFSET_T_ARC_RELINQUISH_CAUSE = 14

    def __int__(self):
        return self.value


logger = logging.getLogger("parser")


class CoredumpElfFile():
    """
    Class to parse ELF file for memory content in various sections.
    There are read-only sections (e.g. text and rodata) where
    the memory content does not need to be dumped via coredump
    and can be retrieved from the ELF file.
    """

    def __init__(self, elffile):
        self.elffile = elffile
        self.fd = None
        self.elf = None
        self.memory_regions = list()
        self.kernel_thread_info_offsets = None
        self.kernel_thread_info_num_offsets = None
        self.kernel_thread_info_size_t_size = None

    def open(self):
        self.fd = open(self.elffile, "rb")
        self.elf = ELFFile(self.fd)

    def close(self):
        self.fd.close()

    def get_memory_regions(self):
        return self.memory_regions

    def get_kernel_thread_info_size_t_size(self):
        return self.kernel_thread_info_size_t_size

    def has_kernel_thread_info(self):
        return self.kernel_thread_info_offsets is not None

    def get_kernel_thread_info_offset(self, thread_info_offset_index):
        if self.has_kernel_thread_info() and thread_info_offset_index <= ThreadInfoOffset.THREAD_INFO_OFFSET_T_ARC_RELINQUISH_CAUSE:
            return self.kernel_thread_info_offsets[thread_info_offset_index]
        else:
            return None

    def parse(self):
        if self.fd is None:
            self.open()

        kernel_thread_info_offsets_segment = None
        kernel_thread_info_num_offsets_segment = None
        _kernel_thread_info_offsets = None
        _kernel_thread_info_num_offsets = None
        _kernel_thread_info_size_t_size = None

        for section in self.elf.iter_sections():
            # Find symbols for _kernel_thread_info data
            if isinstance(section, elftools.elf.sections.SymbolTableSection):
                _kernel_thread_info_offsets = section.get_symbol_by_name("_kernel_thread_info_offsets")
                _kernel_thread_info_num_offsets = section.get_symbol_by_name("_kernel_thread_info_num_offsets")
                _kernel_thread_info_size_t_size = section.get_symbol_by_name("_kernel_thread_info_size_t_size")

            # REALLY NEED to match exact type as all other sections
            # (debug, text, etc.) are descendants where
            # isinstance() would match.
            if type(section) is not elftools.elf.sections.Section: # pylint: disable=unidiomatic-typecheck
                continue

            size = section['sh_size']
            flags = section['sh_flags']
            sec_start = section['sh_addr']
            sec_end = sec_start + size - 1

            store = False
            sect_desc = "?"

            if section['sh_type'] == 'SHT_PROGBITS':
                if (flags & SHF_ALLOC_EXEC) == SHF_ALLOC_EXEC:
                    # Text section
                    store = True
                    sect_desc = "text"
                elif (flags & SHF_WRITE_ALLOC) == SHF_WRITE_ALLOC:
                    # Data section
                    #
                    # Running app changes the content so no need
                    # to store
                    pass
                elif (flags & SHF_ALLOC) == SHF_ALLOC:
                    # Read only data section
                    store = True
                    sect_desc = "read-only data"

            if store:
                mem_region = {"start": sec_start, "end": sec_end, "data": section.data()}
                logger.info("ELF Section: 0x%x to 0x%x of size %d (%s)" %
                            (mem_region["start"],
                             mem_region["end"],
                             len(mem_region["data"]),
                             sect_desc))

                self.memory_regions.append(mem_region)

        if _kernel_thread_info_size_t_size is not None and \
           _kernel_thread_info_num_offsets is not None and \
           _kernel_thread_info_offsets is not None:
            for seg in self.elf.iter_segments():
                if seg.header['p_type'] != 'PT_LOAD':
                    continue

                # Store segment of kernel_thread_info_offsets
                info_offsets_symbol = _kernel_thread_info_offsets[0]
                if info_offsets_symbol['st_value'] >= seg['p_vaddr'] and info_offsets_symbol['st_value'] < seg['p_vaddr'] + seg['p_filesz']:
                    kernel_thread_info_offsets_segment = seg

                # Store segment of kernel_thread_info_num_offsets
                num_offsets_symbol = _kernel_thread_info_num_offsets[0]
                if num_offsets_symbol['st_value'] >= seg['p_vaddr'] and num_offsets_symbol['st_value'] < seg['p_vaddr'] + seg['p_filesz']:
                    kernel_thread_info_num_offsets_segment = seg

                # Read and store size_t size
                size_t_size_symbol = _kernel_thread_info_size_t_size[0]
                if size_t_size_symbol['st_value'] >= seg['p_vaddr'] and size_t_size_symbol['st_value'] < seg['p_vaddr'] + seg['p_filesz']:
                    offset = size_t_size_symbol['st_value'] - seg['p_vaddr'] + seg['p_offset']
                    self.elf.stream.seek(offset)
                    self.kernel_thread_info_size_t_size = struct.unpack('B', self.elf.stream.read(size_t_size_symbol['st_size']))[0]

            struct_format = "I"
            if self.kernel_thread_info_size_t_size == 8:
                struct_format = "Q"

            # Read and store count of offset values
            num_offsets_symbol = _kernel_thread_info_num_offsets[0]
            offset = num_offsets_symbol['st_value'] - kernel_thread_info_num_offsets_segment['p_vaddr'] + kernel_thread_info_num_offsets_segment['p_offset']
            self.elf.stream.seek(offset)
            self.kernel_thread_info_num_offsets = struct.unpack(struct_format, self.elf.stream.read(num_offsets_symbol['st_size']))[0]

            array_format = ""
            for _ in range(self.kernel_thread_info_num_offsets):
                array_format = array_format + struct_format

            # Read and store array of offset values
            info_offsets_symbol = _kernel_thread_info_offsets[0]
            offset = info_offsets_symbol['st_value'] - kernel_thread_info_offsets_segment['p_vaddr'] + kernel_thread_info_offsets_segment['p_offset']
            self.elf.stream.seek(offset)
            self.kernel_thread_info_offsets = struct.unpack(array_format, self.elf.stream.read(info_offsets_symbol['st_size']))

        return True
