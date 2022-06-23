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
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts"))
from twisterlib.testplan import TestPlan
from twisterlib.testinstance import TestInstance
from twisterlib.enviornment import TwisterEnv, parse_arguments

def new_get_toolchain(*args, **kwargs):
    return 'zephyr'

TestPlan.get_toolchain = new_get_toolchain

@pytest.fixture(name='test_data')
def _test_data():
    """ Pytest fixture to load the test data directory"""
    data = ZEPHYR_BASE + "/scripts/tests/twister/test_data/"
    return data

@pytest.fixture(name='testsuites_dir')
def testsuites_directory():
    """ Pytest fixture to load the test data directory"""
    return ZEPHYR_BASE + "/scripts/tests/twister/test_data/testsuites"

@pytest.fixture(name='class_env')
def tesenv_obj(test_data, testsuites_dir, tmpdir_factory):
    """ Pytest fixture to initialize and return the class TestPlan object"""
    options = parse_arguments([])
    env = TwisterEnv(options)
    env.board_roots = [test_data +"board_config/1_level/2_level/"]
    env.test_roots = [testsuites_dir + '/tests', testsuites_dir + '/samples']
    env.outdir = tmpdir_factory.mktemp("sanity_out_demo")
    return env


@pytest.fixture(name='class_testplan')
def testplan_obj(test_data, class_env, testsuites_dir, tmpdir_factory):
    """ Pytest fixture to initialize and return the class TestPlan object"""
    env = class_env
    env.board_roots = [test_data +"board_config/1_level/2_level/"]
    env.test_roots = [testsuites_dir + '/tests', testsuites_dir + '/samples']
    env.outdir = tmpdir_factory.mktemp("sanity_out_demo")
    plan = TestPlan(env)
    return plan

@pytest.fixture(name='all_testsuites_dict')
def testsuites_dict(class_testplan):
    """ Pytest fixture to call add_testcase function of
	Testsuite class and return the dictionary of testsuites"""
    class_testplan.SAMPLE_FILENAME = 'test_sample_app.yaml'
    class_testplan.TESTSUITE_FILENAME = 'test_data.yaml'
    class_testplan.add_testsuites()
    return class_testplan.testsuites

@pytest.fixture(name='platforms_list')
def all_platforms_list(test_data, class_testplan):
    """ Pytest fixture to call add_configurations function of
	Testsuite class and return the Platforms list"""
    class_testplan.env.board_roots = [os.path.abspath(test_data + "board_config")]
    plan = TestPlan(class_testplan.env)
    plan.add_configurations()
    return plan.platforms

@pytest.fixture
def instances_fixture(class_testplan, platforms_list, all_testsuites_dict, tmpdir_factory):
    """ Pytest fixture to call add_instances function of Testsuite class
    and return the instances dictionary"""
    class_testplan.outdir = tmpdir_factory.mktemp("sanity_out_demo")
    class_testplan.platforms = platforms_list
    platform = class_testplan.get_platform("demo_board_2")
    instance_list = []
    for _, testcase in all_testsuites_dict.items():
        instance = TestInstance(testcase, platform, class_testplan.outdir)
        instance_list.append(instance)
    class_testplan.add_instances(instance_list)
    return class_testplan.instances
