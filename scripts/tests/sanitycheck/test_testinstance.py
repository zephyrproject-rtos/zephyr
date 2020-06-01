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
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/sanity_chk"))
from sanitylib import TestInstance, BuildError


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
    testcase = class_testsuite.testcases.get('scripts/tests/sanitycheck/test_data/testcases/tests/test_a/test_a.check_1')

    class_testsuite.platforms = platforms_list
    platform = class_testsuite.get_platform("demo_board_2")
    platform.type = platform_type
    platform.simulation = platform_sim
    testcase.harness = harness
    testcase.build_only = build_only
    testcase.slow = slow

    testinstance = TestInstance(testcase, platform, class_testsuite.outdir)
    testinstance.check_build_or_run(build_only, slow, device_testing, fixture)
    b, r = expected
    assert testinstance.build_only == b
    assert testinstance.run == r

    monkeypatch.setattr("os.name", "nt")
    testinstance.check_build_or_run()
    assert testinstance.build_only and not testinstance.run

TESTDATA_2 = [
    (True, True, ["demo_board_2"], "native", '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y\nCONFIG_ASAN=y'),
    (False, True, ["demo_board_2"], 'native', '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y'),
    (True, True, ["demo_board_2"], 'mcu', '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y'),
    (False, False, ["demo_board_2"], 'native', ''),
    (False, True, ['demo_board_1'], 'native', ''),
    (True, False, ["demo_board_2"], 'native', '\nCONFIG_ASAN=y'),
]

@pytest.mark.parametrize("enable_asan, enable_coverage, coverage_platform, platform_type, expected_content", TESTDATA_2)
def test_create_overlay(class_testsuite, all_testcases_dict, platforms_list, test_data, enable_asan, enable_coverage, coverage_platform, platform_type, expected_content):
    """Test correct content is written to testcase_extra.conf based on if conditions
    TO DO: Add extra_configs to the input list"""
    class_testsuite.testcases = all_testcases_dict
    testcase = class_testsuite.testcases.get('scripts/tests/sanitycheck/test_data/testcases/samples/test_app/sample_test.app')
    class_testsuite.platforms = platforms_list
    platform = class_testsuite.get_platform("demo_board_2")

    testinstance = TestInstance(testcase, platform, class_testsuite.outdir)
    platform.type = platform_type
    assert testinstance.create_overlay(platform, enable_asan, enable_coverage, coverage_platform) == expected_content

def test_calculate_sizes(class_testsuite, all_testcases_dict, platforms_list):
    """ Test Calculate sizes method for zephyr elf"""
    class_testsuite.testcases = all_testcases_dict
    testcase = class_testsuite.testcases.get('scripts/tests/sanitycheck/test_data/testcases/samples/test_app/sample_test.app')
    class_testsuite.platforms = platforms_list
    platform = class_testsuite.get_platform("demo_board_2")
    testinstance = TestInstance(testcase, platform, class_testsuite.outdir)

    with pytest.raises(BuildError):
        assert testinstance.calculate_sizes() == "Missing/multiple output ELF binary"
