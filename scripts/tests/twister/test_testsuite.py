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

from twisterlib.testsuite import scan_file, ScanPathResult, TestSuite
from twisterlib.error import TwisterException

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


TESTDATA_4 = [
    (ZEPHYR_BASE, '.', 'test_c', 'Tests should reference the category and subsystem with a dot as a separator.'),
    (os.path.join(ZEPHYR_BASE, '/scripts/tests'), '.', '', 'Tests should reference the category and subsystem with a dot as a separator.'),
]
@pytest.mark.parametrize("testsuite_root, workdir, name, exception", TESTDATA_4)
def test_get_unique_exception(testsuite_root, workdir, name, exception):
    '''Test to check if tests reference the category and subsystem with a dot as a separator'''

    with pytest.raises(TwisterException):
        unique = TestSuite(testsuite_root, workdir, name)
        assert unique == exception

TESTDATA_3 = [
        (
            ZEPHYR_BASE + '/scripts/tests/twister/test_data/testsuites',
            ZEPHYR_BASE + '/scripts/tests/twister/test_data/testsuites/tests/test_a',
            '/scripts/tests/twister/test_data/testsuites/tests/test_a/test_a.check_1',
            '/scripts/tests/twister/test_data/testsuites/tests/test_a/test_a.check_1'
        ),
        (
            ZEPHYR_BASE,
            ZEPHYR_BASE,
            'test_a.check_1',
            'test_a.check_1'
        ),
        (
            ZEPHYR_BASE,
            ZEPHYR_BASE + '/scripts/tests/twister/test_data/testsuites/test_b',
            '/scripts/tests/twister/test_data/testsuites/test_b/test_b.check_1',
            '/scripts/tests/twister/test_data/testsuites/test_b/test_b.check_1'
        ),
        (
            os.path.join(ZEPHYR_BASE, 'scripts/tests'),
            os.path.join(ZEPHYR_BASE, 'scripts/tests'),
            'test_b.check_1',
            'scripts/tests/test_b.check_1'
        ),
        (
            ZEPHYR_BASE,
            ZEPHYR_BASE,
            'test_a.check_1.check_2',
            'test_a.check_1.check_2'
        ),
]
@pytest.mark.parametrize("testsuite_root, suite_path, name, expected", TESTDATA_3)
def test_get_unique(testsuite_root, suite_path, name, expected):
    '''Test to check if the unique name is given for each testsuite root and workdir'''
    suite = TestSuite(testsuite_root, suite_path, name)
    assert suite.name == expected

TESTDATA_2 = [
        (
            ZEPHYR_BASE + '/scripts/tests/twister/test_data/testsuites',
            ZEPHYR_BASE + '/scripts/tests/twister/test_data/testsuites/tests/test_a',
            'test_a.check_1',
            'test_a.check_1'
        ),
        (
            ZEPHYR_BASE,
            ZEPHYR_BASE,
            'test_a.check_1',
            'test_a.check_1'
        ),
        (
            ZEPHYR_BASE,
            ZEPHYR_BASE + '/scripts/tests/twister/test_data/testsuites/test_b',
            'test_b.check_1',
            'test_b.check_1'
        ),
        (
            os.path.join(ZEPHYR_BASE, 'scripts/tests'),
            os.path.join(ZEPHYR_BASE, 'scripts/tests'),
            'test_b.check_1',
            'test_b.check_1'
        ),
        (
            ZEPHYR_BASE,
            ZEPHYR_BASE,
            'test_a.check_1.check_2',
            'test_a.check_1.check_2'
        ),
]
@pytest.mark.parametrize("testsuite_root, suite_path, name, expected", TESTDATA_2)
def test_get_no_path_name(testsuite_root, suite_path, name, expected):
    '''Test to check if the name without path is given for each testsuite'''
    suite = TestSuite(testsuite_root, suite_path, name, no_path_name=True)
    print(suite.name)
    assert suite.name == expected
