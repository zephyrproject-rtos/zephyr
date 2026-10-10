#!/usr/bin/env python3
#
# Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""Convert a flat binary to the BK7258's on-flash format.

Code is stored in flash as 32 bytes of data followed by a CRC-16 over those
bytes, repeating. The flash controller checks and removes the CRC words as the
CPU reads through the memory mapping, so an image written without
them does not run. Every 32 bytes of address space occupy 34 bytes of flash,
which is why flash offsets and CPU addresses do not differ by a constant.

The build runs this as a post-build step, turning zephyr.bin into
zephyr.crc.bin.
"""

import argparse
import sys
from pathlib import Path

BLOCK = 32


def crc16(data: bytes) -> int:
    """CRC-16 as the flash controller computes it: poly 0x8005, init 0xFFFF."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x8005) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def interleave(image: bytes) -> bytes:
    """Insert a CRC word after every 32 bytes, padding the last block."""
    image += b"\xff" * (-len(image) % BLOCK)
    return b"".join(
        image[i : i + BLOCK] + crc16(image[i : i + BLOCK]).to_bytes(2, "big")
        for i in range(0, len(image), BLOCK)
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0], allow_abbrev=False)
    parser.add_argument("input", type=Path, help="flat application binary, normally zephyr.bin")
    parser.add_argument(
        "output", type=Path, help="image in on-flash format, normally zephyr.crc.bin"
    )
    args = parser.parse_args()
    print(f"Generating {args.output.resolve()} with flash CRC words")
    args.output.write_bytes(interleave(args.input.read_bytes()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
