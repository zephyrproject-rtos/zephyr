# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Xtensa coredump stop-signal regression tests."""

import struct
import sys
from pathlib import Path
from unittest.mock import Mock

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "coredump"))

from coredump_parser.log_parser import CoredumpLogFile  # noqa: E402
from gdbstubs import get_gdbstub  # noqa: E402
from gdbstubs.arch.xtensa import (  # noqa: E402
    XtensaSoc,
    XtensaToolchain,
    get_gdb_reg_definition,
)


@pytest.mark.parametrize("version, soc", [(1, XtensaSoc.ESP32), (2, XtensaSoc.DC233C)])
@pytest.mark.parametrize(
    "cause",
    [0, 2, 3, 5, 6, 8, 9, 12, 13, 14, 15, 16, 17, 18, 20, 24, 25, 26, 28, 29, *range(32, 40), 63],
)
def test_stop_signal(tmp_path, version, soc, cause):
    """Decode a complete dump and report its exception through the GDB '?' packet."""
    definition = get_gdb_reg_definition(soc, XtensaToolchain.ZEPHYR)
    registers = [0] * (struct.calcsize(definition.ARCH_DATA_BLK_STRUCT_REGS) // 4)
    registers[list(definition.RegNum).index(definition.RegNum.EXCCAUSE)] = cause
    arch_data = struct.pack("<BH", soc.value, version)
    if version == 2:
        arch_data += bytes([XtensaToolchain.ZEPHYR.value])
    arch_data += struct.pack(definition.ARCH_DATA_BLK_STRUCT_REGS, *registers)
    dump = tmp_path / "coredump.bin"
    dump.write_bytes(
        struct.pack("<ccHHBBI", b"Z", b"E", 2, 5, 32, 0, 0)
        + struct.pack("<cHH", b"A", 1, len(arch_data))
        + arch_data
    )
    logfile = CoredumpLogFile(dump)
    logfile.open()
    try:
        assert logfile.parse()
        stub = get_gdbstub(logfile, Mock(get_memory_regions=lambda: []))
    finally:
        logfile.close()

    expected = 7
    if cause == 0:
        expected = 4
    elif cause in (3, 8, 14, 15, 16, 17, 18, 20, 24, 25, 26, 28, 29):
        expected = 11
    elif cause in (6, 32, 33, 34, 35, 36, 37, 38, 39):
        expected = 8
    elif cause == 63:
        expected = 6
    assert stub.exception_code == cause
    stub.socket = Mock()
    stub.handle_signal_query_packet()
    packet = f"S{expected:02X}".encode()
    checksum = sum(packet) % 256
    stub.socket.send.assert_called_once_with(b"$" + packet + f"#{checksum:02X}".encode())
