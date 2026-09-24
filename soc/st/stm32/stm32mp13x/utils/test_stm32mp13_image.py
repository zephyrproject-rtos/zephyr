# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import importlib.util
import struct
from pathlib import Path

SCRIPT = Path(__file__).with_name("stm32mp13_image.py")
SPEC = importlib.util.spec_from_file_location("stm32mp13_image", SCRIPT)
stm32mp13_image = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(stm32mp13_image)


def test_create_header():
    """Verify every populated field in the STM32 header version 2.0."""
    payload = bytes((0x01, 0x7F, 0x80, 0xFF))
    header = stm32mp13_image.create_header(payload, 0x2FFE0000, 0, 0x10)

    assert len(header) == 512
    assert header[0:4] == b"STM2"
    assert header[4:68] == bytes(64)
    assert struct.unpack_from("<I", header, 68)[0] == sum(payload)
    assert header[72:76] == bytes((0, 0, 2, 0))
    assert struct.unpack_from("<I", header, 76)[0] == len(payload)
    assert struct.unpack_from("<I", header, 80)[0] == 0x2FFE0000
    assert header[84:88] == bytes(4)
    assert struct.unpack_from("<I", header, 88)[0] == 0
    assert header[92:100] == bytes(8)
    assert struct.unpack_from("<I", header, 100)[0] == 1 << 31
    assert struct.unpack_from("<I", header, 104)[0] == 384
    assert struct.unpack_from("<I", header, 108)[0] == 0x10
    assert header[112:128] == bytes(16)
    assert header[128:132] == b"ST\xff\xff"
    assert struct.unpack_from("<I", header, 132)[0] == 384
    assert header[136:512] == bytes(376)


def test_no_arguments_prints_description(capsys):
    """Verify that invoking the tool without arguments explains its purpose."""
    stm32mp13_image.main([])

    output = capsys.readouterr().out
    assert "Add an unsigned 512-byte STM32 header version 2.0" in output
    assert "loaded by the STM32MP13 BootROM" in output
