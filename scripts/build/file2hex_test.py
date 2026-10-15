# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import gzip
import random
import re
import subprocess
import sys
from pathlib import Path

import pytest


@pytest.mark.parametrize("compressed", [False, True], ids=["plain", "gzip"])
@pytest.mark.parametrize("output_format", ["list", "literal"])
@pytest.mark.parametrize(
    "size, offset, length",
    [
        pytest.param(3328, 11, 1, id="offset-one-byte"),
        pytest.param(3328, 0, 0, id="zero-length"),
        pytest.param(3328, 11, 0, id="zero-length-offset"),
        pytest.param(3328, 0, 1023, id="partial-chunk"),
        pytest.param(3328, 0, 1024, id="exact-chunk"),
        pytest.param(3328, 0, 1025, id="chunk-plus-one"),
        pytest.param(3328, 0, 2048, id="exact-two-chunks"),
        pytest.param(3328, 11, 1500, id="offset-partial-second-chunk"),
        pytest.param(3328, 0, 3328, id="reach-eof"),
        pytest.param(3328, 11, 3317, id="offset-reach-eof"),
        pytest.param(2048, 0, 2048, id="reach-eof-exact-chunks"),
        pytest.param(3328, 0, 5000, id="beyond-eof"),
        pytest.param(3328, 11, 4096, id="offset-beyond-eof"),
        pytest.param(3328, 0, None, id="default"),
        pytest.param(3328, 11, None, id="default-offset"),
        pytest.param(2048, 0, None, id="default-exact-chunks"),
        pytest.param(3328, 11, -1, id="explicit-unlimited"),
        pytest.param(0, 0, None, id="empty-file"),
        pytest.param(3328, 3328, 1, id="offset-at-eof"),
        pytest.param(3328, 4096, 1, id="offset-beyond-eof"),
    ],
)
def test_file2hex(tmp_path, size, offset, length, output_format, compressed):
    data = random.Random(size).randbytes(size)
    input_file = tmp_path / "input.bin"
    input_file.write_bytes(data)
    command = [
        sys.executable,
        str(Path(__file__).with_name("file2hex.py")),
        "--file",
        str(input_file),
        f"--offset={offset}",
        f"--format={output_format}",
    ]
    if length is not None:
        command.append(f"--length={length}")
    if compressed:
        command.append("--gzip")

    output = subprocess.run(command, capture_output=True, text=True, check=True).stdout
    if output_format == "list":
        for line in output.splitlines():
            assert re.fullmatch(r"(?:0x[0-9a-f]{2}, )*0x[0-9a-f]{2},", line)
        payload = bytes(int(value, 16) for value in re.findall(r"0x([0-9a-f]{2})", output))
    else:
        assert re.fullmatch(r'"(?:\\x[0-9a-f]{2})*"', output)
        payload = bytes.fromhex(output[1:-1].replace("\\x", ""))
    if compressed:
        payload = gzip.decompress(payload)

    expected = data[offset:] if length is None or length < 0 else data[offset : offset + length]
    assert payload == expected
