#!/usr/bin/env python3
#
# Copyright (c) 2025 Renesas Electronics Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""Generate a Renesas Partition Device File (RPD) for RA family

RFP is used by Renesas Flash Programmer or Renesas Partition Manager.
"""

import argparse
import os
import sys

from elftools.elf.elffile import ELFFile


def debug(text):
    """Display debug message if --verbose"""
    if args.verbose:
        sys.stdout.write(os.path.basename(sys.argv[0]) + ": " + text + "\n")


def error(text):
    """Display error message"""
    sys.stderr.write(os.path.basename(sys.argv[0]) + ": " + text + "\n")


def exit(text):
    """Exit program with an error message"""
    sys.exit(os.path.basename(sys.argv[0]) + ": " + text + "\n")


class ZephyrElf:
    """
    Represents memory symbols in an elf file.
    """

    def __init__(self):
        """
        Initialize the IDAU object with given parameters
        These values refer to HM under security section
        """
        self.ram_s_start = 0
        self.ram_s_size = 0
        self.ram_c_start = 0
        self.ram_c_size = 0
        self.flash_s_start = 0
        self.flash_s_size = 0
        self.flash_c_start = 0
        self.flash_c_size = 0
        self.data_flash_s_start = 0
        self.data_flash_s_size = 0

    def read_symbol(self, symbols, name):
        """Reads a symbols from symbol table and returns its address"""
        try:
            sym = symbols.get_symbol_by_name(name)
            address = sym[0]["st_value"]
        except Exception as e:
            error(f"Could not find symbol {name} in ELF file {e}")
        debug(f"Found symbol {name} at 0x{address:x}")
        return address

    def parse(self, path):
        """Reads IDAU configuration from elf file"""
        if not os.path.exists(path):
            exit("Invalid kernel path")

        with open(path, "rb") as f:
            elf = ELFFile(f)
            try:
                symtab = elf.get_section_by_name(".symtab")
                # Read memory symbols
                code_flash_s = self.read_symbol(symtab, "__FLASH_S")
                code_flash_nsc = self.read_symbol(symtab, "__FLASH_NSC")
                code_flash_ns = self.read_symbol(symtab, "__FLASH_NS")
                data_flash_s = self.read_symbol(symtab, "__DATA_FLASH_S")
                data_flash_ns = self.read_symbol(symtab, "__DATA_FLASH_NS")
                sram_s = self.read_symbol(symtab, "__RAM_S")
                sram_nsc = self.read_symbol(symtab, "__RAM_NSC")
                sram_ns = self.read_symbol(symtab, "__RAM_NS")

                self.ram_s_start = sram_s
                self.ram_s_size = sram_nsc - sram_s
                self.ram_c_start = sram_nsc
                self.ram_c_size = sram_ns - sram_nsc

                self.flash_s_start = code_flash_s
                self.flash_s_size = code_flash_nsc - code_flash_s
                self.flash_c_start = code_flash_nsc
                self.flash_c_size = code_flash_ns - code_flash_nsc

                self.data_flash_s_start = data_flash_s
                self.data_flash_s_size = data_flash_ns - data_flash_s

            except Exception as e:
                exit(f"Could not find symbol table in ELF file {e}")


def parse_args():
    """Parse command line arguments"""
    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument("-k", "--kernel", required=True, help="Zephyr kernel image")
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Print extra debugging information"
    )
    parser.add_argument("-o", "--output-rpd", required=True, help="output RPD file")

    args = parser.parse_args()


def main():
    parse_args()

    # Read boundary setting from elf file
    elf_parser = ZephyrElf()
    elf_parser.parse(args.kernel)

    with open(args.output_rpd, "w") as rpd:
        rpd.write(f"RAM_S_START=0x{elf_parser.ram_s_start:X}\n")
        rpd.write(f"RAM_S_SIZE=0x{elf_parser.ram_s_size:X}\n")
        rpd.write(f"RAM_C_START=0x{elf_parser.ram_c_start:X}\n")
        rpd.write(f"RAM_C_SIZE=0x{elf_parser.ram_c_size:X}\n")

        rpd.write(f"FLASH_S_START=0x{elf_parser.flash_s_start:X}\n")
        rpd.write(f"FLASH_S_SIZE=0x{elf_parser.flash_s_size:X}\n")
        rpd.write(f"FLASH_C_START=0x{elf_parser.flash_c_start:X}\n")
        rpd.write(f"FLASH_C_SIZE=0x{elf_parser.flash_c_size:X}\n")

        rpd.write(f"DATA_FLASH_S_START=0x{elf_parser.data_flash_s_start:X}\n")
        rpd.write(f"DATA_FLASH_S_SIZE=0x{elf_parser.data_flash_s_size:X}\n")


if __name__ == "__main__":
    main()
