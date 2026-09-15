#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for scripts/ci/twister_report_analyzer.py.

Run from the zephyr root::

    pytest scripts/tests/ci/test_twister_report_analyzer.py -v
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

import pytest

ZEPHYR_BASE = os.environ.get("ZEPHYR_BASE", str(Path(__file__).parents[3]))
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "ci"))

import twister_report_analyzer as tra  # noqa: E402, I001

LD_RETURNED = "/usr/bin/ld.bfd: error: ld returned 1 exit status"
OVERFLOWED = "/usr/bin/ld.bfd: region `FLASH' overflowed by 4096 bytes"
BUILD_STEP = "-- Performing build step for 'mcuboot'"
STDERR_TAIL = "west error: region `RAM' overflowed by 8 bytes"

TESTDATA_BUILD_INFO = [
    (
        [OVERFLOWED, LD_RETURNED],
        " at region FLASH",
    ),
    (
        [BUILD_STEP, OVERFLOWED, LD_RETURNED],
        " at region FLASH at build step for mcuboot",
    ),
    (
        [BUILD_STEP, "-- Completed 'mcuboot'", OVERFLOWED, LD_RETURNED],
        " at region FLASH",
    ),
    (
        ["main.c:1:1: error: nope is not a thing"],
        "",
    ),
    # The overflow is reported on the line before the linker error, so a
    # linker error on the first line has no line to read. Reading lines[i - 1]
    # there wraps around to the end of the log -- which twister fills with the
    # handler's stderr -- and returns before the real overflow further down.
    (
        [LD_RETURNED, BUILD_STEP, OVERFLOWED, LD_RETURNED, STDERR_TAIL],
        " at region FLASH at build step for mcuboot",
    ),
]


@pytest.mark.parametrize(
    "log_lines, expected",
    TESTDATA_BUILD_INFO,
    ids=[
        "overflow before linker error",
        "overflow inside a build step",
        "build step already completed",
        "not a linker failure",
        "linker error first, stderr overflow last",
    ],
)
def test_get_info_from_build_failure(log_lines, expected):
    reports = tra.TwisterReports()
    assert reports._get_info_from_build_failure("\n".join(log_lines)) == expected
