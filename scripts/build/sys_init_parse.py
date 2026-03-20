#!/usr/bin/env python3
#
# Copyright (c) 2026 PicoHeart Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

"""
This script parses the sys_init section of the ELF file
to extract the initialization functions.
Usage:
usage: extract_init_symbols.py
        [-h] [--log-level {DEBUG,INFO,WARNING,ERROR,CRITICAL}]
        [--compiler-path COMPILER_PATH]
        map_file [elf_file]
    python3 sys_init_parse.py <map_file> <elf_file_path>
"""

import argparse
import logging
import re
import subprocess
import sys

# configure logging system
logging.basicConfig(
    level=logging.INFO,  # default log level
    format="%(message)s",  # log format
)

# create logger
logger = logging.getLogger(__name__)

elf_class = 64
compiler_path = ""


def determine_elf_bitness(elf_path):
    """
    Determine whether the ELF file is 32-bit or 64-bit
    :param elf_path: Path to the ELF file
    :return: 32 or 64 indicating the system bitness
    """
    # Use readelf command to view ELF file header information
    result = subprocess.run(
        [compiler_path + "readelf", "-h", elf_path],
        capture_output=True,
        text=True,
        check=True,
    )
    output = result.stdout

    # Find the Class field
    for line in output.split("\n"):
        if "Class:" in line:
            if "ELF64" in line:
                return 64
            elif "ELF32" in line:
                return 32
    return 64


def determine_elf_big_endian(elf_path):
    """
    Determine whether the ELF file is big-endian or little-endian
    :param elf_path: Path to the ELF file
    :return: True if big-endian, False if little-endian
    """
    # Use readelf command to view ELF file header information
    result = subprocess.run(
        [compiler_path + "readelf", "-h", elf_path],
        capture_output=True,
        text=True,
        check=True,
    )
    output = result.stdout

    # Find the Data field
    for line in output.split("\n"):
        if "Data:" in line:
            if "big endian" in line:
                return True
            elif "little endian" in line:
                return False
    return False


def extract_init_fn(entry_addr, elf_file_path, ptr_size):
    """
    Extract the initialization function address from the given entry address

    Args:
        entry_addr: The address of the initialization entry
        elf_file_path: Path to the ELF file
        ptr_size: Pointer size (4 for 32-bit, 8 for 64-bit)

    Returns:
        The address of the initialization function as a hex string, or None if not found
    """
    # Use objdump to read the content around the entry address
    cmd = [
        compiler_path + "objdump",
        "-s",
        "--start-address",
        hex(entry_addr),
        "--stop-address",
        hex(entry_addr + ptr_size * 4),  # Read enough data
        "-w",
        elf_file_path,
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    logger.debug(f"entry_addr = {hex(entry_addr)}")
    logger.debug(f"objdump output: {result.stdout}")
    if result.returncode != 0:
        return None

    # Parse objdump output to extract hexadecimal data
    hex_data = ""
    for line in result.stdout.strip().split("\n"):
        # Use regular expression to match lines starting with hexadecimal address
        # Address format can be: 0x12345678 or 12345678
        if re.match(r"^\s*(0x[0-9a-fA-F]+|[0-9a-fA-F]+)\s", line.strip()):
            parts = line.split()
            if len(parts) >= 2:
                hex_part = "".join(parts[1:-1])  # Exclude address and ASCII part
                hex_data += hex_part

    if not hex_data:
        return None

    # 1. read init_fn field
    logger.debug(f"hex_data = {hex_data}")
    init_fn_hex = hex_data[: ptr_size * 2]
    if len(init_fn_hex) < ptr_size * 2:
        return None

    # Convert to little-endian integer
    init_fn_addr = int.from_bytes(
        bytes.fromhex(init_fn_hex),
        byteorder="big" if determine_elf_big_endian(elf_file_path) else "little",
    )

    # If init_fn is not null (not all zeros), return it directly
    if init_fn_addr != 0:
        return hex(init_fn_addr)

    # 2. If init_fn is null, read dev pointer
    dev_hex = hex_data[ptr_size * 2 : ptr_size * 4]
    if len(dev_hex) < ptr_size * 2:
        return None

    # Convert to little-endian integer
    dev_addr = int.from_bytes(
        bytes.fromhex(dev_hex),
        byteorder="big" if determine_elf_big_endian(elf_file_path) else "little",
    )
    logger.debug(f"dev_addr = {hex(dev_addr)}")
    if dev_addr == 0:
        return None

    # 3. If dev is not null, read ops.init field
    # Calculate the offset of ops.init field in device structure
    ops_init_offset = 5 * ptr_size

    # Use objdump to read the content around the ops.init address
    cmd = [
        compiler_path + "objdump",
        "-s",
        "--start-address",
        hex(dev_addr + ops_init_offset),
        "--stop-address",
        hex(dev_addr + ops_init_offset + ptr_size),
        "-w",
        elf_file_path,
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        return None

    # Parse objdump output to extract hexadecimal data
    hex_data = ""
    logger.debug(f"ops.init objdump output: {result.stdout}")
    for line in result.stdout.strip().split("\n"):
        # Use regular expression to match lines starting with hexadecimal address
        # Address format can be: 0x12345678 or 12345678
        if re.match(r"^\s*(0x[0-9a-fA-F]+|[0-9a-fA-F]+)\s", line.strip()):
            parts = line.split()
            if len(parts) >= 2:
                hex_part = "".join(parts[1:-1])  # Exclude address and ASCII part
                hex_data += hex_part

    if not hex_data:
        return None

    # Convert to little-endian integer
    ops_init_addr = int.from_bytes(
        bytes.fromhex(hex_data),
        byteorder="big" if determine_elf_big_endian(elf_file_path) else "little",
    )
    logger.debug(f"ops.init addr = {hex(ops_init_addr)}")

    if ops_init_addr != 0:
        return hex(ops_init_addr)

    return None


def find_function_name(elf_file_path, fn_address, nm_result):
    """Find function name and source file from address using addr2line"""

    # Convert hex string to integer to normalize format
    addr_int = int(fn_address, 16)

    # Search for the address in nm output
    function_name = "Unknown function"
    source_file = "Unknown file"

    for line in nm_result.stdout.strip().split("\n"):
        parts = line.strip().split()
        if len(parts) >= 3:
            try:
                # Extract and normalize the address from nm output
                nm_addr = int(parts[0], 16)
                if nm_addr == addr_int:
                    function_name = parts[2]
                    # Check if source file information is available
                    if len(parts) >= 4:
                        source_file = parts[3]
                        # Clean up source file path (remove line number if present)
                        source_file = re.sub(r":\d+$", "", source_file)
                    break
            except ValueError:
                # Skip lines that don't contain valid hex addresses
                continue

    return (function_name, source_file)


def extract_init_symbols(map_file_path):
    symbols = []

    try:
        with open(map_file_path) as f:
            lines = f.readlines()
            i = 0
            while i < len(lines):
                line1 = lines[i].strip()
                if line1.startswith(".z_init_"):
                    # line1 is the symbol name
                    symbol_name = line1

                    # line2 contains address, size, and filename
                    if i + 1 < len(lines):
                        line2 = lines[i + 1].strip()
                        # Use regular expression to extract filename from line2
                        file_match = re.search(r"0x[0-9a-fA-F]+\s+0x[0-9a-fA-F]+\s+(.+)$", line2)
                        if file_match:
                            file_name = file_match.group(1)

                            # Extract address and size
                            address_size_match = re.search(
                                r"0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+.+$", line2
                            )
                            if not address_size_match:
                                i += 1
                                continue

                            address = address_size_match.group(1)
                            size_hex = address_size_match.group(2)
                            symbol_size = int(size_hex, 16)

                            # Match all allowed level formats, extract prio number
                            # Support two formats:
                            # 1. .z_init_PRE_KERNEL_1_P_40_SUB_00007_
                            # 2. z_init_PRE_KERNEL_150_00026_
                            priority_pattern = (
                                r"^\.?z_init_(?:EARLY|PRE_KERNEL_1|PRE_KERNEL_2|POST_KERNEL|"
                                r"APPLICATION|SMP)(?:_|_P_)?(\d+)_(?:SUB_)?\d+_?$"
                            )
                            priority_match = re.search(priority_pattern, symbol_name)

                            if priority_match:
                                priority = int(priority_match.group(1))
                                # Ensure priority is within 0-100 range
                                priority = max(0, min(100, priority))
                            else:
                                priority = 0

                            # Extract init level: directly match all allowed
                            # levels defined in init.h
                            # Allowed levels: EARLY, PRE_KERNEL_1, PRE_KERNEL_2,
                            # POST_KERNEL, APPLICATION, SMP
                            level_pattern = (
                                r"^\.?z_init_((?:EARLY|PRE_KERNEL_1|PRE_KERNEL_2|POST_KERNEL|"
                                r"APPLICATION|SMP))(?:_|\d|$)"
                            )
                            level_match = re.search(level_pattern, symbol_name)

                            if level_match:
                                # Extract init level after matching allowed formats
                                init_level = level_match.group(1)
                                if init_level.startswith("POST_KERNEL"):
                                    init_level = "POST_KERNEL"
                            else:
                                init_level = "Unknown"  # Display Unknown if no match

                            # Check if symbol name ends with a number greater than 0
                            # look for the last group of digits before the underscore
                            last_digits_match = re.search(r"(\d+)_$", symbol_name)
                            if last_digits_match:
                                last_digits = int(last_digits_match.group(1))
                                is_device_init = (
                                    "device init" if last_digits > 0 else "non-device init"
                                )
                                device_id = f"ord_{last_digits}" if last_digits > 0 else ""
                            else:
                                is_device_init = "non-device init"
                                device_id = ""

                            # Handle single init entry (size 0x10) or
                            # symbols containing multiple entries
                            if elf_class == 64:
                                if symbol_size == 0x10:
                                    # single init entry
                                    symbols.append(
                                        (
                                            priority,
                                            init_level,
                                            file_name,
                                            is_device_init,
                                            device_id,
                                            address,
                                        )
                                    )
                                elif symbol_size > 0x10:
                                    # symbols containing multiple entries,
                                    # each entry occupies 0x10 bytes
                                    num_entries = symbol_size // 0x10
                                    base_addr = int(address, 16)

                                    for i_entry in range(num_entries):
                                        # Calculate the address of each entry
                                        entry_addr = base_addr + (i_entry * 0x10)
                                        entry_addr_hex = hex(entry_addr)[2:]

                                        # append each entry as a separate symbol
                                        symbols.append(
                                            (
                                                priority,
                                                init_level,
                                                file_name,
                                                is_device_init,
                                                device_id,
                                                entry_addr_hex,
                                            )
                                        )
                            else:
                                if symbol_size == 0x8:
                                    # single init entry
                                    symbols.append(
                                        (
                                            priority,
                                            init_level,
                                            file_name,
                                            is_device_init,
                                            device_id,
                                            address,
                                        )
                                    )
                                elif symbol_size > 0x8:
                                    # symbols containing multiple entries, each entry
                                    # occupies 0x8 bytes
                                    num_entries = symbol_size // 0x8
                                    base_addr = int(address, 16)

                                    for i_entry in range(num_entries):
                                        # Calculate the address of each entry
                                        entry_addr = base_addr + (i_entry * 0x8)
                                        entry_addr_hex = hex(entry_addr)[2:]

                                        # append each entry as a separate symbol
                                        symbols.append(
                                            (
                                                priority,
                                                init_level,
                                                file_name,
                                                is_device_init,
                                                device_id,
                                                entry_addr_hex,
                                            )
                                        )
                        i += 1
                i += 1
    except FileNotFoundError:
        logger.error(f"File '{map_file_path}' not found.")
        sys.exit(1)
    except Exception as e:
        logger.error(f"Error: {e}")
        sys.exit(1)

    # symbols are already sorted by priority in the map file
    return symbols


def main():
    parser = argparse.ArgumentParser(
        description="Extract initialization symbols from Zephyr map file",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        allow_abbrev=False,
    )

    # add positional arguments
    parser.add_argument("map_file", help="Path to zephyr.map file")
    parser.add_argument(
        "elf_file",
        nargs="?",
        help="Path to zephyr.elf file (optional, for function name extraction)",
    )

    # add optional arguments
    parser.add_argument(
        "--log-level",
        "-l",
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
        default="INFO",
        help="Set the logging level",
    )
    parser.add_argument("--compiler-path", "-c", default="", help="Path to the compiler toolchain")

    # parse arguments
    args = parser.parse_args()

    # set logging level
    logger.setLevel(getattr(logging, args.log_level))

    # set global variables
    global compiler_path
    compiler_path = args.compiler_path
    map_file_path = args.map_file
    elf_file_path = args.elf_file

    # determine ELF file bitness
    global elf_class
    elf_class = 64  # default value is 64-bit
    if elf_file_path:
        elf_class = determine_elf_bitness(elf_file_path)
        logger.info(f"ELF file is {elf_class}-bit")

    symbols = extract_init_symbols(map_file_path)

    logger.info("\nSymbols extracted from zephyr.map (in map file order):")

    if elf_file_path:
        logger.info("-" * 150)
        logger.info(
            f"{'Priority':<10} {'Type':<15} {'Init Level':<20} {'Device ID':<10} \
            {'Target Function':<30} {'Source File':<50}"
        )
        logger.info("-" * 150)
        cmd = [compiler_path + "nm", "-n", "-l", elf_file_path]
        logger.debug("Running command: " + " ".join(cmd))

        nm_result = subprocess.run(cmd, capture_output=True, text=True)
        if nm_result.returncode != 0:
            logger.error(f"Error: {nm_result.stderr.strip()}")
            sys.exit(1)
        for (
            priority,
            init_level,
            _,
            is_device_init,
            device_id,
            address,
        ) in symbols:
            # Extract target function name if elf file is provided
            target_function = "N/A"
            source_file = ""
            logger.debug(f"address = {address}")
            if address and elf_file_path:
                # transform address to integer
                entry_addr = int(address, 16)
                # determine pointer size based on ELF file bitness
                ptr_size = 8 if elf_class == 64 else 4
                init_fn_addr = extract_init_fn(entry_addr, elf_file_path, ptr_size)
                if init_fn_addr:
                    target_function, source_file = find_function_name(
                        elf_file_path, init_fn_addr, nm_result
                    )

            logger.info(
                f"{priority:<10} {is_device_init:<15} {init_level:<20} {device_id:<10} \
                {target_function:<30} {source_file:<50}"
            )
    else:
        logger.info("-" * 125)
        logger.info(
            f"{'Priority':<10} {'Type':<15} {'Init Level':<20} {'Device ID':<10} {'File Name':<40}"
        )
        logger.info("-" * 125)

        for (
            priority,
            init_level,
            file_name,
            is_device_init,
            device_id,
        ) in symbols:
            logger.info(
                f"{priority:<10} {is_device_init:<15} {init_level:<20} {device_id:<10} \
                    {file_name:<40}"
            )

    logger.info("-" * 150 if elf_file_path else "-" * 125)
    logger.info(f"Total symbols found: {len(symbols)}")


if __name__ == "__main__":
    main()
