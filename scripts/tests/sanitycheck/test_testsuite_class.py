#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=line-too-long
# pylint: disable=C0321
'''
This test file contains testcases for Testsuite class of sanitycheck
'''
import sys
import os
import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/sanity_chk"))

from sanitylib import TestCase, TestSuite, Platform

def test_testsuite_add_testcases(class_testsuite):
    """ Testing add_testcase function of Testsuite class in sanitycheck """
    # Test 1: Check the list of testcases after calling add testcases function is as expected
    class_testsuite.SAMPLE_FILENAME = 'test_sample_app.yaml'
    class_testsuite.TESTCASE_FILENAME = 'test_data.yaml'
    class_testsuite.add_testcases()
    tests_rel_dir = 'scripts/tests/sanitycheck/test_data/testcases/tests/'
    expected_testcases = ['test_b.check_1',
                          'test_b.check_2',
                          'test_c.check_1',
                          'test_c.check_2',
                          'test_a.check_1',
                          'test_a.check_2',
                          'sample_test.app']
    testcase_list = []
    for key in sorted(class_testsuite.testcases.keys()):
        testcase_list.append(os.path.basename(os.path.normpath(key)))
    assert sorted(testcase_list) == sorted(expected_testcases)

    # Test 2 : Assert Testcase name is expected & all the testcases values are testcase class objects
    testcase = class_testsuite.testcases.get(tests_rel_dir + 'test_a/test_a.check_1')
    assert testcase.name == tests_rel_dir + 'test_a/test_a.check_1'
    assert all(isinstance(n, TestCase) for n in class_testsuite.testcases.values())

@pytest.mark.parametrize("board_root_dir", [("board_config_file_not_exist"), ("board_config")])
def test_add_configurations(test_data, class_testsuite, board_root_dir):
    """ Testing add_configurations function of TestSuite class in Sanitycheck
    Test : Asserting on default platforms list
    """
    class_testsuite.board_roots = os.path.abspath(test_data + board_root_dir)
    suite = TestSuite(class_testsuite.board_roots, class_testsuite.roots, class_testsuite.outdir)
    if board_root_dir == "board_config":
        suite.add_configurations()
        assert sorted(suite.default_platforms) == sorted(['demo_board_1', 'demo_board_3'])
    elif board_root_dir == "board_config_file_not_exist":
        suite.add_configurations()
        assert sorted(suite.default_platforms) != sorted(['demo_board_1'])

def test_get_all_testcases(class_testsuite, all_testcases_dict):
    """ Testing get_all_testcases function of TestSuite class in Sanitycheck """
    class_testsuite.testcases = all_testcases_dict
    expected_tests = ['test_b.check_1', 'test_b.check_2', 'test_c.check_1', 'test_c.check_2', 'test_a.check_1', 'test_a.check_2', 'sample_test.app']
    assert len(class_testsuite.get_all_tests()) == 7
    assert sorted(class_testsuite.get_all_tests()) == sorted(expected_tests)

def test_get_toolchain(class_testsuite, monkeypatch, capsys):
    """ Testing get_toolchain function of TestSuite class in Sanitycheck
    Test 1 : Test toolchain returned by get_toolchain function is same as in the environment.
    Test 2 : Monkeypatch to delete the  ZEPHYR_TOOLCHAIN_VARIANT env var
    and check if appropriate error is raised"""
    monkeypatch.setenv("ZEPHYR_TOOLCHAIN_VARIANT", "zephyr")
    toolchain = class_testsuite.get_toolchain()
    assert toolchain in ["zephyr"]

    monkeypatch.delenv("ZEPHYR_TOOLCHAIN_VARIANT", raising=False)
    with pytest.raises(SystemExit):
        class_testsuite.get_toolchain()
    out, _ = capsys.readouterr()
    assert out == "E: Variable ZEPHYR_TOOLCHAIN_VARIANT is not defined\n"

def test_get_platforms(class_testsuite, platforms_list):
    """ Testing get_platforms function of TestSuite class in Sanitycheck """
    class_testsuite.platforms = platforms_list
    platform = class_testsuite.get_platform("demo_board_1")
    assert isinstance(platform, Platform)
    assert platform.name == "demo_board_1"
