#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''Common fixtures for use in testing the twister tool.'''
import logging
import os
import sys
import pytest

pytest_plugins = ["pytester"]
logging.getLogger("twister").setLevel(logging.DEBUG)  # requires for testing twister

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts"))
from twisterlib.testplan import TestPlan, TestConfiguration
from twisterlib.testinstance import TestInstance
from twisterlib.environment import TwisterEnv, add_parse_arguments, parse_arguments

def new_get_toolchain(*args, **kwargs):
    return 'zephyr'

TestPlan.get_toolchain = new_get_toolchain

@pytest.fixture(name='test_data')
def _test_data():
    """ Pytest fixture to load the test data directory"""
    data = ZEPHYR_BASE + "/scripts/tests/twister/test_data/"
    return data

@pytest.fixture(name='zephyr_base')
def zephyr_base_directory():
    return ZEPHYR_BASE

@pytest.fixture(name='testsuites_dir')
def testsuites_directory():
    """ Pytest fixture to load the test data directory"""
    return ZEPHYR_BASE + "/scripts/tests/twister/test_data/testsuites"

@pytest.fixture(name='class_env')
def tesenv_obj(test_data, testsuites_dir, tmpdir_factory):
    """ Pytest fixture to initialize and return the class TestPlan object"""
    parser = add_parse_arguments()
    options = parse_arguments(parser, [])
    options.detailed_test_id = True
    env = TwisterEnv(options)
    env.board_roots = [os.path.join(test_data, "board_config", "1_level", "2_level")]
    env.test_roots = [os.path.join(testsuites_dir, 'tests'),
                      os.path.join(testsuites_dir, 'samples')]
    env.test_config = os.path.join(test_data, "test_config.yaml")
    env.outdir = tmpdir_factory.mktemp("sanity_out_demo")
    return env


@pytest.fixture(name='class_testplan')
def testplan_obj(class_env):
    """ Pytest fixture to initialize and return the class TestPlan object"""
    env = class_env
    plan = TestPlan(env)
    plan.test_config = TestConfiguration(config_file=env.test_config)
    plan.options.outdir = env.outdir
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
    class_testplan.env.board_roots = [os.path.abspath(os.path.join(test_data, "board_config"))]
    plan = TestPlan(class_testplan.env)
    plan.test_config = TestConfiguration(config_file=class_testplan.env.test_config)
    plan.add_configurations()
    return plan.platforms

@pytest.fixture
def instances_fixture(class_testplan, platforms_list, all_testsuites_dict):
    """ Pytest fixture to call add_instances function of Testsuite class
    and return the instances dictionary"""
    class_testplan.platforms = platforms_list
    platform = class_testplan.get_platform("demo_board_2")
    instance_list = []
    for _, testcase in all_testsuites_dict.items():
        instance = TestInstance(testcase, platform, 'zephyr', class_testplan.env.outdir)
        instance_list.append(instance)
    class_testplan.add_instances(instance_list)
    return class_testplan.instances
