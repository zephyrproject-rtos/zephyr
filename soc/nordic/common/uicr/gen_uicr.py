"""
Copyright (c) 2025 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0
"""

from __future__ import annotations

import argparse
import ctypes as c
import sys
from itertools import groupby, pairwise
from typing import NamedTuple

from elftools.elf.elffile import ELFFile
from intelhex import IntelHex

# The UICR format version produced by this script
UICR_FORMAT_VERSION_MAJOR = 2
UICR_FORMAT_VERSION_MINOR = 0

# Name of the ELF section containing PERIPHCONF entries.
# Must match the name used in the linker script.
PERIPHCONF_SECTION = "uicr_periphconf_entry"

# Common values for representing enabled/disabled in the UICR format.
ENABLED_VALUE = 0xFFFF_FFFF
DISABLED_VALUE = 0xBD23_28A8


class ScriptError(RuntimeError): ...


class PartitionInfo(NamedTuple):
    """Information about a partition for secure storage validation."""

    address: int
    size: int
    name: str


class PeriphconfEntry(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("regptr", c.c_uint32),
        ("value", c.c_uint32),
    ]


PERIPHCONF_ENTRY_SIZE = c.sizeof(PeriphconfEntry)


class Version(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("MINOR", c.c_uint16),
        ("MAJOR", c.c_uint16),
    ]


class Approtect(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("APPLICATION", c.c_uint32),
        ("RADIOCORE", c.c_uint32),
        ("RESERVED", c.c_uint32),
        ("CORESIGHT", c.c_uint32),
    ]


class Protectedmem(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("ENABLE", c.c_uint32),
        ("SIZE4KB", c.c_uint32),
    ]


class Wdtstart(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("ENABLE", c.c_uint32),
        ("INSTANCE", c.c_uint32),
        ("CRV", c.c_uint32),
    ]


class SecurestorageCrypto(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("APPLICATIONSIZE1KB", c.c_uint32),
        ("RADIOCORESIZE1KB", c.c_uint32),
    ]


class SecurestorageIts(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("APPLICATIONSIZE1KB", c.c_uint32),
        ("RADIOCORESIZE1KB", c.c_uint32),
    ]


class Securestorage(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("ENABLE", c.c_uint32),
        ("ADDRESS", c.c_uint32),
        ("CRYPTO", SecurestorageCrypto),
        ("ITS", SecurestorageIts),
    ]


class Periphconf(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("ENABLE", c.c_uint32),
        ("ADDRESS", c.c_uint32),
        ("MAXCOUNT", c.c_uint32),
    ]


class Mpcconf(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("ENABLE", c.c_uint32),
        ("ADDRESS", c.c_uint32),
        ("MAXCOUNT", c.c_uint32),
    ]


class SecondaryTrigger(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("ENABLE", c.c_uint32),
        ("RESETREAS", c.c_uint32),
        ("RESERVED", c.c_uint32),
    ]


class SecondaryProtectedmem(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("ENABLE", c.c_uint32),
        ("SIZE4KB", c.c_uint32),
    ]


class SecondaryWdtstart(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("ENABLE", c.c_uint32),
        ("INSTANCE", c.c_uint32),
        ("CRV", c.c_uint32),
    ]


class SecondaryPeriphconf(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("ENABLE", c.c_uint32),
        ("ADDRESS", c.c_uint32),
        ("MAXCOUNT", c.c_uint32),
    ]


class SecondaryMpcconf(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("ENABLE", c.c_uint32),
        ("ADDRESS", c.c_uint32),
        ("MAXCOUNT", c.c_uint32),
    ]


class Secondary(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("ENABLE", c.c_uint32),
        ("PROCESSOR", c.c_uint32),
        ("TRIGGER", SecondaryTrigger),
        ("ADDRESS", c.c_uint32),
        ("PROTECTEDMEM", SecondaryProtectedmem),
        ("WDTSTART", SecondaryWdtstart),
        ("PERIPHCONF", SecondaryPeriphconf),
        ("MPCCONF", SecondaryMpcconf),
    ]


class Uicr(c.LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("VERSION", Version),
        ("RESERVED", c.c_uint32),
        ("LOCK", c.c_uint32),
        ("RESERVED1", c.c_uint32),
        ("APPROTECT", Approtect),
        ("ERASEPROTECT", c.c_uint32),
        ("PROTECTEDMEM", Protectedmem),
        ("WDTSTART", Wdtstart),
        ("RESERVED2", c.c_uint32),
        ("SECURESTORAGE", Securestorage),
        ("RESERVED3", c.c_uint32 * 5),
        ("PERIPHCONF", Periphconf),
        ("MPCCONF", Mpcconf),
        ("SECONDARY", Secondary),
        ("PADDING", c.c_uint32 * 15),
    ]


def validate_secure_storage_partitions(args: argparse.Namespace) -> None:
    """
    Validate that secure storage partitions are laid out correctly.

    Args:
        args: Parsed command line arguments containing partition information

    Raises:
        ScriptError: If validation fails
    """
    # Expected order: cpuapp_crypto_partition, cpurad_crypto_partition,
    # cpuapp_its_partition, cpurad_its_partition
    partitions = [
        PartitionInfo(
            args.cpuapp_crypto_address, args.cpuapp_crypto_size, "cpuapp_crypto_partition"
        ),
        PartitionInfo(
            args.cpurad_crypto_address, args.cpurad_crypto_size, "cpurad_crypto_partition"
        ),
        PartitionInfo(args.cpuapp_its_address, args.cpuapp_its_size, "cpuapp_its_partition"),
        PartitionInfo(args.cpurad_its_address, args.cpurad_its_size, "cpurad_its_partition"),
    ]

    # Filter out zero-sized partitions (missing partitions)
    present_partitions = [p for p in partitions if p.size > 0]

    # Require at least one subpartition to be present
    if not present_partitions:
        raise ScriptError(
            "At least one secure storage subpartition must be defined. "
            "Define one or more of: cpuapp_crypto_partition, cpurad_crypto_partition, "
            "cpuapp_its_partition, cpurad_its_partition"
        )

    # Check 4KB alignment for secure storage start address
    if args.securestorage_address % 4096 != 0:
        raise ScriptError(
            f"Secure storage address {args.securestorage_address:#x} must be aligned to 4KB "
            f"(4096 bytes)"
        )

    # Check 4KB alignment for secure storage size
    if args.securestorage_size % 4096 != 0:
        raise ScriptError(
            f"Secure storage size {args.securestorage_size} bytes must be aligned to 4KB "
            f"(4096 bytes)"
        )

    # Check that the first present partition starts at the secure storage address
    first_partition = present_partitions[0]
    if first_partition.address != args.securestorage_address:
        raise ScriptError(
            f"First partition {first_partition.name} starts at {first_partition.address:#x}, "
            f"but must start at secure storage address {args.securestorage_address:#x}"
        )

    # Check that all present partitions have sizes that are multiples of 1KB
    for partition in present_partitions:
        if partition.size % 1024 != 0:
            raise ScriptError(
                f"Partition {partition.name} has size {partition.size} bytes, but must be "
                f"a multiple of 1024 bytes (1KB)"
            )

    # Check that partitions are in correct order and don't overlap
    for curr_partition, next_partition in pairwise(present_partitions):
        # Check order - partitions should be in ascending address order
        if curr_partition.address >= next_partition.address:
            raise ScriptError(
                f"Partition {curr_partition.name} (starts at {curr_partition.address:#x}) "
                f"must come before {next_partition.name} (starts at {next_partition.address:#x})"
            )

        # Check for overlap
        curr_end = curr_partition.address + curr_partition.size
        if curr_end > next_partition.address:
            raise ScriptError(
                f"Partition {curr_partition.name} (ends at {curr_end:#x}) overlaps with "
                f"{next_partition.name} (starts at {next_partition.address:#x})"
            )

        # Check for gaps (should be no gaps between consecutive partitions)
        if curr_end < next_partition.address:
            gap = next_partition.address - curr_end
            raise ScriptError(
                f"Gap of {gap} bytes between {curr_partition.name} (ends at {curr_end:#x}) and "
                f"{next_partition.name} (starts at {next_partition.address:#x})"
            )

    # Check that combined subpartition sizes equal secure_storage_partition size
    total_subpartition_size = sum(p.size for p in present_partitions)
    if total_subpartition_size != args.securestorage_size:
        raise ScriptError(
            f"Combined size of subpartitions ({total_subpartition_size} bytes) does not match "
            f"secure_storage_partition size ({args.securestorage_size} bytes). "
            f"The definition is not coherent."
        )


def main() -> None:
    parser = argparse.ArgumentParser(
        allow_abbrev=False,
        description=(
            "Generate artifacts for the UICR and associated configuration blobs from application "
            "build outputs. User Information Configuration Registers (UICR), in the context of "
            "certain Nordic SoCs, are used to configure system resources, like memory and "
            "peripherals, and to protect the device in various ways."
        ),
    )
    parser.add_argument(
        "--in-periphconf-elf",
        dest="in_periphconf_elfs",
        default=[],
        action="append",
        type=argparse.FileType("rb"),
        help=(
            "Path to an ELF file to extract PERIPHCONF data from. Can be provided multiple times. "
            "The PERIPHCONF data from each ELF file is combined in a single list which is sorted "
            "by ascending address and cleared of duplicate entries."
        ),
    )
    parser.add_argument(
        "--out-merged-hex",
        required=True,
        type=argparse.FileType("w", encoding="utf-8"),
        help="Path to write the merged UICR+PERIPHCONF HEX file to",
    )
    parser.add_argument(
        "--out-uicr-hex",
        required=True,
        type=argparse.FileType("w", encoding="utf-8"),
        help="Path to write the UICR-only HEX file to",
    )
    parser.add_argument(
        "--out-periphconf-hex",
        type=argparse.FileType("w", encoding="utf-8"),
        help="Path to write the PERIPHCONF-only HEX file to",
    )
    parser.add_argument(
        "--periphconf-address",
        default=None,
        type=lambda s: int(s, 0),
        help="Absolute flash address of the PERIPHCONF partition (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--periphconf-size",
        default=None,
        type=lambda s: int(s, 0),
        help="Size in bytes of the PERIPHCONF partition (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--uicr-address",
        required=True,
        type=lambda s: int(s, 0),
        help="Absolute flash address of the UICR region (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--securestorage",
        action="store_true",
        help="Enable secure storage support in UICR",
    )
    parser.add_argument(
        "--securestorage-address",
        default=None,
        type=lambda s: int(s, 0),
        help="Absolute flash address of the secure storage partition (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--securestorage-size",
        default=None,
        type=lambda s: int(s, 0),
        help="Size in bytes of the secure storage partition (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--cpuapp-crypto-address",
        default=0,
        type=lambda s: int(s, 0),
        help="Absolute flash address of cpuapp_crypto_partition (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--cpuapp-crypto-size",
        default=0,
        type=lambda s: int(s, 0),
        help="Size in bytes of cpuapp_crypto_partition (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--cpurad-crypto-address",
        default=0,
        type=lambda s: int(s, 0),
        help="Absolute flash address of cpurad_crypto_partition (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--cpurad-crypto-size",
        default=0,
        type=lambda s: int(s, 0),
        help="Size in bytes of cpurad_crypto_partition (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--cpuapp-its-address",
        default=0,
        type=lambda s: int(s, 0),
        help="Absolute flash address of cpuapp_its_partition (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--cpuapp-its-size",
        default=0,
        type=lambda s: int(s, 0),
        help="Size in bytes of cpuapp_its_partition (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--cpurad-its-address",
        default=0,
        type=lambda s: int(s, 0),
        help="Absolute flash address of cpurad_its_partition (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--cpurad-its-size",
        default=0,
        type=lambda s: int(s, 0),
        help="Size in bytes of cpurad_its_partition (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--secondary",
        action="store_true",
        help="Enable secondary firmware support in UICR",
    )
    parser.add_argument(
        "--secondary-address",
        default=None,
        type=lambda s: int(s, 0),
        help="Absolute flash address of the secondary firmware (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--secondary-periphconf-address",
        default=None,
        type=lambda s: int(s, 0),
        help=(
            "Absolute flash address of the secondary PERIPHCONF partition "
            "(decimal or 0x-prefixed hex)"
        ),
    )
    parser.add_argument(
        "--secondary-periphconf-size",
        default=None,
        type=lambda s: int(s, 0),
        help="Size in bytes of the secondary PERIPHCONF partition (decimal or 0x-prefixed hex)",
    )
    parser.add_argument(
        "--in-secondary-periphconf-elf",
        dest="in_secondary_periphconf_elfs",
        default=[],
        action="append",
        type=argparse.FileType("rb"),
        help=(
            "Path to an ELF file to extract secondary PERIPHCONF data from. "
            "Can be provided multiple times. The secondary PERIPHCONF data from each ELF file "
            "is combined in a single list which is sorted by ascending address and cleared "
            "of duplicate entries."
        ),
    )
    parser.add_argument(
        "--out-secondary-periphconf-hex",
        type=argparse.FileType("w", encoding="utf-8"),
        help="Path to write the secondary PERIPHCONF-only HEX file to",
    )
    args = parser.parse_args()

    try:
        # Validate argument dependencies
        if args.out_periphconf_hex:
            if args.periphconf_address is None:
                raise ScriptError(
                    "--periphconf-address is required when --out-periphconf-hex is used"
                )
            if args.periphconf_size is None:
                raise ScriptError("--periphconf-size is required when --out-periphconf-hex is used")

        # Validate secondary argument dependencies
        if args.secondary and args.secondary_address is None:
            raise ScriptError("--secondary-address is required when --secondary is used")

        if args.out_secondary_periphconf_hex:
            if args.secondary_periphconf_address is None:
                raise ScriptError(
                    "--secondary-periphconf-address is required when "
                    "--out-secondary-periphconf-hex is used"
                )
            if args.secondary_periphconf_size is None:
                raise ScriptError(
                    "--secondary-periphconf-size is required when "
                    "--out-secondary-periphconf-hex is used"
                )

        # Validate secure storage argument dependencies
        if args.securestorage:
            if args.securestorage_address is None:
                raise ScriptError(
                    "--securestorage-address is required when --securestorage is used"
                )
            if args.securestorage_size is None:
                raise ScriptError("--securestorage-size is required when --securestorage is used")

            # Validate partition layout
            validate_secure_storage_partitions(args)

        init_values = DISABLED_VALUE.to_bytes(4, "little") * (c.sizeof(Uicr) // 4)
        uicr = Uicr.from_buffer_copy(init_values)

        uicr.VERSION.MAJOR = UICR_FORMAT_VERSION_MAJOR
        uicr.VERSION.MINOR = UICR_FORMAT_VERSION_MINOR

        # Handle secure storage configuration
        if args.securestorage:
            uicr.SECURESTORAGE.ENABLE = ENABLED_VALUE
            uicr.SECURESTORAGE.ADDRESS = args.securestorage_address

            # Set partition sizes in 1KB units
            uicr.SECURESTORAGE.CRYPTO.APPLICATIONSIZE1KB = args.cpuapp_crypto_size // 1024
            uicr.SECURESTORAGE.CRYPTO.RADIOCORESIZE1KB = args.cpurad_crypto_size // 1024
            uicr.SECURESTORAGE.ITS.APPLICATIONSIZE1KB = args.cpuapp_its_size // 1024
            uicr.SECURESTORAGE.ITS.RADIOCORESIZE1KB = args.cpurad_its_size // 1024

        # Process periphconf data first and configure UICR completely before creating hex objects
        periphconf_hex = IntelHex()
        secondary_periphconf_hex = IntelHex()

        if args.out_periphconf_hex:
            periphconf_combined = extract_and_combine_periphconfs(args.in_periphconf_elfs)

            padding_len = args.periphconf_size - len(periphconf_combined)
            periphconf_final = periphconf_combined + bytes([0xFF for _ in range(padding_len)])

            # Add periphconf data to periphconf hex object
            periphconf_hex.frombytes(periphconf_final, offset=args.periphconf_address)

            # Configure UICR with periphconf settings
            uicr.PERIPHCONF.ENABLE = ENABLED_VALUE
            uicr.PERIPHCONF.ADDRESS = args.periphconf_address

            # MAXCOUNT is given in number of 8-byte peripheral
            # configuration entries and periphconf_size is given in
            # bytes. When setting MAXCOUNT based on the
            # periphconf_size we must first assert that
            # periphconf_size has not been misconfigured.
            if args.periphconf_size % 8 != 0:
                raise ScriptError(
                    f"args.periphconf_size was {args.periphconf_size}, but must be divisible by 8"
                )

            uicr.PERIPHCONF.MAXCOUNT = args.periphconf_size // 8

        # Handle secondary firmware configuration
        if args.secondary:
            uicr.SECONDARY.ENABLE = ENABLED_VALUE
            uicr.SECONDARY.ADDRESS = args.secondary_address

            # Handle secondary periphconf if provided
            if args.out_secondary_periphconf_hex:
                secondary_periphconf_combined = extract_and_combine_periphconfs(
                    args.in_secondary_periphconf_elfs
                )

                padding_len = args.secondary_periphconf_size - len(secondary_periphconf_combined)
                secondary_periphconf_final = secondary_periphconf_combined + bytes(
                    [0xFF for _ in range(padding_len)]
                )

                # Add secondary periphconf data to secondary periphconf hex object
                secondary_periphconf_hex.frombytes(
                    secondary_periphconf_final, offset=args.secondary_periphconf_address
                )

                # Configure UICR with secondary periphconf settings
                uicr.SECONDARY.PERIPHCONF.ENABLE = ENABLED_VALUE
                uicr.SECONDARY.PERIPHCONF.ADDRESS = args.secondary_periphconf_address

                # MAXCOUNT is given in number of 8-byte peripheral
                # configuration entries and secondary_periphconf_size is given in
                # bytes. When setting MAXCOUNT based on the
                # secondary_periphconf_size we must first assert that
                # secondary_periphconf_size has not been misconfigured.
                if args.secondary_periphconf_size % 8 != 0:
                    raise ScriptError(
                        f"args.secondary_periphconf_size was {args.secondary_periphconf_size}, "
                        f"but must be divisible by 8"
                    )

                uicr.SECONDARY.PERIPHCONF.MAXCOUNT = args.secondary_periphconf_size // 8

        # Create UICR hex object with final UICR data
        uicr_hex = IntelHex()
        uicr_hex.frombytes(bytes(uicr), offset=args.uicr_address)

        # Create merged hex by combining UICR and periphconf hex objects
        merged_hex = IntelHex()
        merged_hex.fromdict(uicr_hex.todict())

        if args.out_periphconf_hex:
            periphconf_hex.write_hex_file(args.out_periphconf_hex)
            merged_hex.fromdict(periphconf_hex.todict())

        if args.out_secondary_periphconf_hex:
            secondary_periphconf_hex.write_hex_file(args.out_secondary_periphconf_hex)
            merged_hex.fromdict(secondary_periphconf_hex.todict())

        merged_hex.write_hex_file(args.out_merged_hex)
        uicr_hex.write_hex_file(args.out_uicr_hex)

    except ScriptError as e:
        print(f"Error: {e!s}")
        sys.exit(1)


def extract_and_combine_periphconfs(elf_files: list[argparse.FileType]) -> bytes:
    combined_periphconf = []
    ipcmap_index = 0

    for in_file in elf_files:
        elf = ELFFile(in_file)
        conf_section = elf.get_section_by_name(PERIPHCONF_SECTION)
        if conf_section is None:
            continue

        conf_section_data = conf_section.data()
        num_entries = len(conf_section_data) // PERIPHCONF_ENTRY_SIZE
        periphconf = (PeriphconfEntry * num_entries).from_buffer_copy(conf_section_data)
        ipcmap_index = adjust_ipcmap_entries(periphconf, offset_index=ipcmap_index)
        combined_periphconf.extend(periphconf)

    combined_periphconf.sort(key=lambda e: e.regptr)
    deduplicated_periphconf = []

    for regptr, regptr_entries in groupby(combined_periphconf, key=lambda e: e.regptr):
        entries = list(regptr_entries)
        if len(entries) > 1:
            unique_values = {e.value for e in entries}
            if len(unique_values) > 1:
                raise ScriptError(
                    f"PERIPHCONF has conflicting values for register 0x{regptr:09_x}: "
                    + ", ".join([f"0x{val:09_x}" for val in unique_values])
                )
        deduplicated_periphconf.append(entries[0])

    final_periphconf = (PeriphconfEntry * len(deduplicated_periphconf))()
    for i, entry in enumerate(deduplicated_periphconf):
        final_periphconf[i] = entry

    return bytes(final_periphconf)


# This workaround is currently needed to avoid conflicts in IPCMAP whenever more than
# one image uses IPCMAP, because at the moment each image has no way of knowing which
# IPCMAP channel indices it should use for the configuration it generates locally.
#
# What the workaround does is adjust all IPCMAP entries found in the periphconf by the
# given index offset.
#
# The workaround assumes that IPCMAP entries are allocated sequentially starting from 0
# in each image, it will probably not work for arbitrary IPCMAP entries.
def adjust_ipcmap_entries(periphconf: c.Array[PeriphconfEntry], offset_index: int) -> int:
    max_ipcmap_index = offset_index

    for entry in sorted(periphconf, key=lambda e: e.regptr):
        if IPCMAP_CHANNEL_START_ADDR <= entry.regptr < IPCMAP_CHANNEL_END_ADDR:
            entry.regptr += offset_index * IPCMAP_CHANNEL_SIZE
            entry_ipcmap_index = (entry.regptr - IPCMAP_CHANNEL_START_ADDR) // IPCMAP_CHANNEL_SIZE
            max_ipcmap_index = max(max_ipcmap_index, entry_ipcmap_index)

    return max_ipcmap_index + 1


# Size of each IPCMAP.CHANNEL[i]
IPCMAP_CHANNEL_SIZE = 8
# Number of entries in IPCMAP.CHANNEL
IPCMAP_CHANNEL_COUNT = 16
# Address of IPCMAP.CHANNEL[0]
IPCMAP_CHANNEL_START_ADDR = 0x5F92_3000 + 256 * 4
# Address of IPCMAP.CHANNEL[channel count] + 1
IPCMAP_CHANNEL_END_ADDR = IPCMAP_CHANNEL_START_ADDR + IPCMAP_CHANNEL_SIZE * IPCMAP_CHANNEL_COUNT


if __name__ == "__main__":
    main()
