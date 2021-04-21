#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=line-too-long
"""
Tests for testinstance class
"""

import os
import sys
import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))
from twisterlib import TestInstance, BuildError, TestCase, TwisterException


TESTDATA_1 = [
    (False, False, "console", "na", "qemu", False, [], (False, True)),
    (False, False, "console", "native", "qemu", False, [], (False, True)),
    (True, False, "console", "native", "nsim", False, [], (True, False)),
    (True, True, "console", "native", "renode", False, [], (True, False)),
    (False, False, "sensor", "native", "", False, [], (True, False)),
    (False, False, "sensor", "na", "", False, [], (True, False)),
    (False, True, "sensor", "native", "", True, [], (True, False)),
]
@pytest.mark.parametrize("build_only, slow, harness, platform_type, platform_sim, device_testing,fixture, expected", TESTDATA_1)
def test_check_build_or_run(class_testsuite, monkeypatch, all_testcases_dict, platforms_list, build_only, slow, harness, platform_type, platform_sim, device_testing, fixture, expected):
    """" Test to check the conditions for build_only and run scenarios
    Scenario 1: Test when different parameters are passed, build_only and run are set correctly
    Sceanrio 2: Test if build_only is enabled when the OS is Windows"""

    class_testsuite.testcases = all_testcases_dict
    testcase = class_testsuite.testcases.get('scripts/tests/twister/test_data/testcases/tests/test_a/test_a.check_1')

    class_testsuite.platforms = platforms_list
    platform = class_testsuite.get_platform("demo_board_2")
    platform.type = platform_type
    platform.simulation = platform_sim
    testcase.harness = harness
    testcase.build_only = build_only
    testcase.slow = slow

    testinstance = TestInstance(testcase, platform, class_testsuite.outdir)
    run = testinstance.check_runnable(slow, device_testing, fixture)
    _, r = expected
    assert run == r

    monkeypatch.setattr("os.name", "nt")
    run = testinstance.check_runnable()
    assert not run

TESTDATA_2 = [
    (True, True, True, ["demo_board_2"], "native", '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y\nCONFIG_ASAN=y\nCONFIG_UBSAN=y'),
    (True, False, True, ["demo_board_2"], "native", '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y\nCONFIG_ASAN=y'),
    (False, False, True, ["demo_board_2"], 'native', '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y'),
    (True, False, True, ["demo_board_2"], 'mcu', '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y'),
    (False, False, False, ["demo_board_2"], 'native', ''),
    (False, False, True, ['demo_board_1'], 'native', ''),
    (True, False, False, ["demo_board_2"], 'native', '\nCONFIG_ASAN=y'),
    (False, True, False, ["demo_board_2"], 'native', '\nCONFIG_UBSAN=y'),
]

@pytest.mark.parametrize("enable_asan, enable_ubsan, enable_coverage, coverage_platform, platform_type, expected_content", TESTDATA_2)
def test_create_overlay(class_testsuite, all_testcases_dict, platforms_list, enable_asan, enable_ubsan, enable_coverage, coverage_platform, platform_type, expected_content):
    """Test correct content is written to testcase_extra.conf based on if conditions
    TO DO: Add extra_configs to the input list"""
    class_testsuite.testcases = all_testcases_dict
    testcase = class_testsuite.testcases.get('scripts/tests/twister/test_data/testcases/samples/test_app/sample_test.app')
    class_testsuite.platforms = platforms_list
    platform = class_testsuite.get_platform("demo_board_2")

    testinstance = TestInstance(testcase, platform, class_testsuite.outdir)
    platform.type = platform_type
    assert testinstance.create_overlay(platform, enable_asan, enable_ubsan, enable_coverage, coverage_platform) == expected_content

def test_calculate_sizes(class_testsuite, all_testcases_dict, platforms_list):
    """ Test Calculate sizes method for zephyr elf"""
    class_testsuite.testcases = all_testcases_dict
    testcase = class_testsuite.testcases.get('scripts/tests/twister/test_data/testcases/samples/test_app/sample_test.app')
    class_testsuite.platforms = platforms_list
    platform = class_testsuite.get_platform("demo_board_2")
    testinstance = TestInstance(testcase, platform, class_testsuite.outdir)

    with pytest.raises(BuildError):
        assert testinstance.calculate_sizes() == "Missing/multiple output ELF binary"

TESTDATA_3 = [
    (ZEPHYR_BASE + '/scripts/tests/twister/test_data/testcases', ZEPHYR_BASE, '/scripts/tests/twister/test_data/testcases/tests/test_a/test_a.check_1', '/scripts/tests/twister/test_data/testcases/tests/test_a/test_a.check_1'),
    (ZEPHYR_BASE, '.', 'test_a.check_1', 'test_a.check_1'),
    (ZEPHYR_BASE, '/scripts/tests/twister/test_data/testcases/test_b', 'test_b.check_1', '/scripts/tests/twister/test_data/testcases/test_b/test_b.check_1'),
    (os.path.join(ZEPHYR_BASE, '/scripts/tests'), '.', 'test_b.check_1', 'test_b.check_1'),
    (os.path.join(ZEPHYR_BASE, '/scripts/tests'), '.', '.', '.'),
    (ZEPHYR_BASE, '.', 'test_a.check_1.check_2', 'test_a.check_1.check_2'),
]
@pytest.mark.parametrize("testcase_root, workdir, name, expected", TESTDATA_3)
def test_get_unique(testcase_root, workdir, name, expected):
    '''Test to check if the unique name is given for each testcase root and workdir'''
    unique = TestCase(testcase_root, workdir, name)
    assert unique.name == expected

TESTDATA_4 = [
    (ZEPHYR_BASE, '.', 'test_c', 'Tests should reference the category and subsystem with a dot as a separator.'),
    (os.path.join(ZEPHYR_BASE, '/scripts/tests'), '.', '', 'Tests should reference the category and subsystem with a dot as a separator.'),
]
@pytest.mark.parametrize("testcase_root, workdir, name, exception", TESTDATA_4)
def test_get_unique_exception(testcase_root, workdir, name, exception):
    '''Test to check if tests reference the category and subsystem with a dot as a separator'''

    with pytest.raises(TwisterException):
        unique = TestCase(testcase_root, workdir, name)
        assert unique == exception

TESTDATA_5 = [
    ("testcases/tests/test_ztest.c", None, ['a', 'c', 'unit_a', 'newline', 'test_test_aa', 'user', 'last']),
    ("testcases/tests/test_a/test_ztest_error.c", "Found a test that does not start with test_", ['1a', '1c', '2a', '2b']),
    ("testcases/tests/test_a/test_ztest_error_1.c", "found invalid #ifdef, #endif in ztest_test_suite()", ['unit_1a', 'unit_1b', 'Unit_1c']),
]

@pytest.mark.parametrize("test_file, expected_warnings, expected_subcases", TESTDATA_5)
def test_scan_file(test_data, test_file, expected_warnings, expected_subcases):
    '''Testing scan_file method with different ztest files for warnings and results'''

    testcase = TestCase("/scripts/tests/twister/test_data/testcases/tests", ".", "test_a.check_1")

    results, warnings = testcase.scan_file(os.path.join(test_data, test_file))
    assert sorted(results) == sorted(expected_subcases)
    assert warnings == expected_warnings


TESTDATA_6 = [
    ("testcases/tests", ['a', 'c', 'unit_a', 'newline', 'test_test_aa', 'user', 'last']),
    ("testcases/tests/test_a", ['unit_1a', 'unit_1b', 'Unit_1c', '1a', '1c', '2a', '2b']),
]

@pytest.mark.parametrize("test_path, expected_subcases", TESTDATA_6)
def test_subcases(test_data, test_path, expected_subcases):
    '''Testing scan path and parse subcases methods for expected subcases'''
    testcase = TestCase("/scripts/tests/twister/test_data/testcases/tests", ".", "test_a.check_1")

    subcases = testcase.scan_path(os.path.join(test_data, test_path))
    assert sorted(subcases) == sorted(expected_subcases)

    testcase.id = "test_id"
    testcase.parse_subcases(test_data + test_path)
    assert sorted(testcase.cases) == [testcase.id + '.' + x for x in sorted(expected_subcases)]
