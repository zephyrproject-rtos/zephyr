#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''
This test file contains testsuites for testsuite.py module of twister
'''
import sys
import os
import mock
import pytest

from contextlib import nullcontext

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

from twisterlib.testplan import TestPlan, change_skip_to_error_if_integration
from twisterlib.testinstance import TestInstance
from twisterlib.testsuite import TestSuite
from twisterlib.platform import Platform
from twisterlib.quarantine import Quarantine
from twisterlib.error import TwisterRuntimeError


def test_testplan_add_testsuites_short(class_testplan):
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
                          'test_e.check_1',
                          'sample_test.app',
                          'test_config.main']
    testsuite_list = []
    for key in sorted(class_testplan.testsuites.keys()):
        testsuite_list.append(os.path.basename(os.path.normpath(key)))
    assert sorted(testsuite_list) == sorted(expected_testsuites)

    # Test 2 : Assert Testcase name is expected & all testsuites values are testcase class objects
    suite = class_testplan.testsuites.get(tests_rel_dir + 'test_a/test_a.check_1')
    assert suite.name == tests_rel_dir + 'test_a/test_a.check_1'
    assert all(isinstance(n, TestSuite) for n in class_testplan.testsuites.values())

@pytest.mark.parametrize("board_root_dir", [("board_config_file_not_exist"), ("board_config")])
def test_add_configurations_short(test_data, class_env, board_root_dir):
    """ Testing add_configurations function of TestPlan class in Twister
    Test : Asserting on default platforms list
    """
    class_env.board_roots = [os.path.abspath(test_data + board_root_dir)]
    plan = TestPlan(class_env)
    plan.parse_configuration(config_file=class_env.test_config)
    if board_root_dir == "board_config":
        plan.add_configurations()
        assert sorted(plan.default_platforms) == sorted(['demo_board_1', 'demo_board_3'])
    elif board_root_dir == "board_config_file_not_exist":
        plan.add_configurations()
        assert sorted(plan.default_platforms) != sorted(['demo_board_1'])


def test_get_all_testsuites_short(class_testplan, all_testsuites_dict):
    """ Testing get_all_testsuites function of TestPlan class in Twister """
    plan = class_testplan
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
                      'test_d.check_1.unit_1b',
                      'test_e.check_1.1a', 'test_e.check_1.1b',
                      'test_config.main']

    assert sorted(plan.get_all_tests()) == sorted(expected_tests)

def test_get_platforms_short(class_testplan, platforms_list):
    """ Testing get_platforms function of TestPlan class in Twister """
    plan = class_testplan
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
def test_apply_filters_part1(class_testplan, all_testsuites_dict, platforms_list,
                             tc_attribute, tc_value, plat_attribute, plat_value, expected_discards):
    """ Testing apply_filters function of TestPlan class in Twister
    Part 1: Response of apply_filters function have
            appropriate values according to the filters
    """
    plan = class_testplan
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

def test_add_instances_short(tmp_path, class_env, all_testsuites_dict, platforms_list):
    """ Testing add_instances() function of TestPlan class in Twister
    Test 1: instances dictionary keys have expected values (Platform Name + Testcase Name)
    Test 2: Values of 'instances' dictionary in Testsuite class are an
	        instance of 'TestInstance' class
    Test 3: Values of 'instances' dictionary have expected values.
    """
    class_env.outdir = tmp_path
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


QUARANTINE_BASIC = {
    'demo_board_1/scripts/tests/twister/test_data/testsuites/tests/test_a/test_a.check_1' : 'a1 on board_1 and board_3',
    'demo_board_3/scripts/tests/twister/test_data/testsuites/tests/test_a/test_a.check_1' : 'a1 on board_1 and board_3'
}

QUARANTINE_WITH_REGEXP = {
    'demo_board_2/scripts/tests/twister/test_data/testsuites/tests/test_a/test_a.check_2' : 'a2 and c2 on x86',
    'demo_board_1/scripts/tests/twister/test_data/testsuites/tests/test_d/test_d.check_1' : 'all test_d',
    'demo_board_3/scripts/tests/twister/test_data/testsuites/tests/test_d/test_d.check_1' : 'all test_d',
    'demo_board_2/scripts/tests/twister/test_data/testsuites/tests/test_d/test_d.check_1' : 'all test_d',
    'demo_board_2/scripts/tests/twister/test_data/testsuites/tests/test_c/test_c.check_2' : 'a2 and c2 on x86'
}

QUARANTINE_PLATFORM = {
    'demo_board_3/scripts/tests/twister/test_data/testsuites/tests/test_a/test_a.check_1' : 'all on board_3',
    'demo_board_3/scripts/tests/twister/test_data/testsuites/tests/test_a/test_a.check_2' : 'all on board_3',
    'demo_board_3/scripts/tests/twister/test_data/testsuites/tests/test_d/test_d.check_1' : 'all on board_3',
    'demo_board_3/scripts/tests/twister/test_data/testsuites/tests/test_b/test_b.check_1' : 'all on board_3',
    'demo_board_3/scripts/tests/twister/test_data/testsuites/tests/test_b/test_b.check_2' : 'all on board_3',
    'demo_board_3/scripts/tests/twister/test_data/testsuites/tests/test_c/test_c.check_1' : 'all on board_3',
    'demo_board_3/scripts/tests/twister/test_data/testsuites/tests/test_c/test_c.check_2' : 'all on board_3',
    'demo_board_3/scripts/tests/twister/test_data/testsuites/tests/test_e/test_e.check_1' : 'all on board_3',
    'demo_board_3/scripts/tests/twister/test_data/testsuites/tests/test_config/test_config.main' : 'all on board_3'
}

QUARANTINE_MULTIFILES = {
    **QUARANTINE_BASIC,
    **QUARANTINE_WITH_REGEXP
}

@pytest.mark.parametrize(
    ("quarantine_files, quarantine_verify, expected_val"),
    [
        (['basic.yaml'], False, QUARANTINE_BASIC),
        (['with_regexp.yaml'], False, QUARANTINE_WITH_REGEXP),
        (['with_regexp.yaml'], True, QUARANTINE_WITH_REGEXP),
        (['platform.yaml'], False, QUARANTINE_PLATFORM),
        (['basic.yaml', 'with_regexp.yaml'], False, QUARANTINE_MULTIFILES),
        (['empty.yaml'], False, {})
    ],
    ids=[
        'basic',
        'with_regexp',
        'quarantine_verify',
        'platform',
        'multifiles',
        'empty'
    ])
def test_quarantine_short(class_testplan, platforms_list, test_data,
                    quarantine_files, quarantine_verify, expected_val):
    """ Testing quarantine feature in Twister
    """
    class_testplan.options.all = True
    class_testplan.platforms = platforms_list
    class_testplan.platform_names = [p.name for p in platforms_list]
    class_testplan.TESTSUITE_FILENAME = 'test_data.yaml'
    class_testplan.add_testsuites()

    quarantine_list = [
        os.path.join(test_data, 'quarantines', quarantine_file) for quarantine_file in quarantine_files
    ]
    class_testplan.quarantine = Quarantine(quarantine_list)
    class_testplan.options.quarantine_verify = quarantine_verify
    class_testplan.apply_filters()

    for testname, instance in class_testplan.instances.items():
        if quarantine_verify:
            if testname in expected_val:
                assert not instance.status
            else:
                assert instance.status == 'filtered'
                assert instance.reason == "Not under quarantine"
        else:
            if testname in expected_val:
                assert instance.status == 'filtered'
                assert instance.reason == "Quarantine: " + expected_val[testname]
            else:
                assert not instance.status


TESTDATA_PART4 = [
    (os.path.join('test_d', 'test_d.check_1'), ['dummy'],
     None, 'Snippet not supported'),
    (os.path.join('test_c', 'test_c.check_1'), ['cdc-acm-console'],
     0, None),
    (os.path.join('test_d', 'test_d.check_1'), ['dummy', 'cdc-acm-console'],
     2, 'Snippet not supported'),
]

@pytest.mark.parametrize(
    'testpath, required_snippets, expected_filtered_len, expected_filtered_reason',
    TESTDATA_PART4,
    ids=['app', 'global', 'multiple']
)
def test_required_snippets_short(
    class_testplan,
    all_testsuites_dict,
    platforms_list,
    testpath,
    required_snippets,
    expected_filtered_len,
    expected_filtered_reason
):
    """ Testing required_snippets function of TestPlan class in Twister """
    plan = class_testplan
    testpath = os.path.join('scripts', 'tests', 'twister', 'test_data',
                            'testsuites', 'tests', testpath)
    testsuite = class_testplan.testsuites.get(testpath)
    plan.platforms = platforms_list
    plan.platform_names = [p.name for p in platforms_list]
    plan.testsuites = {testpath: testsuite}

    print(plan.testsuites)

    for _, testcase in plan.testsuites.items():
        testcase.exclude_platform = []
        testcase.required_snippets = required_snippets
        testcase.build_on_all = True

    plan.apply_filters()

    filtered_instances = list(
        filter(lambda item: item.status == "filtered", plan.instances.values())
    )
    if expected_filtered_len is not None:
        assert len(filtered_instances) == expected_filtered_len
    if expected_filtered_reason is not None:
        for d in filtered_instances:
            assert d.reason == expected_filtered_reason


def test_testplan_get_level():
    testplan = TestPlan(env=mock.Mock())
    lvl1 = mock.Mock()
    lvl1.name = 'a lvl'
    lvl2 = mock.Mock()
    lvl2.name = 'a lvl'
    lvl3 = mock.Mock()
    lvl3.name = 'other lvl'
    testplan.levels.append(lvl1)
    testplan.levels.append(lvl2)
    testplan.levels.append(lvl3)

    name = 'a lvl'

    res = testplan.get_level(name)
    assert res == lvl1

    res = testplan.get_level(name)
    assert res == lvl1

    testplan.levels.remove(lvl1)
    testplan.levels.remove(lvl2)

    res = testplan.get_level(name)
    assert res is None


TESTDATA_1 = [
    ('', {}),
    (
"""\
levels:
  - name: lvl1
    adds:
      - sc1
      - sc2
    inherits: []
  - name: lvl2
    adds:
      - sc1-1
      - sc1-2
    inherits: [lvl1]
""",
    {
        'lvl1': ['sc1', 'sc2'],
        'lvl2': ['sc1-1', 'sc1-2', 'sc1', 'sc2']
    }
    ),
]

@pytest.mark.parametrize(
    'config_yaml, expected_scenarios',
    TESTDATA_1,
    ids=['no config', 'valid config']
)
def test_testplan_parse_configuration(tmp_path, config_yaml, expected_scenarios):
    testplan = TestPlan(env=mock.Mock())
    testplan.scenarios = ['sc1', 'sc1-1', 'sc1-2', 'sc2']

    tmp_config_file = tmp_path / 'config_file.yaml'
    if config_yaml:
        tmp_config_file.write_text(config_yaml)

    with pytest.raises(TwisterRuntimeError) if not config_yaml else nullcontext():
        testplan.parse_configuration(tmp_config_file)

    if not testplan.levels:
        assert expected_scenarios == {}
    for level in testplan.levels:
        assert sorted(level.scenarios) == sorted(expected_scenarios[level.name])


TESTDATA_2 = [
    ([], [], False),
    (['ts1.tc3'], [], True),
    (['ts2.tc2'], ['- ts2'], False),
]

@pytest.mark.parametrize(
    'sub_tests, expected_outs, expect_error',
    TESTDATA_2,
    ids=['no subtests', 'subtests not found', 'valid subtests']
)
def test_testplan_find_subtests(
    capfd,
    sub_tests,
    expected_outs,
    expect_error
):
    testplan = TestPlan(env=mock.Mock())
    testplan.options = mock.Mock(sub_test=sub_tests)
    testplan.run_individual_testsuite = []
    testplan.testsuites = {
        'ts1': mock.Mock(
            testcases=[
                mock.Mock(),
                mock.Mock(),
            ]
        ),
        'ts2': mock.Mock(
            testcases=[
                mock.Mock(),
                mock.Mock(),
                mock.Mock(),
            ]
        )
    }
    testplan.testsuites['ts1'].name = 'ts1'
    testplan.testsuites['ts1'].testcases[0].name = 'ts1.tc1'
    testplan.testsuites['ts1'].testcases[1].name = 'ts1.tc2'
    testplan.testsuites['ts2'].name = 'ts2'
    testplan.testsuites['ts2'].testcases[0].name = 'ts2.tc1'
    testplan.testsuites['ts2'].testcases[1].name = 'ts2.tc2'
    testplan.testsuites['ts2'].testcases[2].name = 'ts2.tc3'

    with pytest.raises(TwisterRuntimeError) if expect_error else nullcontext():
        testplan.find_subtests()

    out, err = capfd.readouterr()
    sys.stdout.write(out)
    sys.stdout.write(err)

    assert all([printout in out for printout in expected_outs])


TESTDATA_3 = [
    (0, 0, [], False, [], TwisterRuntimeError, []),
    (1, 1, [], False, [], TwisterRuntimeError, []),
    (1, 0, [], True, [], TwisterRuntimeError, ['No quarantine list given to be verified']),
#    (1, 0, ['qfile.yaml'], False, ['# empty'], None, ['Quarantine file qfile.yaml is empty']),
    (1, 0, ['qfile.yaml'], False, ['- platforms:\n  - demo_board_3\n  comment: "board_3"'], None, []),
]

@pytest.mark.parametrize(
    'added_testsuite_count, load_errors, ql, qv, ql_data, exception, expected_logs',
    TESTDATA_3,
    ids=['no tests', 'load errors', 'quarantine verify without quarantine list',
#         'empty quarantine file',
         'valid quarantine file']
)
def test_testplan_discover(
    tmp_path,
    caplog,
    added_testsuite_count,
    load_errors,
    ql,
    qv,
    ql_data,
    exception,
    expected_logs
):
    for qf, data in zip(ql, ql_data):
        tmp_qf = tmp_path / qf
        tmp_qf.write_text(data)

    testplan = TestPlan(env=mock.Mock())
    testplan.options = mock.Mock(
        test='ts1',
        quarantine_list=[tmp_path / qf for qf in ql],
        quarantine_verify=qv,
    )
    testplan.testsuites = {
        'ts1': mock.Mock(id=1),
        'ts2': mock.Mock(id=2),
    }
    testplan.run_individual_testsuite = 'ts0'
    testplan.load_errors = load_errors
    testplan.add_testsuites = mock.Mock(return_value=added_testsuite_count)
    testplan.find_subtests = mock.Mock()
    testplan.report_duplicates = mock.Mock()
    testplan.parse_configuration = mock.Mock()
    testplan.add_configurations = mock.Mock()

    with pytest.raises(exception) if exception else nullcontext():
        testplan.discover()

    testplan.add_testsuites.assert_called_once_with(testsuite_filter='ts1')
    assert all([log in caplog.text for log in expected_logs])


TESTDATA_4 = [
    (None, None, None, None, '00',
     TwisterRuntimeError, [], []),
    (None, True, None, None, '6/4',
     TwisterRuntimeError, set(['t-p3', 't-p4', 't-p1', 't-p2']), []),
    (None, None, 'load_tests.json', None, '0/4',
     TwisterRuntimeError, set(['lt-p1', 'lt-p3', 'lt-p4', 'lt-p2']), []),
    ('suffix', None, None, True, '2/4',
     None, set(['ts-p4', 'ts-p2', 'ts-p3']), [2, 4]),
]

@pytest.mark.parametrize(
    'report_suffix, only_failed, load_tests, test_only, subset,' \
    ' exception, expected_selected_platforms, expected_generate_subset_args',
    TESTDATA_4,
    ids=['apply_filters only', 'only failed', 'load tests', 'test only']
)
def test_testplan_load(
    tmp_path,
    report_suffix,
    only_failed,
    load_tests,
    test_only,
    subset,
    exception,
    expected_selected_platforms,
    expected_generate_subset_args
):
    twister_json = """\
{
    "testsuites": [
        {
            "name": "ts1",
            "platform": "t-p1",
            "testcases": []
        },
        {
            "name": "ts1",
            "platform": "t-p2",
            "testcases": []
        },
        {
            "name": "ts2",
            "platform": "t-p3",
            "testcases": []
        },
        {
            "name": "ts2",
            "platform": "t-p4",
            "testcases": []
        }
    ]
}
"""
    twister_file = tmp_path / 'twister.json'
    twister_file.write_text(twister_json)

    twister_suffix_json = """\
{
    "testsuites": [
        {
            "name": "ts1",
            "platform": "ts-p1",
            "testcases": []
        },
        {
            "name": "ts1",
            "platform": "ts-p2",
            "testcases": []
        },
        {
            "name": "ts2",
            "platform": "ts-p3",
            "testcases": []
        },
        {
            "name": "ts2",
            "platform": "ts-p4",
            "testcases": []
        }
    ]
}
"""
    twister_suffix_file = tmp_path / 'twister_suffix.json'
    twister_suffix_file.write_text(twister_suffix_json)

    load_tests_json = """\
{
    "testsuites": [
        {
            "name": "ts1",
            "platform": "lt-p1",
            "testcases": []
        },
        {
            "name": "ts1",
            "platform": "lt-p2",
            "testcases": []
        },
        {
            "name": "ts2",
            "platform": "lt-p3",
            \"testcases": []
        },
        {
            "name": "ts2",
            "platform": "lt-p4",
            "testcases": []
        }
    ]
}
"""
    load_tests_file = tmp_path / 'load_tests.json'
    load_tests_file.write_text(load_tests_json)

    testplan = TestPlan(env=mock.Mock(outdir=tmp_path))
    testplan.testsuites = {
        'ts1': mock.Mock(testcases=[], extra_configs=[]),
        'ts2': mock.Mock(testcases=[], extra_configs=[]),
    }
    testplan.testsuites['ts1'].name = 'ts1'
    testplan.testsuites['ts2'].name = 'ts2'
    testplan.options = mock.Mock(
        outdir=tmp_path,
        report_suffix=report_suffix,
        only_failed=only_failed,
        load_tests=tmp_path / load_tests if load_tests else None,
        test_only=test_only,
        exclude_platform=['t-p0', 't-p1',
                          'ts-p0', 'ts-p1',
                          'lt-p0', 'lt-p1'],
        platform=['t-p1', 't-p2', 't-p3', 't-p4',
                  'ts-p1', 'ts-p2', 'ts-p3', 'ts-p4',
                  'lt-p1', 'lt-p2', 'lt-p3', 'lt-p4'],
        subset=subset
    )
    testplan.platforms=[mock.Mock(), mock.Mock(), mock.Mock(), mock.Mock(),
                        mock.Mock(), mock.Mock(), mock.Mock(), mock.Mock(),
                        mock.Mock(), mock.Mock(), mock.Mock(), mock.Mock()]
    testplan.platforms[0].name = 't-p1'
    testplan.platforms[1].name = 't-p2'
    testplan.platforms[2].name = 't-p3'
    testplan.platforms[3].name = 't-p4'
    testplan.platforms[4].name = 'ts-p1'
    testplan.platforms[5].name = 'ts-p2'
    testplan.platforms[6].name = 'ts-p3'
    testplan.platforms[7].name = 'ts-p4'
    testplan.platforms[8].name = 'lt-p1'
    testplan.platforms[9].name = 'lt-p2'
    testplan.platforms[10].name = 'lt-p3'
    testplan.platforms[11].name = 'lt-p4'
    testplan.platforms[0].normalized_name = 't-p1'
    testplan.platforms[1].normalized_name = 't-p2'
    testplan.platforms[2].normalized_name = 't-p3'
    testplan.platforms[3].normalized_name = 't-p4'
    testplan.platforms[4].normalized_name = 'ts-p1'
    testplan.platforms[5].normalized_name = 'ts-p2'
    testplan.platforms[6].normalized_name = 'ts-p3'
    testplan.platforms[7].normalized_name = 'ts-p4'
    testplan.platforms[8].normalized_name = 'lt-p1'
    testplan.platforms[9].normalized_name = 'lt-p2'
    testplan.platforms[10].normalized_name = 'lt-p3'
    testplan.platforms[11].normalized_name = 'lt-p4'
    testplan.generate_subset = mock.Mock()
    testplan.apply_filters = mock.Mock()

    with mock.patch('twisterlib.testinstance.TestInstance.create_overlay', mock.Mock()), \
         pytest.raises(exception) if exception else nullcontext():
        testplan.load()

    assert testplan.selected_platforms == expected_selected_platforms
    if expected_generate_subset_args:
        testplan.generate_subset.assert_called_once_with(*expected_generate_subset_args)
    else:
        testplan.generate_subset.assert_not_called()


TESTDATA_5 = [
    (False, False, None, 1, 2,
     ['plat1/testA', 'plat1/testB', 'plat1/testC',
      'plat3/testA', 'plat3/testB', 'plat3/testC']),
    (False, False, None, 1, 5,
     ['plat1/testA',
      'plat3/testA', 'plat3/testB', 'plat3/testC']),
    (False, False, None, 2, 2,
     ['plat2/testA', 'plat2/testB']),
    (True, False, None, 1, 2,
     ['plat1/testA', 'plat2/testA', 'plat1/testB',
      'plat3/testA', 'plat3/testB', 'plat3/testC']),
    (True, False, None, 2, 2,
     ['plat2/testB', 'plat1/testC']),
    (True, True, 123, 1, 2,
     ['plat2/testA', 'plat2/testB', 'plat1/testC',
      'plat3/testB', 'plat3/testA', 'plat3/testC']),
    (True, True, 123, 2, 2,
     ['plat1/testB', 'plat1/testA']),
]

@pytest.mark.parametrize(
    'device_testing, shuffle, seed, subset, sets, expected_subset',
    TESTDATA_5,
    ids=['subset 1', 'subset 1 out of 5', 'subset 2',
         'device testing, subset 1', 'device testing, subset 2',
         'device testing, shuffle with seed, subset 1',
         'device testing, shuffle with seed, subset 2']
)
def test_testplan_generate_subset(
    device_testing,
    shuffle,
    seed,
    subset,
    sets,
    expected_subset
):
    testplan = TestPlan(env=mock.Mock())
    testplan.options = mock.Mock(
        device_testing=device_testing,
        shuffle_tests=shuffle,
        shuffle_tests_seed=seed
    )
    testplan.instances = {
        'plat1/testA': mock.Mock(status=None),
        'plat1/testB': mock.Mock(status=None),
        'plat1/testC': mock.Mock(status=None),
        'plat2/testA': mock.Mock(status=None),
        'plat2/testB': mock.Mock(status=None),
        'plat3/testA': mock.Mock(status='skipped'),
        'plat3/testB': mock.Mock(status='skipped'),
        'plat3/testC': mock.Mock(status='error'),
    }

    testplan.generate_subset(subset, sets)

    assert [instance for instance in testplan.instances.keys()] == \
           expected_subset


def test_testplan_handle_modules():
    testplan = TestPlan(env=mock.Mock())

    modules = [mock.Mock(meta={'name': 'name1'}),
               mock.Mock(meta={'name': 'name2'})]

    with mock.patch('twisterlib.testplan.parse_modules', return_value=modules):
        testplan.handle_modules()

    assert testplan.modules == ['name1', 'name2']


TESTDATA_6 = [
    (True, False, False, 0, 'report_test_tree'),
    (True, True, False, 0, 'report_test_tree'),
    (True, False, True, 0, 'report_test_tree'),
    (True, True, True, 0, 'report_test_tree'),
    (False, True, False, 0, 'report_test_list'),
    (False, True, True, 0, 'report_test_list'),
    (False, False, True, 0, 'report_tag_list'),
    (False, False, False, 1, None),
]

@pytest.mark.parametrize(
    'test_tree, list_tests, list_tags, expected_res, expected_method',
    TESTDATA_6,
    ids=['test tree', 'test tree + test list', 'test tree + tag list',
         'test tree + test list + tag list', 'test list',
         'test list + tag list', 'tag list', 'no report']
)
def test_testplan_report(
    test_tree,
    list_tests,
    list_tags,
    expected_res,
    expected_method
):
    testplan = TestPlan(env=mock.Mock())
    testplan.report_test_tree = mock.Mock()
    testplan.report_test_list = mock.Mock()
    testplan.report_tag_list = mock.Mock()

    testplan.options = mock.Mock(
        test_tree=test_tree,
        list_tests=list_tests,
        list_tags=list_tags,
    )

    res = testplan.report()

    assert res == expected_res

    methods = ['report_test_tree', 'report_test_list', 'report_tag_list']
    if expected_method:
        methods.remove(expected_method)
        getattr(testplan, expected_method).assert_called_once()
    for method in methods:
        getattr(testplan, method).assert_not_called()


TESTDATA_7 = [
    (
        [
            mock.Mock(
                yamlfile='a.yaml',
                scenarios=['scenario1', 'scenario2']
            ),
            mock.Mock(
                yamlfile='b.yaml',
                scenarios=['scenario1']
            )
        ],
        TwisterRuntimeError,
        'Duplicated test scenarios found:\n' \
        '- scenario1 found in:\n' \
        '  - a.yaml\n' \
        '  - b.yaml\n',
        []
    ),
    (
        [
            mock.Mock(
                yamlfile='a.yaml',
                scenarios=['scenario.a.1', 'scenario.a.2']
            ),
            mock.Mock(
                yamlfile='b.yaml',
                scenarios=['scenario.b.1']
            )
        ],
        None,
        None,
        ['No duplicates found.']
    ),
]

@pytest.mark.parametrize(
    'testsuites, expected_error, error_msg, expected_logs',
    TESTDATA_7,
    ids=['a duplicate', 'no duplicates']
)
def test_testplan_report_duplicates(
    capfd,
    caplog,
    testsuites,
    expected_error,
    error_msg,
    expected_logs
):
    def mock_get(name):
        return list(filter(lambda x: name in x.scenarios, testsuites))

    testplan = TestPlan(env=mock.Mock())
    testplan.scenarios = [scenario for testsuite in testsuites \
                                   for scenario in testsuite.scenarios]
    testplan.get_testsuite = mock.Mock(side_effect=mock_get)

    with pytest.raises(expected_error) if expected_error is not None else \
         nullcontext() as err:
        testplan.report_duplicates()

    if expected_error:
        assert str(err._excinfo[1]) == error_msg

    assert all([log in caplog.text for log in expected_logs])


def test_testplan_report_tag_list(capfd):
    testplan = TestPlan(env=mock.Mock())
    testplan.testsuites = {
        'testsuite0': mock.Mock(tags=set(['tag1', 'tag2'])),
        'testsuite1': mock.Mock(tags=set(['tag1', 'tag2', 'tag3'])),
        'testsuite2': mock.Mock(tags=set(['tag1', 'tag3'])),
        'testsuite3': mock.Mock(tags=set(['tag']))
    }

    testplan.report_tag_list()

    out,err = capfd.readouterr()
    sys.stdout.write(out)
    sys.stderr.write(err)

    assert '- tag' in out
    assert '- tag1' in out
    assert '- tag2' in out
    assert '- tag3' in out


def test_testplan_report_test_tree(capfd):
    testplan = TestPlan(env=mock.Mock())
    testplan.get_all_tests = mock.Mock(
        return_value=['1.dummy.case.1', '1.dummy.case.2',
                      '2.dummy.case.1', '2.dummy.case.2',
                      '3.dummy.case.1', '3.dummy.case.2',
                      '4.dummy.case.1', '4.dummy.case.2',
                      '5.dummy.case.1', '5.dummy.case.2',
                      'sample.group1.case1', 'sample.group1.case2',
                      'sample.group2.case', 'sample.group3.case1',
                      'sample.group3.case2', 'sample.group3.case3']
    )

    testplan.report_test_tree()

    out,err = capfd.readouterr()
    sys.stdout.write(out)
    sys.stderr.write(err)

    expected = """
Testsuite
├── Samples
│   ├── group1
│   │   ├── sample.group1.case1
│   │   └── sample.group1.case2
│   ├── group2
│   │   └── sample.group2.case
│   └── group3
│       ├── sample.group3.case1
│       ├── sample.group3.case2
│       └── sample.group3.case3
└── Tests
    ├── 1
    │   └── dummy
    │       ├── 1.dummy.case.1
    │       └── 1.dummy.case.2
    ├── 2
    │   └── dummy
    │       ├── 2.dummy.case.1
    │       └── 2.dummy.case.2
    ├── 3
    │   └── dummy
    │       ├── 3.dummy.case.1
    │       └── 3.dummy.case.2
    ├── 4
    │   └── dummy
    │       ├── 4.dummy.case.1
    │       └── 4.dummy.case.2
    └── 5
        └── dummy
            ├── 5.dummy.case.1
            └── 5.dummy.case.2
"""
    expected = expected[1:]

    assert expected in out


def test_testplan_report_test_list(capfd):
    testplan = TestPlan(env=mock.Mock())
    testplan.get_all_tests = mock.Mock(
        return_value=['4.dummy.case.1', '4.dummy.case.2',
                      '3.dummy.case.2', '2.dummy.case.2',
                      '1.dummy.case.1', '1.dummy.case.2',
                      '3.dummy.case.1', '2.dummy.case.1',
                      '5.dummy.case.1', '5.dummy.case.2']
    )

    testplan.report_test_list()

    out,err = capfd.readouterr()
    sys.stdout.write(out)
    sys.stderr.write(err)

    assert ' - 1.dummy.case.1\n' \
           ' - 1.dummy.case.2\n' \
           ' - 2.dummy.case.1\n' \
           ' - 2.dummy.case.2\n' \
           ' - 3.dummy.case.1\n' \
           ' - 3.dummy.case.2\n' \
           ' - 4.dummy.case.1\n' \
           ' - 4.dummy.case.2\n' \
           ' - 5.dummy.case.1\n' \
           ' - 5.dummy.case.2\n' \
           '10 total.' in out


def test_testplan_config(caplog):
    testplan = TestPlan(env=mock.Mock())
    testplan.coverage_platform = 'dummy cov'

    testplan.config()

    assert 'coverage platform: dummy cov' in caplog.text


def test_testplan_info(capfd):
    TestPlan.info('dummy text')

    out, err = capfd.readouterr()
    sys.stdout.write(out)
    sys.stderr.write(err)

    assert 'dummy text\n' in out


TESTDATA_8 = [
    (False, False, ['p1e2', 'p2', 'p3', 'p3@B'], ['p2']),
    (False, True, None, None),
    (True, False, ['p1e2', 'p2', 'p3', 'p3@B'], ['p3']),
]

@pytest.mark.parametrize(
    'override_default_platforms, create_duplicate, expected_platform_names, expected_defaults',
    TESTDATA_8,
    ids=['no override defaults', 'create duplicate', 'override defaults']
)
def test_testplan_add_configurations(
    tmp_path,
    override_default_platforms,
    create_duplicate,
    expected_platform_names,
    expected_defaults
):
    # tmp_path
    # └ boards  <- board root
    #   ├ arch1
    #   │ ├ p1
    #   │ | ├ p1e1.yaml
    #   │ | └ p1e2.yaml
    #   │ └ p2
    #   │   ├ p2.yaml
    #   │   └ p2-1.yaml <- duplicate
    #   │   └ p2-2.yaml <- load error
    #   └ arch2
    #     └ p3
    #       ├ p3.yaml
    #       └ p3_B.conf

    tmp_board_root_dir = tmp_path / 'boards'
    tmp_board_root_dir.mkdir()

    tmp_arch1_dir = tmp_board_root_dir / 'arch1'
    tmp_arch1_dir.mkdir()

    tmp_p1_dir = tmp_arch1_dir / 'p1'
    tmp_p1_dir.mkdir()

    p1e1_bs_yaml = """\
boards:

  - name: ple1
    vendor: zephyr
    socs:
      - name: unit_testing
  - name: ple2
    vendor: zephyr
    socs:
      - name: unit_testing
"""
    p1e1_yamlfile = tmp_p1_dir / 'board.yml'
    p1e1_yamlfile.write_text(p1e1_bs_yaml)

    p1e1_yaml = """\
identifier: p1e1
name: Platform 1 Edition 1
type: native
arch: arch1
vendor: vendor1
toolchain:
  - zephyr
twister: False
"""
    p1e1_yamlfile = tmp_p1_dir / 'p1e1.yaml'
    p1e1_yamlfile.write_text(p1e1_yaml)

    p1e2_yaml = """\
identifier: p1e2
name: Platform 1 Edition 2
type: native
arch: arch1
vendor: vendor1
toolchain:
  - zephyr
"""
    p1e2_yamlfile = tmp_p1_dir / 'p1e2.yaml'
    p1e2_yamlfile.write_text(p1e2_yaml)

    tmp_p2_dir = tmp_arch1_dir / 'p2'
    tmp_p2_dir.mkdir()

    p2_bs_yaml = """\
boards:

  - name: p2
    vendor: zephyr
    socs:
      - name: unit_testing
  - name: p2_2
    vendor: zephyr
    socs:
      - name: unit_testing
"""
    p2_yamlfile = tmp_p2_dir / 'board.yml'
    p2_yamlfile.write_text(p2_bs_yaml)

    p2_yaml = """\
identifier: p2
name: Platform 2
type: sim
arch: arch1
vendor: vendor2
toolchain:
  - zephyr
testing:
  default: True
"""
    p2_yamlfile = tmp_p2_dir / 'p2.yaml'
    p2_yamlfile.write_text(p2_yaml)

    if create_duplicate:
        p2_yamlfile = tmp_p2_dir / 'p2-1.yaml'
        p2_yamlfile.write_text(p2_yaml)

    p2_2_yaml = """\
testing:
  ć#@%!#!#^#@%@:1.0
identifier: p2_2
name: Platform 2 2
type: sim
arch: arch1
vendor: vendor2
toolchain:
  - zephyr
"""
    p2_2_yamlfile = tmp_p2_dir / 'p2-2.yaml'
    p2_2_yamlfile.write_text(p2_2_yaml)

    tmp_arch2_dir = tmp_board_root_dir / 'arch2'
    tmp_arch2_dir.mkdir()

    tmp_p3_dir = tmp_arch2_dir / 'p3'
    tmp_p3_dir.mkdir()

    p3_bs_yaml = """\
boards:

  - name: p3
    vendor: zephyr
    socs:
      - name: unit_testing
"""
    p3_yamlfile = tmp_p3_dir / 'board.yml'
    p3_yamlfile.write_text(p3_bs_yaml)

    p3_yaml = """\
identifier: p3
name: Platform 3
type: unit
arch: arch2
vendor: vendor3
toolchain:
  - zephyr
"""
    p3_yamlfile = tmp_p3_dir / 'p3.yaml'
    p3_yamlfile.write_text(p3_yaml)
    p3_yamlfile = tmp_p3_dir / 'p3_B.conf'
    p3_yamlfile.write_text('')

    env = mock.Mock(board_roots=[tmp_board_root_dir])

    testplan = TestPlan(env=env)

    testplan.test_config = {
        'platforms': {
            'override_default_platforms': override_default_platforms,
            'default_platforms': ['p3', 'p1e1']
        }
    }

    with pytest.raises(Exception) if create_duplicate else nullcontext():
        testplan.add_configurations()

    if expected_defaults is not None:
        assert sorted(expected_defaults) == sorted(testplan.default_platforms)
    if expected_platform_names is not None:
        assert sorted(expected_platform_names) == sorted(testplan.platform_names)


def test_testplan_get_all_tests():
    testplan = TestPlan(env=mock.Mock())
    tc1 = mock.Mock()
    tc1.name = 'tc1'
    tc2 = mock.Mock()
    tc2.name = 'tc2'
    tc3 = mock.Mock()
    tc3.name = 'tc3'
    tc4 = mock.Mock()
    tc4.name = 'tc4'
    tc5 = mock.Mock()
    tc5.name = 'tc5'
    ts1 = mock.Mock(testcases=[tc1, tc2])
    ts2 = mock.Mock(testcases=[tc3, tc4, tc5])
    testplan.testsuites = {
        'ts1': ts1,
        'ts2': ts2
    }

    res = testplan.get_all_tests()

    assert sorted(res) == ['tc1', 'tc2', 'tc3', 'tc4', 'tc5']


TESTDATA_9 = [
    ([], False, 7),
    ([], True, 5),
    (['good_test/dummy.common.1', 'good_test/dummy.common.2', 'good_test/dummy.common.3'], False, 3),
    (['good_test/dummy.common.1', 'good_test/dummy.common.2', 'good_test/dummy.common.3'], True, 0),
]

@pytest.mark.parametrize(
    'testsuite_filter, use_alt_root, expected_suite_count',
    TESTDATA_9,
    ids=['no testsuite filter', 'no testsuite filter, alt root',
         'testsuite filter', 'testsuite filter, alt root']
)
def test_testplan_add_testsuites(tmp_path, testsuite_filter, use_alt_root, expected_suite_count):
    # tmp_path
    # ├ tests  <- test root
    # │ ├ good_test
    # │ │ └ testcase.yaml
    # │ ├ wrong_test
    # │ │ └ testcase.yaml
    # │ ├ good_sample
    # │ │ └ sample.yaml
    # │ └ others
    # │   └ other.txt
    # └ other_tests  <- alternate test root
    #   └ good_test
    #     └ testcase.yaml
    tmp_test_root_dir = tmp_path / 'tests'
    tmp_test_root_dir.mkdir()

    tmp_good_test_dir = tmp_test_root_dir / 'good_test'
    tmp_good_test_dir.mkdir()
    testcase_yaml_1 = """\
tests:
  dummy.common.1:
    build_on_all: true
  dummy.common.2:
    build_on_all: true
  dummy.common.3:
    build_on_all: true
  dummy.special:
    build_on_all: false
"""
    testfile_1 = tmp_good_test_dir / 'testcase.yaml'
    testfile_1.write_text(testcase_yaml_1)

    tmp_bad_test_dir = tmp_test_root_dir / 'wrong_test'
    tmp_bad_test_dir.mkdir()
    testcase_yaml_2 = """\
tests:
 wrong:
  yaml: {]}
"""
    testfile_2 = tmp_bad_test_dir / 'testcase.yaml'
    testfile_2.write_text(testcase_yaml_2)

    tmp_good_sample_dir = tmp_test_root_dir / 'good_sample'
    tmp_good_sample_dir.mkdir()
    samplecase_yaml_1 = """\
tests:
  sample.dummy.common.1:
    tags:
    - samples
  sample.dummy.common.2:
    tags:
    - samples
  sample.dummy.special.1:
    tags:
    - samples
"""
    samplefile_1 = tmp_good_sample_dir / 'sample.yaml'
    samplefile_1.write_text(samplecase_yaml_1)

    tmp_other_dir = tmp_test_root_dir / 'others'
    tmp_other_dir.mkdir()
    _ = tmp_other_dir / 'other.txt'

    tmp_alt_test_root_dir = tmp_path / 'other_tests'
    tmp_alt_test_root_dir.mkdir()

    tmp_alt_good_test_dir = tmp_alt_test_root_dir / 'good_test'
    tmp_alt_good_test_dir.mkdir()
    testcase_yaml_3 = """\
tests:
  dummy.alt.1:
    build_on_all: true
  dummy.alt.2:
    build_on_all: true
"""
    testfile_3 = tmp_alt_good_test_dir / 'testcase.yaml'
    testfile_3.write_text(testcase_yaml_3)

    env = mock.Mock(
        test_roots=[tmp_test_root_dir],
        alt_config_root=[tmp_alt_test_root_dir] if use_alt_root else []
    )

    testplan = TestPlan(env=env)

    res = testplan.add_testsuites(testsuite_filter)

    assert res == expected_suite_count


def test_testplan_str():
    testplan = TestPlan(env=mock.Mock())
    testplan.name = 'my name'

    res = testplan.__str__()

    assert res == 'my name'


TESTDATA_10 = [
    ('a platform', True),
    ('other platform', False),
]

@pytest.mark.parametrize(
    'name, expect_found',
    TESTDATA_10,
    ids=['platform exists', 'no platform']
)
def test_testplan_get_platform(name, expect_found):
    testplan = TestPlan(env=mock.Mock())
    p1 = mock.Mock()
    p1.name = 'some platform'
    p2 = mock.Mock()
    p2.name = 'a platform'
    testplan.platforms = [p1, p2]

    res = testplan.get_platform(name)

    if expect_found:
        assert res.name == name
    else:
        assert res is None


TESTDATA_11 = [
    (True, 'runnable'),
    (False, 'buildable'),
]

@pytest.mark.parametrize(
    'device_testing, expected_tfilter',
    TESTDATA_11,
    ids=['device testing', 'no device testing']
)
def test_testplan_load_from_file(caplog, device_testing, expected_tfilter):
    def get_platform(name):
        p = mock.Mock()
        p.name = name
        p.normalized_name = name
        return p

    ts1tc1 = mock.Mock()
    ts1tc1.name = 'TS1.tc1'
    ts1 = mock.Mock(testcases=[ts1tc1])
    ts1.name = 'TestSuite 1'
    ts2 = mock.Mock(testcases=[])
    ts2.name = 'TestSuite 2'
    ts3tc1 = mock.Mock()
    ts3tc1.name = 'TS3.tc1'
    ts3tc2 = mock.Mock()
    ts3tc2.name = 'TS3.tc2'
    ts3 = mock.Mock(testcases=[ts3tc1, ts3tc2])
    ts3.name = 'TestSuite 3'
    ts4tc1 = mock.Mock()
    ts4tc1.name = 'TS4.tc1'
    ts4 = mock.Mock(testcases=[ts4tc1])
    ts4.name = 'TestSuite 4'
    ts5 = mock.Mock(testcases=[])
    ts5.name = 'TestSuite 5'

    testplan = TestPlan(env=mock.Mock(outdir=os.path.join('out', 'dir')))
    testplan.options = mock.Mock(device_testing=device_testing, test_only=True)
    testplan.testsuites = {
        'TestSuite 1': ts1,
        'TestSuite 2': ts2,
        'TestSuite 3': ts3,
        'TestSuite 4': ts4,
        'TestSuite 5': ts5
    }

    testplan.get_platform = mock.Mock(side_effect=get_platform)

    testplan_data = """\
{
    "testsuites": [
        {
            "name": "TestSuite 1",
            "platform": "Platform 1",
            "run_id": 1,
            "execution_time": 60.00,
            "used_ram": 4096,
            "available_ram": 12278,
            "used_rom": 1024,
            "available_rom": 1047552,
            "status": "passed",
            "reason": "OK",
            "testcases": [
                {
                    "identifier": "TS1.tc1",
                    "status": "passed",
                    "reason": "passed",
                    "execution_time": 60.00,
                    "log": ""
                }
            ]
        },
        {
            "name": "TestSuite 2",
            "platform": "Platform 1"
        },
        {
            "name": "TestSuite 3",
            "platform": "Platform 1",
            "run_id": 1,
            "execution_time": 360.00,
            "used_ram": 4096,
            "available_ram": 12278,
            "used_rom": 1024,
            "available_rom": 1047552,
            "status": "error",
            "reason": "File Not Found Error",
            "testcases": [
                {
                    "identifier": "TS3.tc1",
                    "status": "error",
                    "reason": "File Not Found Error.",
                    "execution_time": 360.00,
                    "log": "[ERROR]: File 'dummy.yaml' not found!\\nClosing..."
                },
                {
                    "identifier": "TS3.tc2"
                }
            ]
        },
        {
            "name": "TestSuite 4",
            "platform": "Platform 1",
            "execution_time": 360.00,
            "used_ram": 4096,
            "available_ram": 12278,
            "used_rom": 1024,
            "available_rom": 1047552,
            "status": "skipped",
            "reason": "Not in requested test list.",
            "testcases": [
                {
                    "identifier": "TS4.tc1",
                    "status": "skipped",
                    "reason": "Not in requested test list.",
                    "execution_time": 360.00,
                    "log": "[INFO] Parsing..."
                },
                {
                    "identifier": "TS3.tc2"
                }
            ]
        },
        {
            "name": "TestSuite 5",
            "platform": "Platform 2"
        }
    ]
}
"""

    filter_platform = ['Platform 1']

    check_runnable_mock = mock.Mock(return_value=True)

    with mock.patch('builtins.open', mock.mock_open(read_data=testplan_data)), \
         mock.patch('twisterlib.testinstance.TestInstance.check_runnable', check_runnable_mock), \
         mock.patch('twisterlib.testinstance.TestInstance.create_overlay', mock.Mock()):
        testplan.load_from_file('dummy.yaml', filter_platform)

    expected_instances = {
        'Platform 1/TestSuite 1': {
            'metrics': {
                'handler_time': 60.0,
                'used_ram': 4096,
                'used_rom': 1024,
                'available_ram': 12278,
                'available_rom': 1047552
            },
            'retries': 0,
            'testcases': {
                'TS1.tc1': {
                    'status': 'passed',
                    'reason': None,
                    'duration': 60.0,
                    'output': ''
                }
            }
        },
        'Platform 1/TestSuite 2': {
            'metrics': {
                'handler_time': 0,
                'used_ram': 0,
                'used_rom': 0,
                'available_ram': 0,
                'available_rom': 0
            },
            'retries': 0,
            'testcases': []
        },
        'Platform 1/TestSuite 3': {
            'metrics': {
                'handler_time': 360.0,
                'used_ram': 4096,
                'used_rom': 1024,
                'available_ram': 12278,
                'available_rom': 1047552
            },
            'retries': 1,
            'testcases': {
                    'TS3.tc1': {
                        'status': 'error',
                        'reason': None,
                        'duration': 360.0,
                        'output': '[ERROR]: File \'dummy.yaml\' not found!\nClosing...'
                    },
                    'TS3.tc2': {
                        'status': None,
                        'reason': None,
                        'duration': 0,
                        'output': ''
                    }
            }
        },
        'Platform 1/TestSuite 4': {
            'metrics': {
                'handler_time': 360.0,
                'used_ram': 4096,
                'used_rom': 1024,
                'available_ram': 12278,
                'available_rom': 1047552
            },
            'retries': 0,
            'testcases': {
                'TS4.tc1': {
                    'status': 'skipped',
                    'reason': 'Not in requested test list.',
                    'duration': 360.0,
                    'output': '[INFO] Parsing...'
                }
            }
        },
    }

    for n, i in testplan.instances.items():
        assert expected_instances[n]['metrics'] == i.metrics
        assert expected_instances[n]['retries'] == i.retries
        for t in i.testcases:
            assert expected_instances[n]['testcases'][str(t)]['status'] == t.status
            assert expected_instances[n]['testcases'][str(t)]['reason'] == t.reason
            assert expected_instances[n]['testcases'][str(t)]['duration'] == t.duration
            assert expected_instances[n]['testcases'][str(t)]['output'] == t.output

    check_runnable_mock.assert_called_with(mock.ANY, expected_tfilter, mock.ANY, mock.ANY)

    expected_logs = [
        'loading TestSuite 1...',
        'loading TestSuite 2...',
        'loading TestSuite 3...',
        'loading TestSuite 4...',
    ]
    assert all([log in caplog.text for log in expected_logs])


def test_testplan_add_instances():
    testplan = TestPlan(env=mock.Mock())
    instance1 = mock.Mock()
    instance1.name = 'instance 1'
    instance2 = mock.Mock()
    instance2.name = 'instance 2'
    instance_list = [instance1, instance2]

    testplan.add_instances(instance_list)

    assert testplan.instances == {
        'instance 1': instance1,
        'instance 2': instance2,
    }


def test_testplan_get_testsuite():
    testplan = TestPlan(env=mock.Mock())
    testplan.testsuites = {
        'testsuite0': mock.Mock(testcases=[mock.Mock(), mock.Mock()]),
        'testsuite1': mock.Mock(testcases=[mock.Mock()]),
        'testsuite2': mock.Mock(testcases=[mock.Mock(), mock.Mock()]),
        'testsuite3': mock.Mock(testcases=[])
    }
    testplan.testsuites['testsuite0'].testcases[0].name = 'testcase name 0'
    testplan.testsuites['testsuite0'].testcases[1].name = 'testcase name 1'
    testplan.testsuites['testsuite1'].testcases[0].name = 'sample id'
    testplan.testsuites['testsuite2'].testcases[0].name = 'dummy id'
    testplan.testsuites['testsuite2'].testcases[1].name = 'sample id'

    id = 'sample id'

    res = testplan.get_testsuite(id)

    assert len(res) == 2
    assert testplan.testsuites['testsuite1'] in res
    assert testplan.testsuites['testsuite2'] in res


def test_testplan_verify_platforms_existence(caplog):
    testplan = TestPlan(env=mock.Mock())
    testplan.platform_names = ['a platform', 'other platform']

    platform_names = ['other platform', 'some platform']
    log_info = 'PLATFORM ERROR'

    with pytest.raises(SystemExit) as se:
        testplan.verify_platforms_existence(platform_names, log_info)

    assert str(se.value) == '2'
    assert 'PLATFORM ERROR - unrecognized platform - some platform'


TESTDATA_12 = [
    (True),
    (False)
]

@pytest.mark.parametrize(
    'exists',
    TESTDATA_12,
    ids=['links dir exists', 'links dir does not exist']
)
def test_testplan_create_build_dir_links(exists):
    outdir = os.path.join('out', 'dir')
    instances_linked = []

    def mock_link(links_dir_path, instance):
        assert links_dir_path == os.path.join(outdir, 'twister_links')
        instances_linked.append(instance)

    instances = {
        'inst0': mock.Mock(status='passed'),
        'inst1': mock.Mock(status='skipped'),
        'inst2': mock.Mock(status='error'),
    }
    expected_instances = [instances['inst0'], instances['inst2']]

    testplan = TestPlan(env=mock.Mock(outdir=outdir))
    testplan._create_build_dir_link = mock.Mock(side_effect=mock_link)
    testplan.instances = instances

    with mock.patch('os.path.exists', return_value=exists), \
         mock.patch('os.mkdir', mock.Mock()) as mkdir_mock:
        testplan.create_build_dir_links()

    if not exists:
        mkdir_mock.assert_called_once()

    assert expected_instances == instances_linked


TESTDATA_13 = [
    ('nt'),
    ('Linux')
]

@pytest.mark.parametrize(
    'os_name',
    TESTDATA_13,
)
def test_testplan_create_build_dir_link(os_name):
    def mock_makedirs(path, exist_ok=False):
        assert exist_ok
        assert path == instance_build_dir

    def mock_symlink(source, target):
        assert source == instance_build_dir
        assert target == os.path.join('links', 'path', 'test_0')

    def mock_call(cmd, shell=False):
        assert shell
        assert cmd == ['mklink', '/J', os.path.join('links', 'path', 'test_0'),
                       instance_build_dir]

    def mock_join(*paths):
        slash = "\\" if os.name == 'nt' else "/"
        return slash.join(paths)

    with mock.patch('os.name', os_name), \
         mock.patch('os.symlink', side_effect=mock_symlink), \
         mock.patch('os.makedirs', side_effect=mock_makedirs), \
         mock.patch('subprocess.call', side_effect=mock_call), \
         mock.patch('os.path.join', side_effect=mock_join):

        testplan = TestPlan(env=mock.Mock())
        links_dir_path = os.path.join('links', 'path')
        instance_build_dir = os.path.join('some', 'far', 'off', 'build', 'dir')
        instance = mock.Mock(build_dir=instance_build_dir)
        testplan._create_build_dir_link(links_dir_path, instance)

        assert instance.build_dir == os.path.join('links', 'path', 'test_0')
        assert testplan.link_dir_counter == 1


TESTDATA_14 = [
    ('bad platform', 'dummy reason', [],
     'dummy status', 'dummy reason'),
    ('good platform', 'quarantined', [],
     'error', 'quarantined but is one of the integration platforms'),
    ('good platform', 'dummy reason', [{'type': 'command line filter'}],
     'dummy status', 'dummy reason'),
    ('good platform', 'dummy reason', [{'type': 'Skip filter'}],
     'dummy status', 'dummy reason'),
    ('good platform', 'dummy reason', [{'type': 'platform key filter'}],
     'dummy status', 'dummy reason'),
    ('good platform', 'dummy reason', [{'type': 'Toolchain filter'}],
     'dummy status', 'dummy reason'),
    ('good platform', 'dummy reason', [{'type': 'Module filter'}],
     'dummy status', 'dummy reason'),
    ('good platform', 'dummy reason', [{'type': 'testsuite filter'}],
     'error', 'dummy reason but is one of the integration platforms'),
]

@pytest.mark.parametrize(
    'platform_name, reason, filters,' \
    ' expected_status, expected_reason',
    TESTDATA_14,
    ids=['wrong platform', 'quarantined', 'command line filtered',
         'skip filtered', 'platform key filtered', 'toolchain filtered',
         'module filtered', 'skip to error change']
)
def test_change_skip_to_error_if_integration(
    platform_name,
    reason,
    filters,
    expected_status,
    expected_reason
):
    options = mock.Mock()
    platform = mock.Mock()
    platform.name = platform_name
    testsuite = mock.Mock(integration_platforms=['good platform', 'a platform'])
    instance = mock.Mock(
        testsuite=testsuite,
        platform=platform,
        filters=filters,
        status='dummy status',
        reason=reason
    )

    change_skip_to_error_if_integration(options, instance)

    assert instance.status == expected_status
    assert instance.reason == expected_reason
