#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for testinstance class
"""

import os
import sys
import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

from twisterlib.testsuite import scan_file, ScanPathResult

TESTDATA_5 = [
    ("testsuites/tests/test_ztest.c",
     ScanPathResult(
         warnings=None,
         matches=['a', 'c', 'unit_a',
                  'newline',
                  'test_test_aa',
                  'user', 'last'],
         has_registered_test_suites=False,
         has_run_registered_test_suites=False,
         has_test_main=False,
         ztest_suite_names = ["test_api"])),
    ("testsuites/tests/test_a/test_ztest_error.c",
     ScanPathResult(
         warnings="Found a test that does not start with test_",
         matches=['1a', '1c', '2a', '2b'],
         has_registered_test_suites=False,
         has_run_registered_test_suites=False,
         has_test_main=True,
         ztest_suite_names = ["feature1", "feature2"])),
    ("testsuites/tests/test_a/test_ztest_error_1.c",
     ScanPathResult(
         warnings="found invalid #ifdef, #endif in ztest_test_suite()",
         matches=['unit_1a', 'unit_1b', 'Unit_1c'],
         has_registered_test_suites=False,
         has_run_registered_test_suites=False,
         has_test_main=False,
         ztest_suite_names = ["feature3"])),
    ("testsuites/tests/test_d/test_ztest_error_register_test_suite.c",
     ScanPathResult(
         warnings=None, matches=['unit_1a', 'unit_1b'],
         has_registered_test_suites=True,
         has_run_registered_test_suites=False,
         has_test_main=False,
         ztest_suite_names = ["feature4"])),
]

@pytest.mark.parametrize("test_file, expected", TESTDATA_5)
def test_scan_file(test_data, test_file, class_env, expected: ScanPathResult):
    '''Testing scan_file method with different ztest files for warnings and results'''

    result: ScanPathResult = scan_file(os.path.join(test_data, test_file))
    assert result == expected
