#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''Common fixtures for use in testing the twister tool.'''

import os
import sys
import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))
from twisterlib import TestSuite, TestInstance

@pytest.fixture(name='test_data')
def _test_data():
    """ Pytest fixture to load the test data directory"""
    data = ZEPHYR_BASE + "/scripts/tests/twister/test_data/"
    return data

@pytest.fixture(name='testcases_dir')
def testcases_directory():
    """ Pytest fixture to load the test data directory"""
    return ZEPHYR_BASE + "/scripts/tests/twister/test_data/testcases"

@pytest.fixture(name='class_testsuite')
def testsuite_obj(test_data, testcases_dir, tmpdir_factory):
    """ Pytest fixture to initialize and return the class TestSuite object"""
    board_root = test_data +"board_config/1_level/2_level/"
    testcase_root = [testcases_dir + '/tests', testcases_dir + '/samples']
    outdir = tmpdir_factory.mktemp("sanity_out_demo")
    suite = TestSuite(board_root, testcase_root, outdir)
    return suite

@pytest.fixture(name='all_testcases_dict')
def testcases_dict(class_testsuite):
    """ Pytest fixture to call add_testcase function of
	Testsuite class and return the dictionary of testcases"""
    class_testsuite.SAMPLE_FILENAME = 'test_sample_app.yaml'
    class_testsuite.TESTCASE_FILENAME = 'test_data.yaml'
    class_testsuite.add_testcases()
    return class_testsuite.testcases

@pytest.fixture(name='platforms_list')
def all_platforms_list(test_data, class_testsuite):
    """ Pytest fixture to call add_configurations function of
	Testsuite class and return the Platforms list"""
    class_testsuite.board_roots = os.path.abspath(test_data + "board_config")
    suite = TestSuite(class_testsuite.board_roots, class_testsuite.roots, class_testsuite.outdir)
    suite.add_configurations()
    return suite.platforms

@pytest.fixture
def instances_fixture(class_testsuite, platforms_list, all_testcases_dict, tmpdir_factory):
    """ Pytest fixture to call add_instances function of Testsuite class
    and return the instances dictionary"""
    class_testsuite.outdir = tmpdir_factory.mktemp("sanity_out_demo")
    class_testsuite.platforms = platforms_list
    platform = class_testsuite.get_platform("demo_board_2")
    instance_list = []
    for _, testcase in all_testcases_dict.items():
        instance = TestInstance(testcase, platform, class_testsuite.outdir)
        instance_list.append(instance)
    class_testsuite.add_instances(instance_list)
    return class_testsuite.instances
