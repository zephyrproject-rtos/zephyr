#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""
Tests for uf2conv
"""

import contextlib
import io
import os
import struct
import sys
import unittest
from unittest import mock

import uf2conv

UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30
BASE = 0x1000


def block(
    addr,
    payload,
    blockno=0,
    numblocks=1,
    flags=0,
    datalen=None,
    magic0=UF2_MAGIC_START0,
    magic1=UF2_MAGIC_START1,
):
    """One 512-byte UF2 block."""
    head = struct.pack(
        b"<IIIIIIII",
        magic0,
        magic1,
        flags,
        addr,
        len(payload) if datalen is None else datalen,
        blockno,
        numblocks,
        0,
    )
    return head + payload + b"\x00" * (476 - len(payload)) + struct.pack(b"<I", UF2_MAGIC_END)


class TestReadingAUf2Image(unittest.TestCase):
    """convert_from_uf2() walks the blocks and joins what they carry.

    scripts/west_commands/bindesc.py carries a copy of this function; the
    cases here are the ones both of them answer the same way.
    """

    def convert(self, buf):
        # It reports on the last block and leaves appstartaddr behind, so
        # neither the output of the run nor the next one is affected.
        with (
            mock.patch.object(uf2conv, "appstartaddr", 0x2000),
            mock.patch.object(uf2conv, "familyid", 0x0),
            contextlib.redirect_stdout(io.StringIO()) as out,
        ):
            return uf2conv.convert_from_uf2(buf), out.getvalue()

    def refuse(self, buf):
        with self.assertRaises(AssertionError) as caught:
            self.convert(buf)
        return str(caught.exception)

    def test_contiguous_blocks_are_concatenated(self):
        buf = block(BASE, b"A" * 16, 0, 2) + block(BASE + 16, b"B" * 16, 1, 2)

        self.assertEqual(self.convert(buf)[0], b"A" * 16 + b"B" * 16)

    def test_a_gap_between_blocks_is_padded_with_zeros(self):
        """An image with a hole in it is the normal case, not an odd one."""
        buf = block(BASE, b"A" * 16, 0, 2) + block(BASE + 32, b"B" * 16, 1, 2)

        self.assertEqual(self.convert(buf)[0], b"A" * 16 + b"\x00" * 16 + b"B" * 16)

    def test_a_block_with_the_wrong_magic_is_skipped_and_reported(self):
        """Reported and skipped: the rest of the image still comes back."""
        buf = block(BASE, b"A" * 16, 0, 2, magic0=0xDEADBEEF) + block(BASE, b"B" * 16, 1, 2)

        payload, said = self.convert(buf)

        self.assertEqual(payload, b"B" * 16)
        self.assertIn("Skipping block at 0", said)

    def test_a_block_that_is_not_for_flash_is_skipped(self):
        buf = block(BASE, b"A" * 16, 0, 2, flags=0x1) + block(BASE, b"B" * 16, 1, 2)

        self.assertEqual(self.convert(buf)[0], b"B" * 16)

    def test_a_tail_too_short_to_be_a_block_is_ignored(self):
        buf = block(BASE, b"A" * 16) + b"\x00" * 100

        self.assertEqual(self.convert(buf)[0], b"A" * 16)

    def test_a_payload_larger_than_the_block_is_refused(self):
        self.assertEqual(
            self.refuse(block(BASE, b"A" * 16, datalen=477)),
            "Invalid UF2 data size at 0",
        )

    def test_a_block_before_the_one_ahead_of_it_is_refused(self):
        buf = block(BASE, b"A" * 16, 0, 2) + block(BASE - 256, b"B" * 16, 1, 2)

        self.assertEqual(self.refuse(buf), "Block out of order at 512")

    def test_a_gap_that_is_not_a_whole_number_of_words_is_refused(self):
        buf = block(BASE, b"A" * 16, 0, 2) + block(BASE + 19, b"B" * 16, 1, 2)

        self.assertEqual(self.refuse(buf), "Non-word padding size at 512")

    def test_a_gap_of_more_than_ten_megabytes_is_refused(self):
        buf = block(BASE, b"A" * 16, 0, 2) + block(BASE + 16 + 11 * 1024 * 1024, b"B" * 16, 1, 2)

        self.assertEqual(
            self.refuse(buf),
            "More than 10M of padding needed at 512",
        )


class TestGetDrivesOnWindows(unittest.TestCase):
    """get_drives() asks PowerShell which volumes to offer."""

    def get_drives(self, output):
        # has_info() decides what survives; every candidate carries the
        # marker file here, so what is under test is the listing itself.
        with (
            mock.patch.object(sys, "platform", "win32"),
            mock.patch.object(uf2conv.subprocess, "check_output", return_value=output) as run,
            mock.patch.object(uf2conv.os.path, "isfile", return_value=True),
        ):
            return uf2conv.get_drives(), run.call_args[0][0]

    def test_one_device_id_per_line(self):
        drives, _ = self.get_drives(b"D:\r\nE:\r\n")

        self.assertEqual(drives, ["D:", "E:"])

    def test_blank_lines_are_not_drives(self):
        # Nothing attached is an empty answer, not an empty drive letter.
        drives, _ = self.get_drives(b"\r\n")

        self.assertEqual(drives, [])

    def test_the_query_asks_for_a_removable_fat_volume(self):
        """Both halves of what the wmic table used to be read for."""
        _, cmd = self.get_drives(b"")
        query = " ".join(cmd)

        self.assertEqual(cmd[0], "powershell")
        self.assertIn("DriveType=2", query)
        self.assertIn("FileSystem='FAT'", query)

    def test_wmic_is_not_asked(self):
        # It was removed in Windows 11 24H2, so calling it raises before it
        # can answer anything.
        _, cmd = self.get_drives(b"")

        self.assertNotIn("wmic", cmd)


class TestGetDrivesElsewhere(unittest.TestCase):
    """Off Windows the drives are directories under a few search paths."""

    def get_drives(self, platform, dirs, entries, with_marker=()):
        # Paths are joined the way the module joins them, so the assertions
        # below read the same on a POSIX host and on Windows.
        def isdir(path):
            return path in dirs

        def listdir(path):
            return entries.get(path, [])

        def isfile(path):
            return path in with_marker

        with (
            mock.patch.object(sys, "platform", platform),
            mock.patch.dict(uf2conv.os.environ, {"USER": "someone"}, clear=False),
            mock.patch.object(uf2conv.os.path, "isdir", isdir),
            mock.patch.object(uf2conv.os, "listdir", listdir),
            mock.patch.object(uf2conv.os.path, "isfile", isfile),
        ):
            return uf2conv.get_drives()

    def test_a_directory_holding_the_marker_file_is_a_drive(self):
        board = os.path.join("/media", "BOARD")

        drives = self.get_drives(
            "linux",
            dirs={"/media", board},
            entries={"/media": ["BOARD"]},
            with_marker=(board + uf2conv.INFO_FILE,),
        )

        self.assertEqual(drives, [board])

    def test_a_file_beside_it_is_not(self):
        """The entry is what has to be a directory, not the path it is under."""
        board = os.path.join("/media", "BOARD")
        notes = os.path.join("/media", "notes.txt")

        drives = self.get_drives(
            "linux",
            dirs={"/media", board},  # notes.txt is not a directory
            entries={"/media": ["BOARD", "notes.txt"]},
            with_marker=(board + uf2conv.INFO_FILE, notes + uf2conv.INFO_FILE),
        )

        self.assertEqual(drives, [board])

    def test_a_directory_without_the_marker_file_is_not(self):
        board = os.path.join("/media", "BOARD")
        stick = os.path.join("/media", "usb-stick")

        drives = self.get_drives(
            "linux",
            dirs={"/media", board, stick},
            entries={"/media": ["BOARD", "usb-stick"]},
            with_marker=(board + uf2conv.INFO_FILE,),
        )

        self.assertEqual(drives, [board])

    def test_macos_looks_under_volumes(self):
        board = os.path.join("/Volumes", "BOARD")

        drives = self.get_drives(
            "darwin",
            dirs={"/Volumes", board},
            entries={"/Volumes": ["BOARD"]},
            with_marker=(board + uf2conv.INFO_FILE,),
        )

        self.assertEqual(drives, [board])

    def test_nothing_mounted(self):
        self.assertEqual(self.get_drives("linux", dirs=set(), entries={}), [])


class TestConvertingAHexFile(unittest.TestCase):
    """A hex file need not mention every byte of a block."""

    # Four bytes at 0x1000, then end-of-file. Nothing says what the rest of
    # that block holds.
    HEX = ":04100000DEADBEEF59\n:00000001FF\n"

    def convert(self):
        with (
            mock.patch.object(uf2conv, "appstartaddr", None),
            mock.patch.object(uf2conv, "familyid", 0x0),
        ):
            return uf2conv.convert_from_hex_to_uf2(self.HEX)

    def payload(self):
        # One 512-byte block: 32 bytes of header, then the data.
        return self.convert()[32:288]

    def test_the_bytes_the_file_names_are_kept(self):
        self.assertEqual(self.payload()[:4], bytes.fromhex("deadbeef"))

    def test_the_rest_of_the_block_reads_as_erased_flash(self):
        """0xFF, which is what the device would have there anyway."""
        self.assertEqual(set(self.payload()[4:]), {0xFF})


if __name__ == "__main__":
    unittest.main()
