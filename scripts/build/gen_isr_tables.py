#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
# Copyright (c) 2018 Foundries.io
# Copyright (c) 2023 Nordic Semiconductor NA
#
# SPDX-License-Identifier: Apache-2.0
#

import argparse
import importlib
import json
import os
import re
import struct
import sys

from elftools.elf.constants import SH_FLAGS
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection


class gen_isr_log:
    def __init__(self, debug=False):
        self.__debug = debug

    def debug(self, text):
        """Print debug message if debugging is enabled.

        Note - this function requires config global variable to be initialized.
        """
        if self.__debug:
            sys.stdout.write(os.path.basename(sys.argv[0]) + ": " + text + "\n")

    @staticmethod
    def error(text):
        sys.exit(os.path.basename(sys.argv[0]) + ": error: " + text + "\n")

    def set_debug(self, state):
        self.__debug = state


log = gen_isr_log()


class gen_isr_config:
    """All the constants and configuration gathered in single class for readability."""

    # Constants
    __ISR_FLAG_DIRECT = 1 << 0
    __swt_spurious_handler = "z_irq_spurious"
    __swt_shared_handler = "z_shared_isr"
    __vt_spurious_handler = "z_irq_spurious"
    __vt_irq_handler = "_isr_wrapper"
    __shared_array_name = "z_shared_sw_isr_table"
    __sw_isr_array_name = "_sw_isr_table"
    __irq_vector_array_name = "_irq_vector_table"

    @staticmethod
    def __bm(bits):
        return (1 << bits) - 1

    def __init__(self, args, syms, log):
        """Initialize the configuration object.

        The configuration object initialization takes only arguments as a parameter.
        This is done to allow debug function work as soon as possible.
        """
        # Store the arguments required for work
        self.__args = args
        self.__syms = syms
        self.__log = log

        # Select the default interrupt vector handler
        if self.args.sw_isr_table:
            self.__vt_default_handler = self.__vt_irq_handler
        else:
            self.__vt_default_handler = self.__vt_spurious_handler
        # Calculate interrupt bits
        self.__int_bits = [8, 8, 8]
        # The below few hardware independent magic numbers represent various
        # levels of interrupts in a multi-level interrupt system.
        # 0x000000FF - represents the 1st level (i.e. the interrupts
        #              that directly go to the processor).
        # 0x0000FF00 - represents the 2nd level (i.e. the interrupts funnel
        #              into 1 line which then goes into the 1st level)
        # 0x00FF0000 - represents the 3rd level (i.e. the interrupts funnel
        #              into 1 line which then goes into the 2nd level)
        self.__int_lvl_masks = [0x000000FF, 0x0000FF00, 0x00FF0000]

        self.__irq2_baseoffset = None
        self.__irq3_baseoffset = None
        self.__irq2_offsets = None
        self.__irq3_offsets = None

        if self.check_multi_level_interrupts():
            self.__max_irq_per = self.get_sym("CONFIG_MAX_IRQ_PER_AGGREGATOR")

            self.__int_bits[0] = self.get_sym("CONFIG_1ST_LEVEL_INTERRUPT_BITS")
            self.__int_bits[1] = self.get_sym("CONFIG_2ND_LEVEL_INTERRUPT_BITS")
            self.__int_bits[2] = self.get_sym("CONFIG_3RD_LEVEL_INTERRUPT_BITS")

            if sum(self.int_bits) > 32:
                raise ValueError("Too many interrupt bits")

            self.__int_lvl_masks[0] = self.__bm(self.int_bits[0])
            self.__int_lvl_masks[1] = self.__bm(self.int_bits[1]) << self.int_bits[0]
            self.__int_lvl_masks[2] = self.__bm(self.int_bits[2]) << (
                self.int_bits[0] + self.int_bits[1]
            )

            self.__log.debug("Level    Bits        Bitmask")
            self.__log.debug("----------------------------")
            for i in range(3):
                bitmask_str = "0x" + format(self.__int_lvl_masks[i], '08X')
                self.__log.debug(f"{i + 1:>5} {self.__int_bits[i]:>7} {bitmask_str:>14}")

            if self.check_sym("CONFIG_2ND_LEVEL_INTERRUPTS"):
                num_aggregators = self.get_sym("CONFIG_NUM_2ND_LEVEL_AGGREGATORS")
                self.__irq2_baseoffset = self.get_sym("CONFIG_2ND_LVL_ISR_TBL_OFFSET")
                self.__irq2_offsets = [
                    self.get_sym(f'CONFIG_2ND_LVL_INTR_{str(i).zfill(2)}_OFFSET')
                    for i in range(num_aggregators)
                ]

                self.__log.debug(f'2nd level offsets: {self.__irq2_offsets}')

                if self.check_sym("CONFIG_3RD_LEVEL_INTERRUPTS"):
                    num_aggregators = self.get_sym("CONFIG_NUM_3RD_LEVEL_AGGREGATORS")
                    self.__irq3_baseoffset = self.get_sym("CONFIG_3RD_LVL_ISR_TBL_OFFSET")
                    self.__irq3_offsets = [
                        self.get_sym(f'CONFIG_3RD_LVL_INTR_{str(i).zfill(2)}_OFFSET')
                        for i in range(num_aggregators)
                    ]

                    self.__log.debug(f'3rd level offsets: {self.__irq3_offsets}')

    @property
    def args(self):
        return self.__args

    @property
    def swt_spurious_handler(self):
        return self.__swt_spurious_handler

    @property
    def swt_shared_handler(self):
        return self.__swt_shared_handler

    @property
    def vt_default_handler(self):
        return self.__vt_default_handler

    @property
    def shared_array_name(self):
        return self.__shared_array_name

    @property
    def sw_isr_array_name(self):
        return self.__sw_isr_array_name

    @property
    def irq_vector_array_name(self):
        return self.__irq_vector_array_name

    @property
    def int_bits(self):
        return self.__int_bits

    @property
    def int_lvl_masks(self):
        return self.__int_lvl_masks

    def endian_prefix(self):
        if self.args.big_endian:
            return ">"
        else:
            return "<"

    def get_irq_baseoffset(self, lvl):
        if lvl == 2:
            return self.__irq2_baseoffset
        if lvl == 3:
            return self.__irq3_baseoffset
        self.__log.error(f"Unsupported irq level: {lvl}")

    def get_irq_index(self, irq, lvl):
        if lvl == 2:
            offsets = self.__irq2_offsets
        elif lvl == 3:
            offsets = self.__irq3_offsets
        else:
            self.__log.error(f"Unsupported irq level: {lvl}")
        try:
            return offsets.index(irq)
        except ValueError:
            self.__log.error(
                f"IRQ {irq} not present in parent offsets ({offsets}). "
                + " Recheck interrupt configuration."
            )

    def get_swt_table_index(self, offset, irq):
        if not self.check_multi_level_interrupts():
            return irq - offset
        # Calculate index for multi level interrupts
        self.__log.debug('IRQ = ' + hex(irq))
        irq3 = (irq & self.int_lvl_masks[2]) >> (self.int_bits[0] + self.int_bits[1])
        irq2 = (irq & self.int_lvl_masks[1]) >> (self.int_bits[0])
        irq1 = irq & self.int_lvl_masks[0]
        # Figure out third level interrupt position
        if irq3:
            list_index = self.get_irq_index(irq2 - 1, 3)
            irq3_pos = self.get_irq_baseoffset(3) + self.__max_irq_per * list_index + irq3 - 1
            self.__log.debug('IRQ_level = 3')
            self.__log.debug('IRQ_Indx = ' + str(irq3))
            self.__log.debug('IRQ_Pos  = ' + str(irq3_pos))
            return irq3_pos - offset
        # Figure out second level interrupt position
        if irq2:
            list_index = self.get_irq_index(irq1, 2)
            irq2_pos = self.get_irq_baseoffset(2) + self.__max_irq_per * list_index + irq2 - 1
            self.__log.debug('IRQ_level = 2')
            self.__log.debug('IRQ_Indx = ' + str(irq2))
            self.__log.debug('IRQ_Pos  = ' + str(irq2_pos))
            return irq2_pos - offset
        # Figure out first level interrupt position
        self.__log.debug('IRQ_level = 1')
        self.__log.debug('IRQ_Indx = ' + str(irq1))
        self.__log.debug('IRQ_Pos  = ' + str(irq1))
        return irq1 - offset

    def get_intlist_snames(self):
        return self.args.intlist_section

    def test_isr_direct(self, flags):
        return flags & self.__ISR_FLAG_DIRECT

    def get_sym_from_addr(self, addr):
        for key, value in self.__syms.items():
            if addr == value:
                return key
        return None

    def get_sym(self, name):
        value = self.__syms.get(name)
        if value is None:
            value = self.__syms.get(getattr(self, "c_symbol_prefix", "") + name)
        return value

    def check_sym(self, name):
        return name in self.__syms

    def check_multi_level_interrupts(self):
        return self.check_sym("CONFIG_MULTI_LEVEL_INTERRUPTS")

    def check_shared_interrupts(self):
        return self.check_sym("CONFIG_SHARED_INTERRUPTS")

    def check_64b(self):
        return self.check_sym("CONFIG_64BIT")


def get_symbols(obj):
    for section in obj.iter_sections():
        if isinstance(section, SymbolTableSection):
            return {sym.name: sym.entry.st_value for sym in section.iter_symbols()}

    log.error("Could not find symbol table")


def get_metadata_header(obj, section_names):
    """Locate the actual header object, including separate no-alloc headers."""
    sections = {
        obj.get_section_index(section.name): section
        for section in obj.iter_sections()
        if section.name in (*section_names, ".irq_info")
    }
    if not sections:
        raise ValueError("Cannot locate ISR metadata to identify C symbol names")
    headers = []
    for symbols in obj.iter_sections():
        if not isinstance(symbols, SymbolTableSection):
            continue
        for symbol in symbols.iter_symbols():
            index = symbol["st_shndx"]
            if (
                symbol.name.endswith("_iheader")
                and index in sections
                and symbol["st_value"] == sections[index]["sh_addr"]
                and symbol["st_info"]["type"] == "STT_OBJECT"
            ):
                headers.append((sections[index], symbol, symbol.name[: -len("_iheader")]))
    if len(headers) != 1 or re.fullmatch(r"[A-Za-z_.$]*", headers[0][2]) is None:
        raise ValueError("Cannot uniquely identify the ISR metadata C symbol prefix")
    return headers[0]


def get_c_symbol_prefix(obj, section_names):
    """Identify the compiler's C label decoration from the metadata object."""
    return get_metadata_header(obj, section_names)[2]


def read_local_metadata(obj, section_names):
    """Read merged or separate header/record sections and verify their storage."""
    header_section, header_symbol, prefix = get_metadata_header(obj, section_names)
    header = header_section.data()
    if len(header) < 8:
        raise ValueError("Truncated ISR metadata header")
    _, header_size = struct.unpack_from(("<" if obj.little_endian else ">") + "II", header)
    if header_symbol["st_size"] != header_size or not 8 <= header_size <= len(header):
        raise ValueError("ISR metadata header object size does not match its header")
    records = [section for section in obj.iter_sections() if section.name in section_names]
    if header_section.name in section_names:
        if len(records) != 1:
            raise ValueError("Ambiguous merged ISR metadata sections")
        data = header
    else:
        if len(header) != header_size:
            raise ValueError("Separate ISR metadata header contains trailing data")
        data = header + b"".join(section.data() for section in records)
    markers = [
        symbol
        for section in obj.iter_sections()
        if isinstance(section, SymbolTableSection)
        for symbol in section.iter_symbols()
        if symbol.name in {"z_isr_metadata_nonallocated", prefix + "z_isr_metadata_nonallocated"}
        and symbol["st_shndx"] != "SHN_UNDEF"
    ]
    nonallocated = bool(markers)
    if nonallocated:
        if len(markers) != 1 or markers[0]["st_shndx"] != "SHN_ABS" or markers[0]["st_value"] != 1:
            raise ValueError("Invalid ISR nonallocated-metadata capability")
        for section in [header_section, *records]:
            if section["sh_flags"] & (SH_FLAGS.SHF_ALLOC | SH_FLAGS.SHF_EXECINSTR):
                raise ValueError(f"ISR metadata {section.name} is allocated or executable")
            begin, size = section["sh_offset"], section["sh_size"]
            if any(
                segment["p_type"] == "PT_LOAD"
                and (
                    segment.section_in_segment(section)
                    or (
                        size > 0
                        and begin < segment["p_offset"] + segment["p_filesz"]
                        and segment["p_offset"] < begin + size
                    )
                )
                for segment in obj.iter_segments()
            ):
                raise ValueError(f"ISR metadata {section.name} overlaps a load segment")
    return data, prefix, nonallocated


def read_intList_sect(elfobj, snames):
    """
    Load the raw intList section data in a form of byte array.
    """
    intList_sect = None

    for sname in snames:
        intList_sect = elfobj.get_section_by_name(sname)
        if intList_sect is not None:
            log.debug(f"Found intlist section: \"{sname}\"")
            break

    if intList_sect is None:
        log.error("Cannot find the intlist section!")

    intdata = intList_sect.data()

    return intdata


def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument(
        "-e",
        "--big-endian",
        action="store_true",
        help="Target encodes data in big-endian format (little endian is the default)",
    )
    parser.add_argument(
        "-d", "--debug", action="store_true", help="Print additional debugging information"
    )
    parser.add_argument("-o", "--output-source", required=True, help="Output source file")
    parser.add_argument(
        "-l",
        "--linker-output-files",
        nargs=2,
        metavar=("vector_table_link", "software_interrupt_link"),
        help="Output linker files. "
        "Used only if CONFIG_ISR_TABLES_LOCAL_DECLARATION is enabled. "
        "In other case empty file would be generated.",
    )
    parser.add_argument("-k", "--kernel", required=True, help="Zephyr kernel image")
    parser.add_argument("--shared-linker-output", help="Shared ISR table placement fragment")
    parser.add_argument("--entries-linker-output", help="Sparse switch-entry placement fragment")
    parser.add_argument("--cmake-output", help="CMake linker placement records")
    parser.add_argument("--layout-output", help="ISR layout manifest for final ELF verification")
    parser.add_argument("-s", "--sw-isr-table", action="store_true", help="Generate SW ISR table")
    parser.add_argument("-V", "--vector-table", action="store_true", help="Generate vector table")
    parser.add_argument(
        "-i",
        "--intlist-section",
        action="append",
        required=True,
        help="The name of the section to search for the interrupt data. "
        "This is accumulative argument. The first section found would be used.",
    )

    return parser.parse_args()


def get_manual_vector_tables(obj, symbol_prefix):
    """Record C vector objects supplied by the SoC instead of the generator."""
    name = symbol_prefix + "_irq_vector_table"
    symbols = [
        symbol
        for section in obj.iter_sections()
        if isinstance(section, SymbolTableSection)
        for symbol in section.iter_symbols()
        if symbol.name == name and symbol["st_shndx"] != "SHN_UNDEF"
    ]
    if not symbols:
        return []
    if len(symbols) != 1:
        raise ValueError(f"Cannot uniquely identify manual vector table {name}")
    symbol = symbols[0]
    # Assembly vectors can have architecture-specific instruction formats and
    # zero-size NOTYPE aliases. Do not infer a C object extent for those aliases.
    if symbol["st_info"]["type"] != "STT_OBJECT" or symbol["st_size"] == 0:
        return []
    if not isinstance(symbol["st_shndx"], int):
        raise ValueError(f"Manual vector table {name} has no output section")
    section = obj.get_section(symbol["st_shndx"])
    if not section["sh_flags"] & SH_FLAGS.SHF_ALLOC:
        raise ValueError(f"Manual vector table {name} is not allocated")
    return [
        {
            "symbol": name,
            "size": symbol["st_size"],
            "alignment": min(obj.elfclass // 8, section["sh_addralign"] or 1),
            "writable": bool(section["sh_flags"] & SH_FLAGS.SHF_WRITE),
        }
    ]


def main():
    args = parse_args()
    # Configure logging as soon as possible
    log.set_debug(args.debug)

    manual_vectors = []
    with open(args.kernel, "rb") as fp:
        kernel = ELFFile(fp)
        args.big_endian = not kernel.little_endian
        config = gen_isr_config(args, get_symbols(kernel), log)

        if config.check_sym("CONFIG_ISR_TABLES_LOCAL_DECLARATION"):
            intlist_data, config.c_symbol_prefix, config.metadata_nonallocated = (
                read_local_metadata(kernel, config.get_intlist_snames())
            )
            parser_module = importlib.import_module('gen_isr_tables_parser_local')
            parser = parser_module.gen_isr_parser(intlist_data, config, log)
            if not args.vector_table:
                manual_vectors = get_manual_vector_tables(kernel, config.c_symbol_prefix)
            if (
                config.check_sym("CONFIG_CPU_CORTEX_M")
                and args.vector_table
                and config.check_sym("CONFIG_CMAKE_LINKER_GENERATOR")
            ):
                vector_address = config.get_sym("_irq_vector_table")
                if vector_address is None:
                    log.error("Cannot locate the Cortex-M IRQ vector output section")
                rom_start = config.get_sym("__rom_start_start")
                if rom_start is None:
                    log.error("Cannot locate the Cortex-M .rom_start base symbol")
                vector_sections = [
                    section
                    for section in kernel.iter_sections()
                    if section["sh_flags"] & SH_FLAGS.SHF_ALLOC
                    and section["sh_addr"]
                    <= vector_address
                    < section["sh_addr"] + section["sh_size"]
                ]
                if len(vector_sections) != 1:
                    log.error("Cannot identify the Cortex-M IRQ vector output section")
                vector_section = vector_sections[0]
                vector_offset = vector_address - rom_start
                vector_size = parser.count * parser.header["vector_size"]
                if (
                    vector_offset < 0
                    or vector_address + vector_size
                    > vector_section["sh_addr"] + vector_section["sh_size"]
                ):
                    log.error("Cortex-M IRQ vectors are outside .rom_start")
                parser.vector_cmake_placement = (".rom_start", vector_offset)
        else:
            intlist_data = read_intList_sect(kernel, config.get_intlist_snames())
            parser_module = importlib.import_module('gen_isr_tables_parser_carrays')
            parser = parser_module.gen_isr_parser(intlist_data, config, log)

    with open(args.output_source, "w") as fp:
        parser.write_source(fp)

    if args.linker_output_files is not None:
        with (
            open(args.linker_output_files[0], "w") as fp_vt,
            open(args.linker_output_files[1], "w") as fp_swi,
        ):
            if hasattr(parser, 'write_linker_vt'):
                parser.write_linker_vt(fp_vt)
            else:
                log.debug("Chosen parser does not support vector table linker file")
                fp_vt.write('/* Empty */\n')
            if hasattr(parser, 'write_linker_swi'):
                parser.write_linker_swi(fp_swi)
            else:
                log.debug("Chosen parser does not support software interrupt linker file")
                fp_swi.write('/* Empty */\n')

    for filename, method in (
        (args.shared_linker_output, "write_linker_shared"),
        (args.entries_linker_output, "write_linker_entries"),
        (args.cmake_output, "write_cmake"),
    ):
        if filename is not None:
            with open(filename, "w") as stream:
                if hasattr(parser, method):
                    getattr(parser, method)(stream)

    if args.layout_output is not None:
        with open(args.layout_output, "w") as stream:
            manifest = parser.layout_manifest()
            manifest["manual_vectors"] = manual_vectors
            json.dump(manifest, stream, indent=2)
            stream.write("\n")


if __name__ == "__main__":
    main()
