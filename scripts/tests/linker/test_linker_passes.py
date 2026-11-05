# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Exercise the production link-pass predicate, including discovery's empty pass."""

import re
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]


@pytest.mark.parametrize(
    ("current", "rules", "expected"),
    [
        ("", "", 1),
        ("", "LINKER_ZEPHYR_PREBUILT;LINKER_ZEPHYR_FINAL", 0),
        ("", "NOT;LINKER_ZEPHYR_PREBUILT;LINKER_ZEPHYR_FINAL", 1),
        ("LINKER_ZEPHYR_PREBUILT", "LINKER_ZEPHYR_PREBUILT;LINKER_ZEPHYR_FINAL", 1),
        ("LINKER_ZEPHYR_FINAL", "LINKER_ZEPHYR_PREBUILT;LINKER_ZEPHYR_FINAL", 1),
        ("LINKER_ZEPHYR_PREBUILT;CUSTOM", "LINKER_ZEPHYR_PREBUILT", 1),
        ("LINKER_ZEPHYR_PREBUILT;CUSTOM", "NOT;LINKER_ZEPHYR_PREBUILT", 0),
        ("CUSTOM", "NOT;LINKER_ZEPHYR_PREBUILT;LINKER_ZEPHYR_FINAL", 1),
    ],
)
def test_link_pass_selection(tmp_path, current, rules, expected):
    cmake = shutil.which("cmake")
    if cmake is None:
        pytest.skip("CMake is required for link-pass predicate tests")
    source = (ROOT / "cmake/linker/linker_script_common.cmake").read_text()
    predicate = re.search(r"function\(is_active_in_pass .*?endfunction\(\)", source, re.S)
    assert predicate is not None
    script = tmp_path / "check-pass.cmake"
    script.write_text(
        "cmake_minimum_required(VERSION 3.20)\n"
        + predicate.group(0)
        + f'\nis_active_in_pass(selected "{current}" "{rules}")\n'
        + f'if(NOT selected EQUAL {expected})\n'
        + '  message(FATAL_ERROR "Incorrect link-pass selection: ${selected}")\n'
        + "endif()\n"
    )
    result = subprocess.run([cmake, "-P", str(script)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
