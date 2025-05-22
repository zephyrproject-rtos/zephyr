#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Class for Dictionary-based Logging Database
"""

import base64
import copy
import json

from .mipi_syst import gen_syst_xml_file
from .utils import extract_one_string_in_section
from .utils import find_string_in_mappings


ARCHS = {
    "arc" : {
        "kconfig": "CONFIG_ARC",
    },
    "arm" : {
        "kconfig": "CONFIG_ARM",
    },
    "arm64" : {
        "kconfig": "CONFIG_ARM64",
    },
    "mips" : {
        "kconfig": "CONFIG_MIPS",
    },
    "sparc" : {
        "kconfig": "CONFIG_SPARC",
    },
    "x86" : {
        "kconfig": "CONFIG_X86",
    },
    "nios2" : {
        "kconfig": "CONFIG_NIOS2",

        # Small static strings are put into section "datas"
        # so we need to include them also.
        #
        # See include/arch/nios2/linker.ld on .sdata.*
        # for explanation.
        "extra_string_section": ['datas'],
    },
    "posix" : {
        "kconfig": "CONFIG_ARCH_POSIX",
    },
    "riscv32e" : {
        "kconfig": "CONFIG_RISCV_ISA_RV32E",
    },
    "riscv" : {
        "kconfig": "CONFIG_RISCV",
    },
    "xtensa" : {
        "kconfig": "CONFIG_XTENSA",
    },
}


class LogDatabase():
    """Class of log database"""
    # Update this if database format of dictionary based logging
    # has changed
    ZEPHYR_DICT_LOG_VER = 3

    LITTLE_ENDIAN = True
    BIG_ENDIAN = False

    def __init__(self):
        new_db = {}

        new_db['version'] = self.ZEPHYR_DICT_LOG_VER
        new_db['target'] = {}
        new_db['log_subsys'] = {}
        new_db['log_subsys']['log_instances'] = {}
        new_db['build_id'] = None
        new_db['arch'] = None
        new_db['kconfigs'] = {}

        self.database = new_db


    def get_version(self):
        """Get Database Version"""
        return self.database['version']


    def get_build_id(self):
        """Get Build ID"""
        return self.database['build_id']


    def set_build_id(self, build_id):
        """Set Build ID in Database"""
        self.database['build_id'] = build_id


    def get_arch(self):
        """Get the Target Architecture"""
        return self.database['arch']


    def set_arch(self, arch):
        """Set the Target Architecture"""
        self.database['arch'] = arch


    def get_tgt_bits(self):
        """Get Target Bitness: 32 or 64"""
        if 'bits' in self.database['target']:
            return self.database['target']['bits']

        return None


    def set_tgt_bits(self, bits):
        """Set Target Bitness: 32 or 64"""
        self.database['target']['bits'] = bits


    def is_tgt_64bit(self):
        """Return True if target is 64-bit, False if 32-bit.
        None if error."""
        if 'bits' not in self.database['target']:
            return None

        if self.database['target']['bits'] == 32:
            return False

        if self.database['target']['bits'] == 64:
            return True

        return None


    def get_tgt_endianness(self):
        """
        Get Target Endianness.

        Return True if little endian, False if big.
        """
        if 'little_endianness' in self.database['target']:
            return self.database['target']['little_endianness']

        return None


    def set_tgt_endianness(self, endianness):
        """
        Set Target Endianness

        True if little endian, False if big.
        """
        self.database['target']['little_endianness'] = endianness


    def is_tgt_little_endian(self):
        """Return True if target is little endian"""
        if 'little_endianness' not in self.database['target']:
            return None

        return self.database['target']['little_endianness'] == self.LITTLE_ENDIAN


    def get_string_mappings(self):
        """Get string mappings to database"""
        return self.database['string_mappings']


    def set_string_mappings(self, database):
        """Add string mappings to database"""
        self.database['string_mappings'] = database


    def has_string_mappings(self):
        """Return True if there are string mappings in database"""
        if 'string_mappings' in self.database:
            return True

        return False


    def has_string_sections(self):
        """Return True if there are any static string sections"""
        if 'sections' not in self.database:
            return False

        return len(self.database['sections']) != 0


    def __find_string_in_mappings(self, string_ptr):
        """
        Find string pointed by string_ptr in the string mapping
        list. Return None if not found.
        """
        return find_string_in_mappings(self.database['string_mappings'], string_ptr)


    def __find_string_in_sections(self, string_ptr):
        """
        Find string pointed by string_ptr in the binary data
        sections. Return None if not found.
        """
        for _, sect in self.database['sections'].items():
            one_str = extract_one_string_in_section(sect, string_ptr)

            if one_str is not None:
                return one_str

        return None


    def find_string(self, string_ptr):
        """Find string pointed by string_ptr in the database.
        Return None if not found."""
        one_str = None

        if self.has_string_mappings():
            one_str = self.__find_string_in_mappings(string_ptr)

        if one_str is None and self.has_string_sections():
            one_str = self.__find_string_in_sections(string_ptr)

        return one_str


    def add_log_instance(self, source_id, name, level, address):
        """Add one log instance into database"""
        self.database['log_subsys']['log_instances'][source_id] = {
            'source_id' : source_id,
            'name'      : name,
            'level'     : level,
            'addr'      : address,
        }


    def get_log_source_string(self, domain_id, source_id):
        """Get the source string based on source ID"""
        # JSON stores key as string, so we need to convert
        src_id = str(source_id)

        if src_id in self.database['log_subsys']['log_instances']:
            return self.database['log_subsys']['log_instances'][src_id]['name']

        return f"unknown<{domain_id}:{source_id}>"


    def add_kconfig(self, name, val):
        """Add a kconfig name-value pair into database"""
        self.database['kconfigs'][name] = val


    def get_kconfigs(self):
        """Return kconfig name-value pairs"""
        return self.database['kconfigs']


    @staticmethod
    def read_json_database(db_file_name):
        """Read database from file and return a LogDatabase object"""
        try:
            with open(db_file_name, "r", encoding="iso-8859-1") as db_fd:
                json_db = json.load(db_fd)
        except (OSError, json.JSONDecodeError):
            return None

        # Decode data in JSON back into binary data
        if 'sections' in json_db:
            for _, sect in json_db['sections'].items():
                sect['data'] = base64.b64decode(sect['data_b64'])

        database = LogDatabase()
        database.database = json_db

        # JSON encodes the addresses in string mappings as literal strings.
        # So convert them back to integers, as this is needed for partial
        # matchings.
        if database.has_string_mappings():
            new_str_map = {}

            for addr, one_str in database.get_string_mappings().items():
                new_str_map[int(addr)] = one_str

            database.set_string_mappings(new_str_map)

        return database


    @staticmethod
    def write_json_database(db_file_name, database):
        """Write the database into file"""
        json_db = copy.deepcopy(database.database)

        # Make database object into something JSON can dump
        if 'sections' in json_db:
            for _, sect in json_db['sections'].items():
                encoded = base64.b64encode(sect['data'])
                sect['data_b64'] = encoded.decode('ascii')
                del sect['data']

        try:
            with open(db_file_name, "w", encoding="iso-8859-1") as db_fd:
                db_fd.write(json.dumps(json_db))
        except OSError:
            return False

        return True

    @staticmethod
    def write_syst_database(db_file_name, database):
        """
        Write the database into MIPI Sys-T Collateral XML file
        """

        try:
            with open(db_file_name, "w", encoding="iso-8859-1") as db_fd:
                xml = gen_syst_xml_file(database)
                db_fd.write(xml)
        except OSError:
            return False

        return True
