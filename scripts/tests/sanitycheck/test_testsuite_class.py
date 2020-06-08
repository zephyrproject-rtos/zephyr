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

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/sanity_chk"))

from sanitylib import TestCase

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
