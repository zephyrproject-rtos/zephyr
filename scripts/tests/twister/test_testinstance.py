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
from twisterlib.testinstance import TestInstance
from twisterlib.error import BuildError
from twisterlib.runner import TwisterRunner
from expr_parser import reserved


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
def test_check_build_or_run(class_testplan, monkeypatch, all_testsuites_dict, platforms_list, build_only, slow, harness, platform_type, platform_sim, device_testing, fixture, expected):
    """" Test to check the conditions for build_only and run scenarios
    Scenario 1: Test when different parameters are passed, build_only and run are set correctly
    Scenario 2: Test if build_only is enabled when the OS is Windows"""

    class_testplan.testsuites = all_testsuites_dict
    testsuite = class_testplan.testsuites.get('scripts/tests/twister/test_data/testsuites/tests/test_a/test_a.check_1')
    print(testsuite)

    class_testplan.platforms = platforms_list
    platform = class_testplan.get_platform("demo_board_2")
    platform.type = platform_type
    platform.simulation = platform_sim
    testsuite.harness = harness
    testsuite.build_only = build_only
    testsuite.slow = slow

    testinstance = TestInstance(testsuite, platform, class_testplan.env.outdir)
    run = testinstance.check_runnable(slow, device_testing, fixture)
    _, r = expected
    assert run == r

    monkeypatch.setattr("os.name", "nt")
    run = testinstance.check_runnable()
    assert not run

TESTDATA_2 = [
    (True, True, True, ["demo_board_2"], "native", None, '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y\nCONFIG_ASAN=y\nCONFIG_UBSAN=y'),
    (True, False, True, ["demo_board_2"], "native", None, '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y\nCONFIG_ASAN=y'),
    (False, False, True, ["demo_board_2"], 'native', None, '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y'),
    (True, False, True, ["demo_board_2"], 'mcu', None, '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y'),
    (False, False, False, ["demo_board_2"], 'native', None, ''),
    (False, False, True, ['demo_board_1'], 'native', None, ''),
    (True, False, False, ["demo_board_2"], 'native', None, '\nCONFIG_ASAN=y'),
    (False, True, False, ["demo_board_2"], 'native', None, '\nCONFIG_UBSAN=y'),
    (False, False, False, ["demo_board_2"], 'native', ["CONFIG_LOG=y"], 'CONFIG_LOG=y'),
    (False, False, False, ["demo_board_2"], 'native', ["arch:x86_demo:CONFIG_LOG=y"], 'CONFIG_LOG=y'),
    (False, False, False, ["demo_board_2"], 'native', ["arch:arm_demo:CONFIG_LOG=y"], ''),
    (False, False, False, ["demo_board_2"], 'native', ["platform:demo_board_2:CONFIG_LOG=y"], 'CONFIG_LOG=y'),
    (False, False, False, ["demo_board_2"], 'native', ["platform:demo_board_1:CONFIG_LOG=y"], ''),
]

@pytest.mark.parametrize("enable_asan, enable_ubsan, enable_coverage, coverage_platform, platform_type, extra_configs, expected_content", TESTDATA_2)
def test_create_overlay(class_testplan, all_testsuites_dict, platforms_list, enable_asan, enable_ubsan, enable_coverage, coverage_platform, platform_type, extra_configs, expected_content):
    """Test correct content is written to testcase_extra.conf based on if conditions."""
    class_testplan.testsuites = all_testsuites_dict
    testcase = class_testplan.testsuites.get('scripts/tests/twister/test_data/testsuites/samples/test_app/sample_test.app')

    if extra_configs:
        testcase.extra_configs = extra_configs

    class_testplan.platforms = platforms_list
    platform = class_testplan.get_platform("demo_board_2")

    testinstance = TestInstance(testcase, platform, class_testplan.env.outdir)
    platform.type = platform_type
    assert testinstance.create_overlay(platform, enable_asan, enable_ubsan, enable_coverage, coverage_platform) == expected_content

def test_calculate_sizes(class_testplan, all_testsuites_dict, platforms_list):
    """ Test Calculate sizes method for zephyr elf"""
    class_testplan.testsuites = all_testsuites_dict
    testcase = class_testplan.testsuites.get('scripts/tests/twister/test_data/testsuites/samples/test_app/sample_test.app')
    class_testplan.platforms = platforms_list
    platform = class_testplan.get_platform("demo_board_2")
    testinstance = TestInstance(testcase, platform, class_testplan.env.outdir)

    with pytest.raises(BuildError):
        assert testinstance.calculate_sizes() == "Missing/multiple output ELF binary"

TESTDATA_3 = [
    ('CONFIG_ARCH_HAS_THREAD_LOCAL_STORAGE and CONFIG_TOOLCHAIN_SUPPORTS_THREAD_LOCAL_STORAGE and not (CONFIG_TOOLCHAIN_ARCMWDT_SUPPORTS_THREAD_LOCAL_STORAGE and CONFIG_USERSPACE)', ['kconfig']),
    ('(dt_compat_enabled("st,stm32-flash-controller") or dt_compat_enabled("st,stm32h7-flash-controller")) and dt_label_with_parent_compat_enabled("storage_partition", "fixed-partitions")', ['dts']),
    ('((CONFIG_FLASH_HAS_DRIVER_ENABLED and not CONFIG_TRUSTED_EXECUTION_NONSECURE) and dt_label_with_parent_compat_enabled("storage_partition", "fixed-partitions")) or (CONFIG_FLASH_HAS_DRIVER_ENABLED and CONFIG_TRUSTED_EXECUTION_NONSECURE and dt_label_with_parent_compat_enabled("slot1_ns_partition", "fixed-partitions"))', ['dts', 'kconfig']),
    ('((CONFIG_CPU_AARCH32_CORTEX_R or CONFIG_CPU_CORTEX_M) and CONFIG_CPU_HAS_FPU and TOOLCHAIN_HAS_NEWLIB == 1) or CONFIG_ARCH_POSIX', ['full'])
]

@pytest.mark.parametrize("filter_expr, expected_stages", TESTDATA_3)
def test_which_filter_stages(filter_expr, expected_stages):
    logic_keys = reserved.keys()
    stages = TwisterRunner.get_cmake_filter_stages(filter_expr, logic_keys)
    assert sorted(stages) == sorted(expected_stages)
