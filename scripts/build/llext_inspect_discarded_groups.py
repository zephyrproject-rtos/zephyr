#!/usr/bin/env python3
#
# Copyright (c) 2025 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

"""
Inspects Zephyr build artifacts (ELF, plus Kconfig if available)
to display information about symbols placed in the discard exports
table due to their symbol group not being enabled.
"""

import argparse
import logging
import re
import sys
from pathlib import Path

import llext_elf_toolkit
from elftools.elf.elffile import ELFFile

#!!!!! WARNING !!!!!
#
# These constants MUST be kept in sync with linker script
# 'include/zephyr/linker/llext-sections.ld' and various
# definitions in 'include/subsys/llext/llext.h'.
#
#!!!!! WARNING !!!!!

LLEXT_DISCARDED_EXPORTS_TABLE_SECTION_NAME = "llext_discarded_exports_table"
LLEXT_DISCARDED_EXPORTS_STRTAB_SECTION_NAME = "llext_discarded_exports_strtab"
STRUCT_Z_LLEXT_DISCARDED_CONST_SYMBOL_DESC = "PP"  # two pointers
GROUP_EXPORT_KCONFIG_PREFIX = "CONFIG_LLEXT_EXPORT_SYMBOL_GROUP_"


REPORT_TOP_LEVEL_NOTICE = """\
This file lists all symbols that have been marked as LLEXT exports
but will not be added to the export table because their symbol group
has not been enabled. Refer to LLEXT Documentation for more details.
"""


class Export:
    def __init__(self, tpl, read_str):
        # [0] = name (pseudo pointer)
        # [1] = group name (pseudo pointer)
        self.name = read_str(tpl[0])
        self.group = read_str(tpl[1])

    def __lt__(self, other):  # Sort by symbol name
        return self.name < other.name


def parse_dotconfig(dotconfig_path: Path):
    """
    Parses options from `.config` file at location 'dotconfig_path'.

    Note: tri-state `"m"` is transformed into integer `2`.
    """
    # Options should receive value 'n' if a line
    # containing 'CONFIG_xxx is not set' is present
    UNSET_OPTION_REGEXP = re.compile(r"# (CONFIG_.+) is not set")

    options: dict[str, str | int | bool] = {}

    with dotconfig_path.open("r") as fd:
        for line in fd:
            line = line.strip()
            if len(line) <= 1:
                continue

            # Handle comment lines
            if line[0] == '#':
                if match := UNSET_OPTION_REGEXP.match(line):
                    options[match.group(1)] = False
                continue

            key, value = line.split("=")

            if value == "y":
                value = True
            elif value == "n":
                value = False
            elif value == "m":
                value = 2
            elif value[0] == value[-1] == '"':
                value = value.strip('"')
            else:
                value = int(value, base=0)

            options[key] = value

    return options


def _init_log(verbose: int):
    """Initialize a logger object."""
    log = logging.getLogger(__file__)

    console = logging.StreamHandler()
    console.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
    log.addHandler(console)

    if verbose > 1:
        log.setLevel(logging.DEBUG)
    elif verbose > 0:
        log.setLevel(logging.INFO)
    else:
        log.setLevel(logging.WARNING)

    return log


def _parse_args(argv):
    """Parse the command line arguments."""
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument("-f", "--elf-file", type=Path, help="ELF file to process")
    parser.add_argument("--output-report", type=Path, help="Output report file (plaintext)")
    parser.add_argument(
        "--dotconfig-file",
        type=Path,
        required=False,
        help=".config file of build - used to check for missing Kconfig options",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="count",
        help=("enable verbose output, can be used multiple times to increase verbosity level"),
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = _parse_args(argv)

    log = _init_log(args.verbose or 0)

    log.info(f"{__file__}: {args.elf_file}")

    if args.dotconfig_file:
        kconfig_options = parse_dotconfig(args.dotconfig_file)
    else:
        kconfig_options = None

    with open(args.elf_file, 'rb') as elf_fd:
        elf = ELFFile(elf_fd)
        ptr_size = elf.elfclass // 8

        sym_desc = llext_elf_toolkit.get_target_specific_structure(
            STRUCT_Z_LLEXT_DISCARDED_CONST_SYMBOL_DESC,
            ptr_size,
            endianness='little' if elf.little_endian else 'big',
        )

        # Find discarded exports table and strtab
        try:
            exptab_section = llext_elf_toolkit.SectionDescriptor(
                elf, LLEXT_DISCARDED_EXPORTS_TABLE_SECTION_NAME
            )
            strtab_section = llext_elf_toolkit.SectionDescriptor(
                elf, LLEXT_DISCARDED_EXPORTS_STRTAB_SECTION_NAME
            )

            # Create helper to read strings from strtab
            def read_str(ptr):
                # Implementation detail: in linker script, we force
                # section base to 0x0; no need to subtract it here.
                raw_str = b''

                elf_fd.seek(strtab_section.offset + ptr)

                c = elf_fd.read(1)
                while c != b'\0':
                    raw_str += c
                    c = elf_fd.read(1)

                return raw_str.decode("utf-8")

            # Parse discarded exports table
            if (exptab_section.size % sym_desc.size) != 0:
                log.error(
                    "Unexpected discarded exptab section size %u (not multiple of entry size=%u)",
                    exptab_section.size,
                    sym_desc.size,
                )
                return 1

            # Separate based on group
            discarded_exports: dict[str, list[Export]] = {}

            num_discarded_exports = exptab_section.size // sym_desc.size
            for i in range(num_discarded_exports):
                elf_fd.seek(exptab_section.offset + i * sym_desc.size)
                raw_entry = elf_fd.read(sym_desc.size)

                export = Export(sym_desc.unpack(raw_entry), read_str)

                group_exports = discarded_exports.get(export.group, [])
                group_exports.append(export)
                discarded_exports[export.group] = group_exports
        except KeyError:
            log.info("Discarded exports table or strtab section not found")
            discarded_exports = {}

    # Sort symbol groups by name
    discarded_exports = dict(sorted(discarded_exports.items()))

    # Inspect all groups and print information to output
    with open(args.output_report, 'w') as out_fd:
        out_fd.write(REPORT_TOP_LEVEL_NOTICE + "\n")

        exps = discarded_exports.items()
        if len(exps) == 0:
            out_fd.write("<No discarded symbols found in image>")

        for group_name, exports in exps:
            exports = sorted(exports)

            # Write group header
            out_fd.write("=" * 100 + "\n" + f"{'Group ' + group_name:^100}\n")

            if group_name.upper() != group_name:
                log.warning("Name of LLEXT symbol group '%s' is not uppercase", group_name)
                out_fd.write("Group name should be uppercase!\n")
            elif kconfig_options:
                # Verify that Kconfig option for group exists if we have
                # .config, but only if the group's name is in uppercase
                kcfg_sym = GROUP_EXPORT_KCONFIG_PREFIX + group_name
                if kconfig_options.get(kcfg_sym) is None:
                    log.warning("Could not find Kconfig option %s", kcfg_sym)
                    out_fd.write(
                        f'{"Could not find Kconfig option " + kcfg_sym + ":":^100}\n'
                        f'{"symbol group will never be included in the LLEXT export table!":^100}\n'
                    )

            out_fd.write("=" * 100 + "\n")

            # Write symbols listing
            out_fd.write("Symbol Name\n")
            out_fd.write("-" * 80 + "\n")
            for e in exports:
                out_fd.write(e.name + "\n")

            out_fd.write("\n")

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
