#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""
Tests for reports.py
"""

import json
from pathlib import Path
from unittest import mock

import pytest
from twisterlib.reports import JsonReport, ReportingJSONEncoder, ReportStatus


def test_report_status_is_its_value():
    """ReportStatus goes into XML attributes, so str() has to be the value."""
    assert str(ReportStatus.ERROR) == 'error'
    assert str(ReportStatus.FAIL) == 'failure'
    assert str(ReportStatus.SKIP) == 'skipped'
    # It is a str subclass, so it also compares equal to the bare string.
    assert ReportStatus.FAIL == 'failure'


def test_reporting_json_encoder_writes_paths_as_strings():
    encoded = json.dumps({'outdir': Path('a') / 'b'}, cls=ReportingJSONEncoder)
    assert json.loads(encoded) == {'outdir': str(Path('a') / 'b')}


def test_reporting_json_encoder_still_refuses_what_json_refuses():
    with pytest.raises(TypeError):
        json.dumps({'x': object()}, cls=ReportingJSONEncoder)


def test_process_log_returns_empty_for_a_missing_file(tmp_path):
    assert JsonReport.process_log(tmp_path / 'nothing-here.log') == ''


def test_process_log_drops_unprintable_bytes(tmp_path):
    """Terminal escapes and NULs go, printable characters stay.

    Decoding happens first and is strict UTF-8, so what this pins is the
    filtering of bytes that decode, not tolerance of bytes that do not.
    """
    log = tmp_path / 'build.log'
    log.write_bytes(b'before\x1b[31mafter\x00end\n')

    assert JsonReport.process_log(log) == 'before[31mafterend\n'


TESTDATA_CMAKE_FAILURE = [
    (
        ['warning: the FOO symbol is deprecated', 'error: Aborting due to Kconfig warnings'],
        'warning: the FOO symbol is deprecated',
    ),
    (
        ['warning: undefined symbol: FOO', 'error: Aborting due to Kconfig warnings'],
        'undefined symbol: FOO',
    ),
    (
        ['error: Aborting due to Kconfig warnings'],
        'no warning found',
    ),
    (
        ['main.c:1:10: fatal error: nope.h: No such file or directory'],
        'fatal error: nope.h: No such file or directory',
    ),
    (
        ['devicetree error: /soc/uart@0: bad node'],
        'devicetree error',
    ),
    (
        ['CMake Error at cmake/modules/x.cmake:1 (message):', '', '  the reason'],
        'CMake Error at cmake/modules/x.cmake:1 (message):   the reason',
    ),
    (
        ['CMake Error at cmake/modules/x.cmake:1 (message):'],
        'CMake Error at cmake/modules/x.cmake:1 (message):',
    ),
    (
        ['[1/2] Building C object main.c.obj', '[2/2] Linking C executable zephyr.elf'],
        None,
    ),
]


@pytest.mark.parametrize(
    'lines, expected',
    TESTDATA_CMAKE_FAILURE,
    ids=[
        'warning kept',
        'undefined symbol',
        'no warning',
        'fatal error',
        'devicetree',
        'cmake error with detail',
        'cmake error alone',
        'nothing to report',
    ],
)
def test_parse_cmake_build_failure(lines, expected):
    assert JsonReport._parse_cmake_build_failure('\n'.join(lines)) == expected


LD_RETURNED = '/usr/bin/ld.bfd: error: ld returned 1 exit status'
OVERFLOWED = "ld.bfd: region `FLASH' overflowed by 4096 bytes"
LD_WARNING = '/usr/bin/ld.bfd: warning: something to say'

TESTDATA_BUILD_FAILURE = [
    (
        ["main.c:(.text+0x0): undefined reference to `missing_symbol'", LD_RETURNED],
        "undefined reference to `missing_symbol'",
    ),
    (
        [OVERFLOWED, LD_RETURNED],
        'ld.bfd: region overflowed',
    ),
    (
        [LD_WARNING, LD_RETURNED],
        'ld.bfd: warning: something to say',
    ),
    (
        ["main.c: in function `main':", LD_RETURNED],
        "in function `main':",
    ),
    # Both could decide it: last_warning is checked first, so the function
    # name wins over the region the line before the linker error names.
    (
        ["main.c: in function `main':", OVERFLOWED, LD_RETURNED],
        "in function `main':",
    ),
    (
        ['main.c:1:1: error: nope is not a thing'],
        'error: nope is not a thing',
    ),
    # The line before the linker error is what decides the reason, so when the
    # linker error is the first line there is no line to decide it -- including
    # when the log ends with text that would have decided it. reports.py
    # appends the handler's stderr to the build log before parsing, so the last
    # line can come from a different file entirely.
    (
        [LD_RETURNED, 'ninja: build stopped.', OVERFLOWED],
        LD_RETURNED,
    ),
    (
        [LD_RETURNED, 'ninja: build stopped.', LD_WARNING],
        LD_RETURNED,
    ),
    (
        [LD_RETURNED],
        LD_RETURNED,
    ),
    (
        ['[1/2] Building C object main.c.obj'],
        None,
    ),
]


@pytest.mark.parametrize(
    'lines, expected',
    TESTDATA_BUILD_FAILURE,
    ids=[
        'undefined reference',
        'overflow before',
        'ld warning before',
        'in function before',
        'in function beats overflow',
        'plain error',
        'linker error first, overflow last',
        'linker error first, warning last',
        'linker error alone',
        'nothing to report',
    ],
)
def test_parse_build_failure(lines, expected):
    assert JsonReport._parse_build_failure('\n'.join(lines)) == expected


TESTDATA_DETAILED_REASON = [
    (
        'CMake build failure',
        ['main.c:1:10: fatal error: nope.h: No such file or directory'],
        'CMake build failure - fatal error: nope.h: No such file or directory',
    ),
    (
        'Build failure',
        ['main.c:1:1: error: nope is not a thing'],
        'Build failure - error: nope is not a thing',
    ),
    # A reason the parsers say nothing about comes back untouched.
    ('CMake build failure', ['[1/2] Building C object main.c.obj'], 'CMake build failure'),
    ('Build failure', ['[1/2] Building C object main.c.obj'], 'Build failure'),
    ('Timeout', ['main.c:1:1: error: nope is not a thing'], 'Timeout'),
]


@pytest.mark.parametrize(
    'reason, lines, expected',
    TESTDATA_DETAILED_REASON,
    ids=[
        'cmake detailed',
        'build detailed',
        'cmake plain',
        'build plain',
        'other reason untouched',
    ],
)
def test_get_detailed_reason(reason, lines, expected):
    report = JsonReport(mock.Mock(), {})
    assert report.get_detailed_reason(reason, '\n'.join(lines)) == expected
