#!/usr/bin/env python3
#
# Copyright 2020-2021 Beken
# Copyright (c) 2026 Embracecactus
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from pathlib import Path

BLOCK_SIZE = 32
CRC_POLYNOMIAL = 0x8005

# The BootROM only boots a flash image on cold reset when this marker is
# present at offset 0x100. The offset holds the vectors of IRQ 48 and 49.
BOOT_MARKER_OFFSET = 0x100
BOOT_MARKER = b"BK7236\x10\x00"


def crc16_cms(data: bytes) -> int:
    crc = 0xFFFF

    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = (crc << 1) ^ CRC_POLYNOMIAL if crc & 0x8000 else crc << 1
            crc &= 0xFFFF

    return crc


def add_block_crc(data: bytes) -> bytes:
    output = bytearray()

    for offset in range(0, len(data), BLOCK_SIZE):
        block = data[offset : offset + BLOCK_SIZE].ljust(BLOCK_SIZE, b"\xff")
        output.extend(block)
        output.extend(crc16_cms(block).to_bytes(2, "big"))

    return bytes(output)


def add_boot_marker(data: bytes) -> bytes:
    end = BOOT_MARKER_OFFSET + len(BOOT_MARKER)

    if len(data) < end:
        raise ValueError(f"image is smaller than {end:#x} bytes")

    return data[:BOOT_MARKER_OFFSET] + BOOT_MARKER + data[end:]


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Create a BK7258 flash image with boot marker and per-block CRC",
        allow_abbrev=False,
    )
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    args.output.write_bytes(add_block_crc(add_boot_marker(args.input.read_bytes())))


if __name__ == "__main__":
    main()
