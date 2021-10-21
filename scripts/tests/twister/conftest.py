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
from twisterlib import TestRunner, TestRunner, TestInstance

def new_get_toolchain(*args, **kwargs):
    return 'zephyr'

TestSuite.get_toolchain = new_get_toolchain

@pytest.fixture(name='test_data')
def _test_data():
    """ Pytest fixture to load the test data directory"""
    data = ZEPHYR_BASE + "/scripts/tests/twister/test_data/"
    return data

@pytest.fixture(name='testcases_dir')
def testcases_directory():
    """ Pytest fixture to load the test data directory"""
    return ZEPHYR_BASE + "/scripts/tests/twister/test_data/testcases"

@pytest.fixture(name='class_testrunner')
def testrunner_obj(test_data, testcases_dir, tmpdir_factory):
    """ Pytest fixture to initialize and return the class TestRunner object"""
    board_root = test_data +"board_config/1_level/2_level/"
    testcase_root = [testcases_dir + '/tests', testcases_dir + '/samples']
    outdir = tmpdir_factory.mktemp("sanity_out_demo")
    runner = TestRunner(board_root, testcase_root, outdir)
    return runner

@pytest.fixture(name='all_scenarios_dict')
def testcases_dict(class_testrunner):
    """ Pytest fixture to call add_suites function of
	TestRunner class and return the dictionary of scenarios"""
    class_testrunner.SAMPLE_FILENAME = 'test_sample_app.yaml'
    class_testrunner.TESTS_FILENAME = 'test_data.yaml'
    class_testrunner.add_suites()
    return class_testrunner.suites

@pytest.fixture(name='platforms_list')
def all_platforms_list(test_data, class_testrunner):
    """ Pytest fixture to call add_configurations function of
	TestRunner class and return the Platforms list"""
    class_testrunner.board_roots = os.path.abspath(test_data + "board_config")
    runner = TestRunner(class_testrunner.board_roots, class_testrunner.roots, class_testrunner.outdir)
    runner.add_configurations()
    return runner.platforms

@pytest.fixture
def instances_fixture(class_testrunner, platforms_list, all_scenarios_dict, tmpdir_factory):
    """ Pytest fixture to call add_instances function of TestRunner class
    and return the instances dictionary"""
    class_testrunner.outdir = tmpdir_factory.mktemp("sanity_out_demo")
    class_testrunner.platforms = platforms_list
    platform = class_testrunner.get_platform("demo_board_2")
    instance_list = []
    for _, testcase in all_scenarios_dict.items():
        instance = TestInstance(testcase, platform, class_testrunner.outdir)
        instance_list.append(instance)
    class_testrunner.add_instances(instance_list)
    return class_testrunner.instances
