#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''
This test file contains testcases for Testsuite class of twister
'''
import sys
import os
import csv
import pytest
from mock import call, patch, MagicMock

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

from twisterlib import TestCase, TestSuite, TestInstance, Platform

def test_testsuite_add_testcases(class_testsuite):
    """ Testing add_testcase function of Testsuite class in twister """
    # Test 1: Check the list of testcases after calling add testcases function is as expected
    class_testsuite.SAMPLE_FILENAME = 'test_sample_app.yaml'
    class_testsuite.TESTCASE_FILENAME = 'test_data.yaml'
    class_testsuite.add_testcases()
    tests_rel_dir = 'scripts/tests/twister/test_data/testcases/tests/'
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

    # Test 2 : Assert Testcase name is expected & all testcases values are testcase class objects
    testcase = class_testsuite.testcases.get(tests_rel_dir + 'test_a/test_a.check_1')
    assert testcase.name == tests_rel_dir + 'test_a/test_a.check_1'
    assert all(isinstance(n, TestCase) for n in class_testsuite.testcases.values())

@pytest.mark.parametrize("board_root_dir", [("board_config_file_not_exist"), ("board_config")])
def test_add_configurations(test_data, class_testsuite, board_root_dir):
    """ Testing add_configurations function of TestSuite class in Twister
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
    """ Testing get_all_testcases function of TestSuite class in Twister """
    class_testsuite.testcases = all_testcases_dict
    expected_tests = ['sample_test.app', 'test_a.check_1.1a', 'test_a.check_1.1c',
                    'test_a.check_1.2a', 'test_a.check_1.2b', 'test_a.check_1.Unit_1c', 'test_a.check_1.unit_1a', 'test_a.check_1.unit_1b', 'test_a.check_2.1a', 'test_a.check_2.1c', 'test_a.check_2.2a', 'test_a.check_2.2b', 'test_a.check_2.Unit_1c', 'test_a.check_2.unit_1a', 'test_a.check_2.unit_1b', 'test_b.check_1', 'test_b.check_2', 'test_c.check_1', 'test_c.check_2']
    assert len(class_testsuite.get_all_tests()) == 19
    assert sorted(class_testsuite.get_all_tests()) == sorted(expected_tests)

def test_get_toolchain(class_testsuite, monkeypatch, capsys):
    """ Testing get_toolchain function of TestSuite class in Twister
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
    """ Testing get_platforms function of TestSuite class in Twister """
    class_testsuite.platforms = platforms_list
    platform = class_testsuite.get_platform("demo_board_1")
    assert isinstance(platform, Platform)
    assert platform.name == "demo_board_1"

def test_load_from_file(test_data, class_testsuite,
                        platforms_list, all_testcases_dict, caplog, tmpdir_factory):
    """ Testing load_from_file function of TestSuite class in Twister """
    # Scenario 1 : Validating the error raised if file to load from doesn't exist
    with pytest.raises(SystemExit):
        class_testsuite.load_from_file(test_data +"twister_test.csv")
    assert "Couldn't find input file with list of tests." in caplog.text

    # Scenario 2: Testing if the 'instances' dictionary in Testsuite class contains
    # the expected values after execution of load_from_file function
    # Note: tmp_dir is the temporary directory created so that the contents
    # get deleted after invocation of this testcase.
    tmp_dir = tmpdir_factory.mktemp("tmp")
    class_testsuite.outdir = tmp_dir
    class_testsuite.platforms = platforms_list
    class_testsuite.testcases = all_testcases_dict
    instance_name_list = []
    failed_platform_list = []
    with open(os.path.join(test_data, "twister.csv"), "r") as filepath:
        for row in csv.DictReader(filepath):
            testcase_root = os.path.join(ZEPHYR_BASE,
                                         "scripts/tests/twister/test_data/testcases")
            workdir = row['test'].split('/')[-3] + "/" + row['test'].split('/')[-2]
            test_name = os.path.basename(os.path.normpath(row['test']))
            testcase = TestCase(testcase_root, workdir, test_name)
            testcase.build_only = False
            instance_name = row["platform"] + "/" + row["test"]
            instance_name_list.append(instance_name)
        class_testsuite.load_from_file(test_data + "twister.csv")
        assert list(class_testsuite.instances.keys()) == instance_name_list

        #Scenario 3 : Assert the number of times mock method (get_platform) is called,
        # equals to the number of testcases failed
        failed_platform_list = [row["platform"]
                                for row in csv.DictReader(filepath)
                                if row["status"] == "failed"]
        for row in failed_platform_list:
            with patch.object(TestSuite, 'get_platform') as mock_method:
                class_testsuite.load_from_file(class_testsuite.outdir + "twister.csv",
                                               filter_status=["Skipped", "Passed"])
                calls = [call(row)]
                mock_method.assert_has_calls(calls, any_order=True)
                assert mock_method.call_count == len(failed_platform_list)

    # Scenario 4 : Assert add_instances function is called from load_from_file function
    class_testsuite.add_instances = MagicMock(side_effect=class_testsuite.add_instances)
    class_testsuite.load_from_file(test_data + "twister.csv")
    class_testsuite.add_instances.assert_called()

    # Scenario 5 : Validate if the Keyerror is raised in case if a header expected is missing
    with pytest.raises(SystemExit):
        class_testsuite.load_from_file(test_data + "twister_keyerror.csv")
    assert "Key error while parsing tests file.('status')" in caplog.text

TESTDATA_PART1 = [
    ("toolchain_allow", ['gcc'], None, None, "Not in testcase toolchain allow list"),
    ("platform_allow", ['demo_board_1'], None, None, "Not in testcase platform allow list"),
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
def test_apply_filters_part1(class_testsuite, all_testcases_dict, platforms_list,
                             tc_attribute, tc_value, plat_attribute, plat_value, expected_discards):
    """ Testing apply_filters function of TestSuite class in Twister
    Part 1: Response of apply_filters function (discard dictionary) have
            appropriate values according to the filters
    """
    if tc_attribute is None and plat_attribute is None:
        discards = class_testsuite.apply_filters()
        assert not discards

    class_testsuite.platforms = platforms_list
    class_testsuite.testcases = all_testcases_dict
    for plat in class_testsuite.platforms:
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
    for _, testcase in class_testsuite.testcases.items():
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
        for _, testcase in class_testsuite.testcases.items():
            testcase.build_on_all = tc_value
        discards = class_testsuite.apply_filters(exclude_platform=['demo_board_1'])
    elif plat_attribute == "supported_toolchains":
        discards = class_testsuite.apply_filters(force_toolchain=False,
                                                 exclude_platform=['demo_board_1'],
                                                 platform=['demo_board_2'])
    elif tc_attribute is None and plat_attribute is None:
        discards = class_testsuite.apply_filters()
    else:
        discards = class_testsuite.apply_filters(exclude_platform=['demo_board_1'],
                                                 platform=['demo_board_2'])

    for x in [expected_discards]:
        assert x in discards.values()

TESTDATA_PART2 = [
    ("runnable", "True", "Not runnable on device"),
    ("exclude_tag", ['test_a'], "Command line testcase exclude filter"),
    ("run_individual_tests", ['scripts/tests/twister/test_data/testcases/tests/test_a/test_a.check_1'], "Testcase name filter"),
    ("arch", ['arm_test'], "Command line testcase arch filter"),
    ("tag", ['test_d'], "Command line testcase tag filter")
    ]


@pytest.mark.parametrize("extra_filter, extra_filter_value, expected_discards", TESTDATA_PART2)
def test_apply_filters_part2(class_testsuite, all_testcases_dict,
                             platforms_list, extra_filter, extra_filter_value, expected_discards):
    """ Testing apply_filters function of TestSuite class in Twister
    Part 2 : Response of apply_filters function (discard dictionary) have
             appropriate values according to the filters
    """

    class_testsuite.platforms = platforms_list
    class_testsuite.testcases = all_testcases_dict
    kwargs = {
        extra_filter : extra_filter_value,
        "exclude_platform" : [
            'demo_board_1'
            ],
        "platform" : [
            'demo_board_2'
            ]
        }
    discards = class_testsuite.apply_filters(**kwargs)
    assert discards
    for d in discards.values():
        assert d == expected_discards


TESTDATA_PART3 = [
    (20, 20, -1, 0),
    (-2, -1, 10, 20),
    (0, 0, 0, 0)
    ]

@pytest.mark.parametrize("tc_min_flash, plat_flash, tc_min_ram, plat_ram",
                         TESTDATA_PART3)
def test_apply_filters_part3(class_testsuite, all_testcases_dict, platforms_list,
                             tc_min_flash, plat_flash, tc_min_ram, plat_ram):
    """ Testing apply_filters function of TestSuite class in Twister
    Part 3 : Testing edge cases for ram and flash values of platforms & testcases
    """
    class_testsuite.platforms = platforms_list
    class_testsuite.testcases = all_testcases_dict

    for plat in class_testsuite.platforms:
        plat.flash = plat_flash
        plat.ram = plat_ram
    for _, testcase in class_testsuite.testcases.items():
        testcase.min_ram = tc_min_ram
        testcase.min_flash = tc_min_flash
    discards = class_testsuite.apply_filters(exclude_platform=['demo_board_1'],
                                             platform=['demo_board_2'])
    assert not discards

def test_add_instances(test_data, class_testsuite, all_testcases_dict, platforms_list):
    """ Testing add_instances() function of TestSuite class in Twister
    Test 1: instances dictionary keys have expected values (Platform Name + Testcase Name)
    Test 2: Values of 'instances' dictionary in Testsuite class are an
	        instance of 'TestInstance' class
    Test 3: Values of 'instances' dictionary have expected values.
    """
    class_testsuite.outdir = test_data
    class_testsuite.platforms = platforms_list
    platform = class_testsuite.get_platform("demo_board_2")
    instance_list = []
    for _, testcase in all_testcases_dict.items():
        instance = TestInstance(testcase, platform, class_testsuite.outdir)
        instance_list.append(instance)
    class_testsuite.add_instances(instance_list)
    assert list(class_testsuite.instances.keys()) == \
		   [platform.name + '/' + s for s in list(all_testcases_dict.keys())]
    assert all(isinstance(n, TestInstance) for n in list(class_testsuite.instances.values()))
    assert list(class_testsuite.instances.values()) == instance_list
