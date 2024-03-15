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
import string
import struct
import sys

import dictionary_parser.log_database
from dictionary_parser.log_database import LogDatabase
from dictionary_parser.utils import extract_one_string_in_section
from dictionary_parser.utils import find_string_in_mappings

import elftools
from elftools.elf.constants import SH_FLAGS
from elftools.elf.elffile import ELFFile
from elftools.elf.descriptions import describe_ei_data
from elftools.elf.sections import SymbolTableSection
from elftools.dwarf.descriptions import (
    describe_DWARF_expr
)
from elftools.dwarf.locationlists import (
    LocationExpr, LocationParser
)


LOGGER_FORMAT = "%(name)s: %(levelname)s: %(message)s"
logger = logging.getLogger(os.path.basename(sys.argv[0]))


# Sections that contains static strings
STATIC_STRING_SECTIONS = [
    'rodata',
    '.rodata',
    'pinned.rodata',
]

# Sections that contains static strings but are not part of the binary (allocable).
REMOVED_STRING_SECTIONS = [
    'log_strings'
]


# Regulation expression to match DWARF location
DT_LOCATION_REGEX = re.compile(r"\(DW_OP_addr: ([0-9a-f]+)")


# Format string for pointers (default for 32-bit pointers)
PTR_FMT = '0x%08x'


# Potential string encodings. Add as needed.
STR_ENCODINGS = [
    'ascii',
    'iso-8859-1',
]


# List of acceptable escape character
ACCEPTABLE_ESCAPE_CHARS = [
    b'\r',
    b'\n',
]


def parse_args():
    """Parse command line arguments"""
    argparser = argparse.ArgumentParser(allow_abbrev=False)

    argparser.add_argument("elffile", help="Zephyr ELF binary")
    argparser.add_argument("--build", help="Build ID")
    argparser.add_argument("--build-header",
                           help="Header file containing BUILD_VERSION define")
    argparser.add_argument("--debug", action="store_true",
                           help="Print extra debugging information")
    argparser.add_argument("-v", "--verbose", action="store_true",
                           help="Print more information")

    outfile_grp = argparser.add_mutually_exclusive_group(required=True)
    outfile_grp.add_argument("--json",
                             help="Output Dictionary Logging Database file in JSON")
    outfile_grp.add_argument("--syst",
                             help="Output MIPI Sys-T Collateral XML file")

    return argparser.parse_args()


def extract_elf_code_data_sections(elf, wildcards = None):
    """Find all sections in ELF file"""
    sections = {}

    for sect in elf.iter_sections():
        # Only Allocated sections with PROGBITS are included
        # since they actually have code/data.
        #
        # On contrary, BSS is allocated but NOBITS.
        if (((wildcards is not None) and (sect.name in wildcards)) or
            ((sect['sh_flags'] & SH_FLAGS.SHF_ALLOC) == SH_FLAGS.SHF_ALLOC
            and sect['sh_type'] == 'SHT_PROGBITS')
        ):
            sections[sect.name] = {
                    'name'    : sect.name,
                    'size'    : sect['sh_size'],
                    'start'   : sect['sh_addr'],
                    'end'     : sect['sh_addr'] + sect['sh_size'] - 1,
                    'data'    : sect.data(),
                }

    return sections


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


def parse_log_const_symbols(database, log_const_area, log_const_symbols, string_mappings):
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

    first_offset -= log_const_area['start']

    # find all log_const_*
    for sym in log_const_symbols:
        # Find data offset in log_const_area for this symbol
        offset = sym.entry['st_value'] - log_const_area['start']

        idx_s = offset
        idx_e = offset + datum_size

        datum = log_const_area['data'][idx_s:idx_e]

        if len(datum) != datum_size:
            # Not enough data to unpack
            continue

        str_ptr, level = struct.unpack(formatter, datum)

        # Offset to rodata section for string
        instance_name = find_string_in_mappings(string_mappings, str_ptr)
        if instance_name is None:
            instance_name = "unknown"

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


def extract_logging_subsys_information(elf, database, string_mappings):
    """
    Extract logging subsys related information and store in database.

    For example, this extracts the list of log instances to establish
    mapping from source ID to name.
    """
    # Extract log constant section for module names
    section_log_const = find_elf_sections(elf, "log_const_area")
    if section_log_const is None:
        # ESP32 puts "log_const_*" info log_static_section instead of log_const_areas
        section_log_const = find_elf_sections(elf, "log_static_section")

    if section_log_const is None:
        logger.error("Cannot find section 'log_const_areas' in ELF file, exiting...")
        sys.exit(1)

    # Find all "log_const_*" symbols and parse them
    log_const_symbols = find_log_const_symbols(elf)
    parse_log_const_symbols(database, section_log_const, log_const_symbols, string_mappings)


def is_die_attr_ref(attr):
    """
    Returns True if the DIE attribute is a reference.
    """
    return bool(attr.form in ('DW_FORM_ref1', 'DW_FORM_ref2',
                              'DW_FORM_ref4', 'DW_FORM_ref8',
                              'DW_FORM_ref'))


def find_die_var_base_type(compile_unit, die, is_const):
    """
    Finds the base type of a DIE and returns the name.
    If DW_AT_type is a reference, it will recursively go through
    the references to find the base type. Returns None is no
    base type is found.
    """
    # DIE is of base type. So extract the name.
    if die.tag == 'DW_TAG_base_type':
        return die.attributes['DW_AT_name'].value.decode('ascii'), is_const

    # Not a type, cannot continue
    if not 'DW_AT_type' in die.attributes:
        return None, None

    if die.tag == 'DW_TAG_const_type':
        is_const = True

    # DIE is probably a reference to another.
    # If so, check if the reference is a base type.
    type_attr = die.attributes['DW_AT_type']
    if is_die_attr_ref(type_attr):
        ref_addr = compile_unit.cu_offset + type_attr.raw_value
        ref_die = compile_unit.get_DIE_from_refaddr(ref_addr)

        return find_die_var_base_type(compile_unit, ref_die, is_const)

    # Not a base type, and not reference
    return None, None


def is_die_var_const_char(compile_unit, die):
    """
    Returns True if DIE of type variable is const char.
    """
    var_type, is_const = find_die_var_base_type(compile_unit, die, False)

    if var_type is not None and var_type.endswith('char') and is_const:
        return True

    return False


def extract_string_variables(elf):
    """
    Find all string variables (char) in all Compilation Units and
    Debug information Entry (DIE) in ELF file.
    """
    dwarf_info = elf.get_dwarf_info()
    loc_lists = dwarf_info.location_lists()
    loc_parser = LocationParser(loc_lists)

    strings = []

    # Loop through all Compilation Units and
    # Debug information Entry (DIE) to extract all string variables
    for compile_unit in dwarf_info.iter_CUs():
        for die in compile_unit.iter_DIEs():
            # Only care about variables with location information
            # and of type "char"
            if die.tag == 'DW_TAG_variable':
                if ('DW_AT_type' in die.attributes
                    and 'DW_AT_location' in die.attributes
                    and is_die_var_const_char(compile_unit, die)
                ):
                    # Extract location information, which is
                    # its address in memory.
                    loc_attr = die.attributes['DW_AT_location']
                    if loc_parser.attribute_has_location(loc_attr, die.cu['version']):
                        loc = loc_parser.parse_from_attribute(loc_attr, die.cu['version'])
                        if isinstance(loc, LocationExpr):
                            try:
                                addr = describe_DWARF_expr(loc.loc_expr,
                                                        dwarf_info.structs)

                                matcher = DT_LOCATION_REGEX.match(addr)
                                if matcher:
                                    addr = int(matcher.group(1), 16)
                                    if addr > 0:
                                        strings.append({
                                            'name': die.attributes['DW_AT_name'].value,
                                            'addr': addr,
                                            'die': die
                                        })
                            except KeyError:
                                pass

    return strings

def try_decode_string(str_maybe):
    """Check if it is a printable string"""
    for encoding in STR_ENCODINGS:
        try:
            return str_maybe.decode(encoding)
        except UnicodeDecodeError:
            pass

    return None

def is_printable(b):
    # Check if string is printable according to Python
    # since the parser (written in Python) will need to
    # print the string.
    #
    # Note that '\r' and '\n' are not included in
    # string.printable so they need to be checked separately.
    return (b in string.printable) or (b in ACCEPTABLE_ESCAPE_CHARS)

def extract_strings_in_one_section(section, str_mappings):
    """Extract NULL-terminated strings in one ELF section"""
    data = section['data']
    idx = 0
    start = None
    for x in data:
        if is_printable(chr(x)):
            # Printable character, potential part of string
            if start is None:
                # Beginning of potential string
                start = idx
        elif x == 0:
            # End of possible string
            if start is not None:
                # Found potential string
                str_maybe = data[start : idx]
                decoded_str = try_decode_string(str_maybe)

                if decoded_str is not None:
                    addr = section['start'] + start

                    if addr not in str_mappings:
                        str_mappings[addr] = decoded_str

                        # Decoded string may contain un-printable characters
                        # (e.g. extended ASC-II characters) or control
                        # characters (e.g. '\r' or '\n'), so simply print
                        # the byte string instead.
                        logger.debug('Found string via extraction at ' + PTR_FMT + ': %s',
                                     addr, str_maybe)

                        # GCC-based toolchain will reuse the NULL character
                        # for empty strings. There is no way to know which
                        # one is being reused, so just treat all NULL character
                        # at the end of legitimate strings as empty strings.
                        null_addr = section['start'] + idx
                        str_mappings[null_addr] = ''

                        logger.debug('Found null string via extraction at ' + PTR_FMT,
                                     null_addr)
                start = None
        else:
            # Non-printable byte, remove start location
            start = None
        idx += 1

    return str_mappings


def extract_static_strings(elf, database, section_extraction=False):
    """
    Extract static strings from ELF file using DWARF,
    and also extraction from binary data.
    """
    string_mappings = {}

    elf_sections = extract_elf_code_data_sections(elf, REMOVED_STRING_SECTIONS)

    # Extract strings using ELF DWARF information
    str_vars = extract_string_variables(elf)
    for str_var in str_vars:
        for _, sect in elf_sections.items():
            one_str = extract_one_string_in_section(sect, str_var['addr'])
            if one_str is not None:
                string_mappings[str_var['addr']] = one_str
                logger.debug('Found string variable at ' + PTR_FMT + ': %s',
                             str_var['addr'], one_str)
                break

    if section_extraction:
        # Extract strings from ELF sections
        string_sections = STATIC_STRING_SECTIONS
        rawstr_map = {}

        # Some architectures may put static strings into additional sections.
        # So need to extract them too.
        arch_data = dictionary_parser.log_database.ARCHS[database.get_arch()]
        if "extra_string_section" in arch_data:
            string_sections.extend(arch_data['extra_string_section'])

        for sect_name in string_sections:
            if sect_name in elf_sections:
                rawstr_map = extract_strings_in_one_section(elf_sections[sect_name],
                                                                rawstr_map)

        for one_str in rawstr_map:
            if one_str not in string_mappings:
                string_mappings[one_str] = rawstr_map[one_str]

    return string_mappings


def main():
    """Main function of database generator"""
    args = parse_args()

    # Setup logging
    logging.basicConfig(format=LOGGER_FORMAT, level=logging.WARNING)
    if args.debug:
        logger.setLevel(logging.DEBUG)
    elif args.verbose:
        logger.setLevel(logging.INFO)

    elffile = open(args.elffile, "rb")
    if not elffile:
        logger.error("ERROR: Cannot open ELF file: %s, exiting...", args.elffile)
        sys.exit(1)

    logger.info("ELF file %s", args.elffile)

    if args.json:
        logger.info("JSON Database file %s", args.json)
        section_extraction = True

    if args.syst:
        logger.info("MIPI Sys-T Collateral file %s", args.syst)
        section_extraction = False

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

    if database.is_tgt_64bit():
        global PTR_FMT
        PTR_FMT = '0x%016x'

    # Extract strings from ELF files
    string_mappings = extract_static_strings(elf, database, section_extraction)
    if len(string_mappings) > 0:
        database.set_string_mappings(string_mappings)
        logger.info("Found %d strings", len(string_mappings))

    # Extract information related to logging subsystem
    if not section_extraction:
        # The logging subsys information (e.g. log module names)
        # may require additional strings outside of those extracted
        # via ELF DWARF variables. So generate a new string mappings
        # with strings in various ELF sections.
        string_mappings = extract_static_strings(elf, database, section_extraction=True)

    extract_logging_subsys_information(elf, database, string_mappings)

    # Write database file
    if args.json:
        if not LogDatabase.write_json_database(args.json, database):
            logger.error("ERROR: Cannot open database file for write: %s, exiting...",
                         args.json)
            sys.exit(1)

    if args.syst:
        if not LogDatabase.write_syst_database(args.syst, database):
            logger.error("ERROR: Cannot open database file for write: %s, exiting...",
                         args.syst)
            sys.exit(1)

    elffile.close()


if __name__ == "__main__":
    main()
