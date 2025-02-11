#!/usr/bin/env python3
#
# Copyright (c) 2024 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

"""Injects SLIDs in LLEXT ELFs' symbol tables.

When Kconfig option CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID is enabled,
all imports from the Zephyr kernel & application are resolved using
SLIDs instead of symbol names. This script stores the SLID of all
imported symbols in their associated entry in the ELF symbol table
to allow the LLEXT subsystem to link it properly at runtime.

Note that this script is idempotent in theory. However, to prevent
any catastrophic problem, the script will abort if the 'st_value'
field of the `ElfX_Sym` structure is found to be non-zero, which is
the case after one invocation. For this reason, in practice, the script
cannot actually be executed twice on the same ELF file.
"""

import argparse
import logging
import shutil
import sys

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

import llext_slidlib

class LLEXTSymtabPreparator():
    def __init__(self, elf_path, log):
        self.log = log
        self.elf_path = elf_path
        self.elf_fd = open(elf_path, "rb+")
        self.elf = ELFFile(self.elf_fd)

    def _find_symtab(self):
        supported_symtab_sections = [
            ".symtab",
            ".dynsym",
        ]

        symtab = None
        for section_name in supported_symtab_sections:
            symtab = self.elf.get_section_by_name(section_name)
            if not isinstance(symtab, SymbolTableSection):
                self.log.debug(f"section {section_name} not found.")
            else:
                self.log.info(f"processing '{section_name}' symbol table...")
                self.log.debug(f"(symbol table is at file offset 0x{symtab['sh_offset']:X})")
                break
        return symtab

    def _find_imports_in_symtab(self, symtab):
        i = 0
        imports = []
        for sym in symtab.iter_symbols():
            #Check if symbol is an import
            if sym.entry['st_info']['type'] == 'STT_NOTYPE' and \
                sym.entry['st_info']['bind'] == 'STB_GLOBAL' and \
                sym.entry['st_shndx'] == 'SHN_UNDEF':

                self.log.debug(f"found imported symbol '{sym.name}' at index {i}")
                imports.append((i, sym))

            i += 1
        return imports

    def _prepare_inner(self):
        #1) Locate the symbol table
        symtab = self._find_symtab()
        if symtab is None:
            self.log.error("no symbol table found in file")
            return 1

        #2) Find imported symbols in symbol table
        imports = self._find_imports_in_symtab(symtab)
        self.log.info(f"LLEXT has {len(imports)} import(s)")

        #3) Write SLIDs in each symbol's 'st_value' field
        def make_stvalue_reader_writer():
            byteorder = "little" if self.elf.little_endian else "big"
            if self.elf.elfclass == 32:
                sizeof_Elf_Sym = 0x10    #sizeof(Elf32_Sym)
                offsetof_st_value = 0x4  #offsetof(Elf32_Sym, st_value)
                sizeof_st_value = 0x4    #sizeof(Elf32_Sym.st_value)
            else:
                sizeof_Elf_Sym = 0x18
                offsetof_st_value = 0x8
                sizeof_st_value = 0x8

            def seek(symidx):
                self.elf_fd.seek(
                    symtab['sh_offset'] +
                    symidx * sizeof_Elf_Sym +
                    offsetof_st_value)

            def reader(symbol_index):
                seek(symbol_index)
                return int.from_bytes(self.elf_fd.read(sizeof_st_value), byteorder)

            def writer(symbol_index, st_value):
                seek(symbol_index)
                self.elf_fd.write(int.to_bytes(st_value, sizeof_st_value, byteorder))

            return reader, writer

        rd_st_val, wr_st_val = make_stvalue_reader_writer()
        slid_size = self.elf.elfclass // 8

        for (index, symbol) in imports:
            slid = llext_slidlib.generate_slid(symbol.name, slid_size)
            slid_as_str = llext_slidlib.format_slid(slid, slid_size)
            msg = f"{symbol.name} -> {slid_as_str}"

            self.log.info(msg)

            # Make sure we're not overwriting something actually important
            original_st_value = rd_st_val(index)
            if original_st_value != 0:
                self.log.error(f"unexpected non-zero st_value for symbol {symbol.name}")
                return 1

            wr_st_val(index, slid)

        return 0

    def prepare_llext(self):
        res = self._prepare_inner()
        self.elf_fd.close()
        return res

# Disable duplicate code warning for the code that follows,
# as it is expected for these functions to be similar.
# pylint: disable=duplicate-code
def _parse_args(argv):
    """Parse the command line arguments."""
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)

    parser.add_argument("-f", "--elf-file", required=True,
                        help="LLEXT ELF file to process")
    parser.add_argument("-o", "--output-file",
                        help=("Additional output file where processed ELF "
                        "will be copied"))
    parser.add_argument("-sl", "--slid-listing",
                        help="write the SLID listing to a file")
    parser.add_argument("-v", "--verbose", action="count",
                        help=("enable verbose output, can be used multiple times "
                              "to increase verbosity level"))
    parser.add_argument("--always-succeed", action="store_true",
                        help="always exit with a return code of 0, used for testing")

    return parser.parse_args(argv)

def _init_log(verbose):
    """Initialize a logger object."""
    log = logging.getLogger(__file__)

    console = logging.StreamHandler()
    console.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
    log.addHandler(console)

    if verbose and verbose > 1:
        log.setLevel(logging.DEBUG)
    elif verbose and verbose > 0:
        log.setLevel(logging.INFO)
    else:
        log.setLevel(logging.WARNING)

    return log

def main(argv=None):
    args = _parse_args(argv)

    log = _init_log(args.verbose)

    log.info(f"inject_slids_in_llext: {args.elf_file}")

    preparator = LLEXTSymtabPreparator(args.elf_file, log)

    res = preparator.prepare_llext()

    if args.always_succeed:
        return 0

    if res == 0 and args.output_file:
        shutil.copy(args.elf_file, args.output_file)

    return res

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
