#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import logging

import elftools
from elftools.elf.elffile import ELFFile


# ELF section flags
SHF_WRITE = 0x1
SHF_ALLOC = 0x2
SHF_EXEC = 0x4
SHF_WRITE_ALLOC = SHF_WRITE | SHF_ALLOC
SHF_ALLOC_EXEC = SHF_ALLOC | SHF_EXEC


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

    def open(self):
        self.fd = open(self.elffile, "rb")
        self.elf = ELFFile(self.fd)

    def close(self):
        self.fd.close()

    def get_memory_regions(self):
        return self.memory_regions

    def parse(self):
        if self.fd is None:
            self.open()

        for section in self.elf.iter_sections():
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

        return True
