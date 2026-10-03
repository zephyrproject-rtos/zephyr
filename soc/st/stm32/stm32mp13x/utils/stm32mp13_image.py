#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Generate an unsigned STM32MP13 image with an STM32 header version 2.0."""

import argparse
import struct
import sys
from pathlib import Path

DESCRIPTION = (
    "Add an unsigned 512-byte STM32 header version 2.0 to a raw STM32MP13 binary. "
    "The generated .stm32 image can be loaded by the STM32MP13 BootROM."
)
HEADER_SIZE = 512
POST_HEADER_SIZE = HEADER_SIZE - 128
UINT32_MAX = (1 << 32) - 1


def uint32(value):
    """Parse value as an unsigned 32-bit integer.

    @param value Integer text accepted by ``int(value, 0)``.
    @return The parsed integer.
    """
    parsed = int(value, 0)
    if not 0 <= parsed <= UINT32_MAX:
        raise argparse.ArgumentTypeError(f"value outside unsigned 32-bit range: {value}")

    return parsed


def create_header(payload, entry_point, load_address, binary_type):
    """Create an STM32 header version 2.0 for an STM32MP13 payload.

    @param payload Image bytes covered by the header.
    @param entry_point Address where the BootROM starts the image.
    @param load_address Address where the BootROM loads the image.
    @param binary_type STM32 image type stored in the header.
    @return The 512-byte image header.
    """
    if len(payload) > UINT32_MAX:
        raise ValueError("payload is too large for the STM32 image header")

    header = bytearray(HEADER_SIZE)
    header[0:4] = b"STM2"
    struct.pack_into("<I", header, 68, sum(payload) & UINT32_MAX)
    header[72:76] = bytes((0, 0, 2, 0))
    struct.pack_into("<I", header, 76, len(payload))
    struct.pack_into("<I", header, 80, entry_point)
    struct.pack_into("<I", header, 88, load_address)
    struct.pack_into("<I", header, 100, 1 << 31)
    struct.pack_into("<I", header, 104, POST_HEADER_SIZE)
    struct.pack_into("<I", header, 108, binary_type)
    header[128:132] = b"ST\xff\xff"
    struct.pack_into("<I", header, 132, POST_HEADER_SIZE)

    return bytes(header)


def main(argv=None):
    """Generate an STM32MP13 image from the command-line arguments.

    @param argv Arguments to parse, or ``None`` to use the process arguments.
    """
    parser = argparse.ArgumentParser(description=DESCRIPTION, allow_abbrev=False)
    parser.add_argument("input", type=Path, help="input binary")
    parser.add_argument("output", type=Path, help="output STM32 image")
    parser.add_argument("--entry-point", type=uint32, required=True, help="image entry address")
    parser.add_argument("--load-address", type=uint32, required=True, help="image load address")
    parser.add_argument("--binary-type", type=uint32, required=True, help="STM32 image type")

    if argv is None:
        argv = sys.argv[1:]
    if len(argv) == 0:
        parser.print_help()
        return

    args = parser.parse_args(argv)

    payload = args.input.read_bytes()
    header = create_header(payload, args.entry_point, args.load_address, args.binary_type)
    args.output.write_bytes(header + payload)


if __name__ == "__main__":
    main()
