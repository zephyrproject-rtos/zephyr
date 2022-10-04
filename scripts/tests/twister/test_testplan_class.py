#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''
This test file contains testsuites for Testsuite class of twister
'''
import sys
import os
import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

from twisterlib.testplan import TestPlan
from twisterlib.testinstance import TestInstance
from twisterlib.testsuite import TestSuite
from twisterlib.platform import Platform


def test_testplan_add_testsuites(class_testplan):
    """ Testing add_testcase function of Testsuite class in twister """
    # Test 1: Check the list of testsuites after calling add testsuites function is as expected
    class_testplan.SAMPLE_FILENAME = 'test_sample_app.yaml'
    class_testplan.TESTSUITE_FILENAME = 'test_data.yaml'
    class_testplan.add_testsuites()

    tests_rel_dir = 'scripts/tests/twister/test_data/testsuites/tests/'
    expected_testsuites = ['test_b.check_1',
                          'test_b.check_2',
                          'test_c.check_1',
                          'test_c.check_2',
                          'test_a.check_1',
                          'test_a.check_2',
                          'test_d.check_1',
                          'sample_test.app']
    testsuite_list = []
    for key in sorted(class_testplan.testsuites.keys()):
        testsuite_list.append(os.path.basename(os.path.normpath(key)))
    assert sorted(testsuite_list) == sorted(expected_testsuites)

    # Test 2 : Assert Testcase name is expected & all testsuites values are testcase class objects
    suite = class_testplan.testsuites.get(tests_rel_dir + 'test_a/test_a.check_1')
    assert suite.name == tests_rel_dir + 'test_a/test_a.check_1'
    assert all(isinstance(n, TestSuite) for n in class_testplan.testsuites.values())

@pytest.mark.parametrize("board_root_dir", [("board_config_file_not_exist"), ("board_config")])
def test_add_configurations(test_data, class_env, board_root_dir):
    """ Testing add_configurations function of TestPlan class in Twister
    Test : Asserting on default platforms list
    """
    class_env.board_roots = [os.path.abspath(test_data + board_root_dir)]
    plan = TestPlan(class_env)
    if board_root_dir == "board_config":
        plan.add_configurations()
        assert sorted(plan.default_platforms) == sorted(['demo_board_1', 'demo_board_3'])
    elif board_root_dir == "board_config_file_not_exist":
        plan.add_configurations()
        assert sorted(plan.default_platforms) != sorted(['demo_board_1'])


def test_get_all_testsuites(class_env, all_testsuites_dict):
    """ Testing get_all_testsuites function of TestPlan class in Twister """
    plan = TestPlan(class_env)
    plan.testsuites = all_testsuites_dict
    expected_tests = ['sample_test.app', 'test_a.check_1.1a',
                      'test_a.check_1.1c',
                      'test_a.check_1.2a', 'test_a.check_1.2b',
                      'test_a.check_1.Unit_1c', 'test_a.check_1.unit_1a',
                      'test_a.check_1.unit_1b', 'test_a.check_2.1a',
                      'test_a.check_2.1c', 'test_a.check_2.2a',
                      'test_a.check_2.2b', 'test_a.check_2.Unit_1c',
                      'test_a.check_2.unit_1a', 'test_a.check_2.unit_1b',
                      'test_b.check_1', 'test_b.check_2', 'test_c.check_1',
                      'test_c.check_2', 'test_d.check_1.unit_1a',
                      'test_d.check_1.unit_1b']
    tests = plan.get_all_tests()
    result = [c.name for c in tests]
    assert len(plan.get_all_tests()) == len(expected_tests)
    assert sorted(result) == sorted(expected_tests)

def test_get_platforms(class_env, platforms_list):
    """ Testing get_platforms function of TestPlan class in Twister """
    plan = TestPlan(class_env)
    plan.platforms = platforms_list
    platform = plan.get_platform("demo_board_1")
    assert isinstance(platform, Platform)
    assert platform.name == "demo_board_1"

TESTDATA_PART1 = [
    ("toolchain_allow", ['gcc'], None, None, "Not in testsuite toolchain allow list"),
    ("platform_allow", ['demo_board_1'], None, None, "Not in testsuite platform allow list"),
    ("toolchain_exclude", ['zephyr'], None, None, "In test case toolchain exclude"),
    ("platform_exclude", ['demo_board_2'], None, None, "In test case platform exclude"),
    ("arch_exclude", ['x86_demo'], None, None, "In test case arch exclude"),
    ("arch_allow", ['arm'], None, None, "Not in test case arch allow list"),
    ("skip", True, None, None, "Skip filter"),
    ("tags", set(['sensor', 'bluetooth']), "ignore_tags", ['bluetooth'], "Excluded tags per platform (exclude_tags)"),
    ("min_flash", "2024", "flash", "1024", "Not enough FLASH"),
    ("min_ram", "500", "ram", "256", "Not enough RAM"),
    ("None", "None", "env", ['BSIM_OUT_PATH', 'demo_env'], "Environment (BSIM_OUT_PATH, demo_env) not satisfied"),
    ("build_on_all", True, None, None, "Platform is excluded on command line."),
    (None, None, "supported_toolchains", ['gcc'], "Not supported by the toolchain"),
]


@pytest.mark.parametrize("tc_attribute, tc_value, plat_attribute, plat_value, expected_discards",
                         TESTDATA_PART1)
def test_apply_filters_part1(class_env, all_testsuites_dict, platforms_list,
                             tc_attribute, tc_value, plat_attribute, plat_value, expected_discards):
    """ Testing apply_filters function of TestPlan class in Twister
    Part 1: Response of apply_filters function have
            appropriate values according to the filters
    """
    plan = TestPlan(class_env)
    if tc_attribute is None and plat_attribute is None:
        plan.apply_filters()

    plan.platforms = platforms_list
    plan.platform_names = [p.name for p in platforms_list]
    plan.testsuites = all_testsuites_dict
    for plat in plan.platforms:
        if plat_attribute == "ignore_tags":
            plat.ignore_tags = plat_value
        if plat_attribute == "flash":
            plat.flash = plat_value
        if plat_attribute == "ram":
            plat.ram = plat_value
        if plat_attribute == "env":
            plat.env = plat_value
            plat.env_satisfied = False
        if plat_attribute == "supported_toolchains":
            plat.supported_toolchains = plat_value
    for _, testcase in plan.testsuites.items():
        if tc_attribute == "toolchain_allow":
            testcase.toolchain_allow = tc_value
        if tc_attribute == "platform_allow":
            testcase.platform_allow = tc_value
        if tc_attribute == "toolchain_exclude":
            testcase.toolchain_exclude = tc_value
        if tc_attribute == "platform_exclude":
            testcase.platform_exclude = tc_value
        if tc_attribute == "arch_exclude":
            testcase.arch_exclude = tc_value
        if tc_attribute == "arch_allow":
            testcase.arch_allow = tc_value
        if tc_attribute == "skip":
            testcase.skip = tc_value
        if tc_attribute == "tags":
            testcase.tags = tc_value
        if tc_attribute == "min_flash":
            testcase.min_flash = tc_value
        if tc_attribute == "min_ram":
            testcase.min_ram = tc_value

    if tc_attribute == "build_on_all":
        for _, testcase in plan.testsuites.items():
            testcase.build_on_all = tc_value
        plan.apply_filters(exclude_platform=['demo_board_1'])
    elif plat_attribute == "supported_toolchains":
        plan.apply_filters(force_toolchain=False,
                                                 exclude_platform=['demo_board_1'],
                                                 platform=['demo_board_2'])
    elif tc_attribute is None and plat_attribute is None:
        plan.apply_filters()
    else:
        plan.apply_filters(exclude_platform=['demo_board_1'],
                                                 platform=['demo_board_2'])

    filtered_instances = list(filter(lambda item:  item.status == "filtered", plan.instances.values()))
    for d in filtered_instances:
        assert d.reason == expected_discards

TESTDATA_PART2 = [
    ("runnable", "True", "Not runnable on device"),
    ("exclude_tag", ['test_a'], "Command line testsuite exclude filter"),
    ("run_individual_tests", ['scripts/tests/twister/test_data/testsuites/tests/test_a/test_a.check_1'], "TestSuite name filter"),
    ("arch", ['arm_test'], "Command line testsuite arch filter"),
    ("tag", ['test_d'], "Command line testsuite tag filter")
    ]


@pytest.mark.parametrize("extra_filter, extra_filter_value, expected_discards", TESTDATA_PART2)
def test_apply_filters_part2(class_testplan, all_testsuites_dict,
                             platforms_list, extra_filter, extra_filter_value, expected_discards):
    """ Testing apply_filters function of TestPlan class in Twister
    Part 2 : Response of apply_filters function (discard dictionary) have
             appropriate values according to the filters
    """

    class_testplan.platforms = platforms_list
    class_testplan.platform_names = [p.name for p in platforms_list]
    class_testplan.testsuites = all_testsuites_dict
    kwargs = {
        extra_filter : extra_filter_value,
        "exclude_platform" : [
            'demo_board_1'
            ],
        "platform" : [
            'demo_board_2'
            ]
        }
    class_testplan.apply_filters(**kwargs)
    filtered_instances = list(filter(lambda item:  item.status == "filtered", class_testplan.instances.values()))
    for d in filtered_instances:
        assert d.reason == expected_discards


TESTDATA_PART3 = [
    (20, 20, -1, 0),
    (-2, -1, 10, 20),
    (0, 0, 0, 0)
    ]

@pytest.mark.parametrize("tc_min_flash, plat_flash, tc_min_ram, plat_ram",
                         TESTDATA_PART3)
def test_apply_filters_part3(class_testplan, all_testsuites_dict, platforms_list,
                             tc_min_flash, plat_flash, tc_min_ram, plat_ram):
    """ Testing apply_filters function of TestPlan class in Twister
    Part 3 : Testing edge cases for ram and flash values of platforms & testsuites
    """
    class_testplan.platforms = platforms_list
    class_testplan.platform_names = [p.name for p in platforms_list]
    class_testplan.testsuites = all_testsuites_dict

    for plat in class_testplan.platforms:
        plat.flash = plat_flash
        plat.ram = plat_ram
    for _, testcase in class_testplan.testsuites.items():
        testcase.min_ram = tc_min_ram
        testcase.min_flash = tc_min_flash
    class_testplan.apply_filters(exclude_platform=['demo_board_1'],
                                             platform=['demo_board_2'])

    filtered_instances = list(filter(lambda item:  item.status == "filtered", class_testplan.instances.values()))
    assert not filtered_instances

def test_add_instances(test_data, class_env, all_testsuites_dict, platforms_list):
    """ Testing add_instances() function of TestPlan class in Twister
    Test 1: instances dictionary keys have expected values (Platform Name + Testcase Name)
    Test 2: Values of 'instances' dictionary in Testsuite class are an
	        instance of 'TestInstance' class
    Test 3: Values of 'instances' dictionary have expected values.
    """
    class_env.outdir = test_data
    plan = TestPlan(class_env)
    plan.platforms = platforms_list
    platform = plan.get_platform("demo_board_2")
    instance_list = []
    for _, testcase in all_testsuites_dict.items():
        instance = TestInstance(testcase, platform, class_env.outdir)
        instance_list.append(instance)
    plan.add_instances(instance_list)
    assert list(plan.instances.keys()) == \
		   [platform.name + '/' + s for s in list(all_testsuites_dict.keys())]
    assert all(isinstance(n, TestInstance) for n in list(plan.instances.values()))
    assert list(plan.instances.values()) == instance_list
