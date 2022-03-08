#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Dictionary-based Logging Database Generator

This takes the built Zephyr ELF binary and produces a JSON database
file for dictionary-based logging. This database is used together
with the parser to decode binary log messages.
"""

import argparse
import logging
import os
import re
import struct
import sys

import dictionary_parser.log_database
from dictionary_parser.log_database import LogDatabase

import elftools
from elftools.elf.elffile import ELFFile
from elftools.elf.descriptions import describe_ei_data
from elftools.elf.sections import SymbolTableSection


LOGGER_FORMAT = "%(name)s: %(levelname)s: %(message)s"
logger = logging.getLogger(os.path.basename(sys.argv[0]))


# Sections that contains static strings
STATIC_STRING_SECTIONS = [
    'rodata',
    '.rodata',
    'pinned.rodata',
    'log_strings_sections'
]


def parse_args():
    """Parse command line arguments"""
    argparser = argparse.ArgumentParser()

    argparser.add_argument("elffile", help="Zephyr ELF binary")
    argparser.add_argument("dbfile", help="Dictionary Logging Database file")
    argparser.add_argument("--build", help="Build ID")
    argparser.add_argument("--build-header",
                           help="Header file containing BUILD_VERSION define")
    argparser.add_argument("--debug", action="store_true",
                           help="Print extra debugging information")
    argparser.add_argument("-v", "--verbose", action="store_true",
                           help="Print more information")

    return argparser.parse_args()


def find_elf_sections(elf, sh_name):
    """Find all sections in ELF file"""
    for section in elf.iter_sections():
        if section.name == sh_name:
            ret = {
                'name'    : section.name,
                'size'    : section['sh_size'],
                'start'   : section['sh_addr'],
                'end'     : section['sh_addr'] + section['sh_size'] - 1,
                'data'    : section.data(),
            }

            return ret

    return None


def get_kconfig_symbols(elf):
    """Get kconfig symbols from the ELF file"""
    for section in elf.iter_sections():
        if isinstance(section, SymbolTableSection):
            return {sym.name: sym.entry.st_value
                    for sym in section.iter_symbols()
                       if sym.name.startswith("CONFIG_")}

    raise LookupError("Could not find symbol table")


def find_log_const_symbols(elf):
    """Extract all "log_const_*" symbols from ELF file"""
    symbol_tables = [s for s in elf.iter_sections()
                     if isinstance(s, elftools.elf.sections.SymbolTableSection)]

    ret_list = []

    for section in symbol_tables:
        if not isinstance(section, elftools.elf.sections.SymbolTableSection):
            continue

        if section['sh_entsize'] == 0:
            continue

        for symbol in section.iter_symbols():
            if symbol.name.startswith("log_const_"):
                ret_list.append(symbol)

    return ret_list


def parse_log_const_symbols(database, log_const_section, log_const_symbols):
    """Find the log instances and map source IDs to names"""
    if database.is_tgt_little_endian():
        formatter = "<"
    else:
        formatter = ">"

    if database.is_tgt_64bit():
        # 64-bit pointer to string
        formatter += "Q"
    else:
        # 32-bit pointer to string
        formatter += "L"

    # log instance level
    formatter += "B"

    datum_size = struct.calcsize(formatter)

    # Get the address of first log instance
    first_offset = log_const_symbols[0].entry['st_value']
    for sym in log_const_symbols:
        if sym.entry['st_value'] < first_offset:
            first_offset = sym.entry['st_value']

    first_offset -= log_const_section['start']

    # find all log_const_*
    for sym in log_const_symbols:
        # Find data offset in log_const_section for this symbol
        offset = sym.entry['st_value'] - log_const_section['start']

        idx_s = offset
        idx_e = offset + datum_size

        datum = log_const_section['data'][idx_s:idx_e]

        if len(datum) != datum_size:
            # Not enough data to unpack
            continue

        str_ptr, level = struct.unpack(formatter, datum)

        # Offset to rodata section for string
        instance_name = database.find_string(str_ptr)

        logger.info("Found Log Instance: %s, level: %d", instance_name, level)

        # source ID is simply the element index in the log instance array
        source_id = int((offset - first_offset) / sym.entry['st_size'])

        database.add_log_instance(source_id, instance_name, level, sym.entry['st_value'])


def extract_elf_information(elf, database):
    """Extract information from ELF file and store in database"""
    e_ident = elf.header['e_ident']
    elf_data = describe_ei_data(e_ident['EI_DATA'])

    if elf_data == elftools.elf.descriptions._DESCR_EI_DATA['ELFDATA2LSB']:
        database.set_tgt_endianness(LogDatabase.LITTLE_ENDIAN)
    elif elf_data == elftools.elf.descriptions._DESCR_EI_DATA['ELFDATA2MSB']:
        database.set_tgt_endianness(LogDatabase.BIG_ENDIAN)
    else:
        logger.error("Cannot determine endianness from ELF file, exiting...")
        sys.exit(1)


def process_kconfigs(elf, database):
    """Process kconfigs to extract information"""
    kconfigs = get_kconfig_symbols(elf)

    # 32 or 64-bit target
    database.set_tgt_bits(64 if "CONFIG_64BIT" in kconfigs else 32)

    # Architecture
    for name, arch in dictionary_parser.log_database.ARCHS.items():
        if arch['kconfig'] in kconfigs:
            database.set_arch(name)
            break

    # Put some kconfigs into the database
    #
    # Use 32-bit timestamp? or 64-bit?
    if "CONFIG_LOG_TIMESTAMP_64BIT" in kconfigs:
        database.add_kconfig("CONFIG_LOG_TIMESTAMP_64BIT",
                             kconfigs['CONFIG_LOG_TIMESTAMP_64BIT'])


def extract_static_string_sections(elf, database):
    """Extract sections containing static strings"""
    string_sections = STATIC_STRING_SECTIONS

    # Some architectures may put static strings into additional sections.
    # So need to extract them too.
    arch_data = dictionary_parser.log_database.ARCHS[database.get_arch()]
    if "extra_string_section" in arch_data:
        string_sections.extend(arch_data['extra_string_section'])

    for name in string_sections:
        content = find_elf_sections(elf, name)
        if content is None:
            continue

        logger.info("Found section: %s, 0x%x - 0x%x",
                    name, content['start'], content['end'])
        database.add_string_section(name, content)

    if not database.has_string_sections():
        logger.error("Cannot find any static string sections in ELF, exiting...")
        sys.exit(1)


def extract_logging_subsys_information(elf, database):
    """
    Extract logging subsys related information and store in database.

    For example, this extracts the list of log instances to establish
    mapping from source ID to name.
    """
    # Extract log constant section for module names
    section_log_const = find_elf_sections(elf, "log_const_sections")
    if section_log_const is None:
        # ESP32 puts "log_const_*" info log_static_section instead of log_const_sections
        section_log_const = find_elf_sections(elf, "log_static_section")

    if section_log_const is None:
        logger.error("Cannot find section 'log_const_sections' in ELF file, exiting...")
        sys.exit(1)

    # Find all "log_const_*" symbols and parse them
    log_const_symbols = find_log_const_symbols(elf)
    parse_log_const_symbols(database, section_log_const, log_const_symbols)


def main():
    """Main function of database generator"""
    args = parse_args()

    # Setup logging
    logging.basicConfig(format=LOGGER_FORMAT)
    if args.verbose:
        logger.setLevel(logging.INFO)
    elif args.debug:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.WARNING)

    elffile = open(args.elffile, "rb")
    if not elffile:
        logger.error("ERROR: Cannot open ELF file: %s, exiting...", args.elffile)
        sys.exit(1)

    logger.info("ELF file %s", args.elffile)
    logger.info("Database file %s", args.dbfile)

    elf = ELFFile(elffile)

    database = LogDatabase()

    if args.build_header:
        with open(args.build_header) as f:
            for l in f:
                match = re.match(r'\s*#define\s+BUILD_VERSION\s+(.*)', l)
                if match:
                    database.set_build_id(match.group(1))
                    break

    if args.build:
        database.set_build_id(args.build)
        logger.info("Build ID: %s", args.build)

    extract_elf_information(elf, database)

    process_kconfigs(elf, database)

    logger.info("Target: %s, %d-bit", database.get_arch(), database.get_tgt_bits())
    if database.is_tgt_little_endian():
        logger.info("Endianness: Little")
    else:
        logger.info("Endianness: Big")

    # Extract sections from ELF files that contain strings
    extract_static_string_sections(elf, database)

    # Extract information related to logging subsystem
    extract_logging_subsys_information(elf, database)

    # Write database file
    if not LogDatabase.write_json_database(args.dbfile, database):
        logger.error("ERROR: Cannot open database file for write: %s, exiting...", args.dbfile)
        sys.exit(1)

    elffile.close()


if __name__ == "__main__":
    main()
