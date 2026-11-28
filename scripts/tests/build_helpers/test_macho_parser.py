#!/usr/bin/env python3

# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for the Mach-O parser used by check_init_priorities.py."""

import io
import os
import struct
import sys
import unittest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/build_helpers"))

from macho_parser import MachOFile  # noqa: E402


def _macho_image():
    """Build a minimal 64-bit Mach-O image with one init entry."""
    segment_command_size = 72 + 80
    symtab_command_size = 24
    load_commands_size = segment_command_size + symtab_command_size
    data_offset = 32 + load_commands_size
    data = struct.pack("<QQ", 0x2000, 0)
    symbol_offset = data_offset + len(data)
    strings = b"\0___init_test\0"
    string_offset = symbol_offset + 16

    header = struct.pack(
        "<IiiIIIII",
        0xFEEDFACF,
        0x0100000C,
        0,
        2,
        2,
        load_commands_size,
        0,
        0,
    )
    segment = struct.pack(
        "<II16sQQQQiiII",
        0x19,
        segment_command_size,
        b"__DATA\0\0\0\0\0\0\0\0\0\0",
        0x1000,
        len(data),
        data_offset,
        len(data),
        7,
        3,
        1,
        0,
    )
    section = struct.pack(
        "<16s16sQQIIIIIIII",
        b"zi1\0\0\0\0\0\0\0\0\0\0\0\0\0",
        b"__DATA\0\0\0\0\0\0\0\0\0\0",
        0x1000,
        len(data),
        data_offset,
        3,
        0,
        0,
        0,
        0,
        0,
        0,
    )
    symtab = struct.pack(
        "<IIIIII",
        2,
        symtab_command_size,
        symbol_offset,
        1,
        string_offset,
        len(strings),
    )
    symbol = struct.pack("<IBBHQ", 1, 0x0E, 1, 0, 0x1000)

    return header + segment + section + symtab + data + symbol + strings


class MachOFileTest(unittest.TestCase):
    """Test the Mach-O structures consumed by the init checker."""

    def test_init_entry_and_pointer(self):
        image = MachOFile(io.BytesIO(_macho_image()))

        self.assertEqual(image.init_entries()[1][0].name, "__init_test")
        self.assertEqual(image.init_entries()[1][0].size, 16)
        self.assertEqual(image.read_pointer(0x1000, 0, 1), 0x2000)
        self.assertEqual(image.read_pointer(0x1000, 1, 1), 0)


if __name__ == "__main__":
    unittest.main()
