#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=line-too-long
"""
Tests for testinstance class
"""

from contextlib import nullcontext
import os
import sys
import pytest
import mock

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

from twisterlib.statuses import TwisterStatus
from twisterlib.testinstance import TestInstance
from twisterlib.error import BuildError
from twisterlib.runner import TwisterRunner
from twisterlib.handlers import QEMUHandler
from expr_parser import reserved


TESTDATA_PART_1 = [
    (False, False, "console", "na", "qemu", False, [], (False, True)),
    (False, False, "console", "native", "qemu", False, [], (False, True)),
    (True, False, "console", "native", "nsim", False, [], (True, False)),
    (True, True, "console", "native", "renode", False, [], (True, False)),
    (False, False, "sensor", "native", "", False, [], (True, False)),
    (False, False, "sensor", "na", "", False, [], (True, False)),
    (False, True, "sensor", "native", "", True, [], (True, False)),
]
@pytest.mark.parametrize(
    "build_only, slow, harness, platform_type, platform_sim, device_testing,fixture, expected",
    TESTDATA_PART_1
)
def test_check_build_or_run(
    class_testplan,
    all_testsuites_dict,
    platforms_list,
    build_only,
    slow,
    harness,
    platform_type,
    platform_sim,
    device_testing,
    fixture,
    expected
):
    """" Test to check the conditions for build_only and run scenarios
    Scenario 1: Test when different parameters are passed, build_only and run are set correctly
    Scenario 2: Test if build_only is enabled when the OS is Windows"""

    class_testplan.testsuites = all_testsuites_dict
    testsuite = class_testplan.testsuites.get('scripts/tests/twister/test_data/testsuites/tests/'
                                              'test_a/test_a.check_1')
    print(testsuite)

    class_testplan.platforms = platforms_list
    platform = class_testplan.get_platform("demo_board_2")
    platform.type = platform_type
    platform.simulation = platform_sim
    testsuite.harness = harness
    testsuite.build_only = build_only
    testsuite.slow = slow

    testinstance = TestInstance(testsuite, platform, class_testplan.env.outdir)
    env = mock.Mock(
        options=mock.Mock(
            device_testing=False,
            enable_slow=slow,
            fixtures=fixture,
            filter=""
        )
    )
    run = testinstance.check_runnable(env.options)
    _, r = expected
    assert run == r

    with mock.patch('os.name', 'nt'):
        # path to QEMU binary is not in QEMU_BIN_PATH environment variable
        run = testinstance.check_runnable(env.options)
        assert not run

        # mock path to QEMU binary in QEMU_BIN_PATH environment variable
        with mock.patch('os.environ', {'QEMU_BIN_PATH': ''}):
            run = testinstance.check_runnable(env.options)
            _, r = expected
            assert run == r


TESTDATA_PART_2 = [
    (True, True, True, ["demo_board_2/unit_testing"], "native",
     None, '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y\nCONFIG_ASAN=y\nCONFIG_UBSAN=y'),
    (True, False, True, ["demo_board_2/unit_testing"], "native",
     None, '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y\nCONFIG_ASAN=y'),
    (False, False, True, ["demo_board_2/unit_testing"], 'native',
     None, '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y'),
    (True, False, True, ["demo_board_2/unit_testing"], 'mcu',
     None, '\nCONFIG_COVERAGE=y\nCONFIG_COVERAGE_DUMP=y'),
    (False, False, False, ["demo_board_2/unit_testing"], 'native', None, ''),
    (False, False, True, ['demo_board_1'], 'native', None, ''),
    (True, False, False, ["demo_board_2"], 'native', None, '\nCONFIG_ASAN=y'),
    (False, True, False, ["demo_board_2"], 'native', None, '\nCONFIG_UBSAN=y'),
    (False, False, False, ["demo_board_2"], 'native',
     ["CONFIG_LOG=y"], 'CONFIG_LOG=y'),
    (False, False, False, ["demo_board_2"], 'native',
     ["arch:x86:CONFIG_LOG=y"], 'CONFIG_LOG=y'),
    (False, False, False, ["demo_board_2"], 'native',
     ["arch:arm:CONFIG_LOG=y"], ''),
    (False, False, False, ["demo_board_2"], 'native',
     ["platform:demo_board_2/unit_testing:CONFIG_LOG=y"], 'CONFIG_LOG=y'),
    (False, False, False, ["demo_board_2"], 'native',
     ["platform:demo_board_1:CONFIG_LOG=y"], ''),
]

@pytest.mark.parametrize(
    'enable_asan, enable_ubsan, enable_coverage, coverage_platform, platform_type,'
    ' extra_configs, expected_content',
    TESTDATA_PART_2
)
def test_create_overlay(
    class_testplan,
    all_testsuites_dict,
    platforms_list,
    enable_asan,
    enable_ubsan,
    enable_coverage,
    coverage_platform,
    platform_type,
    extra_configs,
    expected_content
):
    """Test correct content is written to testcase_extra.conf based on if conditions."""
    class_testplan.testsuites = all_testsuites_dict
    testcase = class_testplan.testsuites.get('scripts/tests/twister/test_data/testsuites/samples/'
                                             'test_app/sample_test.app')

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
    testcase = class_testplan.testsuites.get('scripts/tests/twister/test_data/testsuites/samples/'
                                             'test_app/sample_test.app')
    class_testplan.platforms = platforms_list
    platform = class_testplan.get_platform("demo_board_2")
    testinstance = TestInstance(testcase, platform, class_testplan.env.outdir)

    with pytest.raises(BuildError):
        assert testinstance.calculate_sizes() == "Missing/multiple output ELF binary"

TESTDATA_PART_3 = [
    (
        'CONFIG_ARCH_HAS_THREAD_LOCAL_STORAGE and' \
        ' CONFIG_TOOLCHAIN_SUPPORTS_THREAD_LOCAL_STORAGE and' \
        ' not (CONFIG_TOOLCHAIN_ARCMWDT_SUPPORTS_THREAD_LOCAL_STORAGE and CONFIG_USERSPACE)',
        ['kconfig']
    ),
    (
        '(dt_compat_enabled("st,stm32-flash-controller") or' \
        ' dt_compat_enabled("st,stm32h7-flash-controller")) and' \
        ' dt_label_with_parent_compat_enabled("storage_partition", "fixed-partitions")',
        ['dts']
    ),
    (
        '((CONFIG_FLASH_HAS_DRIVER_ENABLED and not CONFIG_TRUSTED_EXECUTION_NONSECURE) and' \
        ' dt_label_with_parent_compat_enabled("storage_partition", "fixed-partitions")) or' \
        ' (CONFIG_FLASH_HAS_DRIVER_ENABLED and CONFIG_TRUSTED_EXECUTION_NONSECURE and' \
        ' dt_label_with_parent_compat_enabled("slot1_ns_partition", "fixed-partitions"))',
        ['dts', 'kconfig']
    ),
    (
        '((CONFIG_CPU_AARCH32_CORTEX_R or CONFIG_CPU_CORTEX_M) and' \
        ' CONFIG_CPU_HAS_FPU and TOOLCHAIN_HAS_NEWLIB == 1) or CONFIG_ARCH_POSIX',
        ['full']
    )
]

@pytest.mark.parametrize("filter_expr, expected_stages", TESTDATA_PART_3)
def test_which_filter_stages(filter_expr, expected_stages):
    logic_keys = reserved.keys()
    stages = TwisterRunner.get_cmake_filter_stages(filter_expr, logic_keys)
    assert sorted(stages) == sorted(expected_stages)


@pytest.fixture(name='testinstance')
def sample_testinstance(all_testsuites_dict, class_testplan, platforms_list, request):
    testsuite_path = 'scripts/tests/twister/test_data/testsuites'
    if request.param['testsuite_kind']  == 'sample':
        testsuite_path += '/samples/test_app/sample_test.app'
    elif request.param['testsuite_kind'] == 'tests':
        testsuite_path += '/tests/test_a/test_a.check_1'

    class_testplan.testsuites = all_testsuites_dict
    testsuite = class_testplan.testsuites.get(testsuite_path)
    class_testplan.platforms = platforms_list
    platform = class_testplan.get_platform(request.param.get('board_name', 'demo_board_2'))

    testinstance = TestInstance(testsuite, platform, class_testplan.env.outdir)
    return testinstance


TESTDATA_1 = [
    (False),
    (True),
]

@pytest.mark.parametrize('detailed_test_id', TESTDATA_1)
def test_testinstance_init(all_testsuites_dict, class_testplan, platforms_list, detailed_test_id):
    testsuite_path = 'scripts/tests/twister/test_data/testsuites/samples/test_app/sample_test.app'
    class_testplan.testsuites = all_testsuites_dict
    testsuite = class_testplan.testsuites.get(testsuite_path)
    testsuite.detailed_test_id = detailed_test_id
    class_testplan.platforms = platforms_list
    platform = class_testplan.get_platform("demo_board_2/unit_testing")

    testinstance = TestInstance(testsuite, platform, class_testplan.env.outdir)

    if detailed_test_id:
        assert testinstance.build_dir == os.path.join(class_testplan.env.outdir, platform.normalized_name, testsuite_path)
    else:
        assert testinstance.build_dir == os.path.join(class_testplan.env.outdir, platform.normalized_name, testsuite.source_dir_rel, testsuite.name)


@pytest.mark.parametrize('testinstance', [{'testsuite_kind': 'sample'}], indirect=True)
def test_testinstance_record(testinstance):
    testinstance.testcases = [mock.Mock()]
    recording = [ {'field_1':  'recording_1_1', 'field_2': 'recording_1_2'},
                  {'field_1':  'recording_2_1', 'field_2': 'recording_2_2'}
                ]
    with mock.patch(
        'builtins.open',
        mock.mock_open(read_data='')
    ) as mock_file, \
        mock.patch(
        'csv.DictWriter.writerow',
        mock.Mock()
    ) as mock_writeheader, \
        mock.patch(
        'csv.DictWriter.writerows',
        mock.Mock()
    ) as mock_writerows:
        testinstance.record(recording)

    print(mock_file.mock_calls)

    mock_file.assert_called_with(
        os.path.join(testinstance.build_dir, 'recording.csv'),
        'wt'
    )

    mock_writeheader.assert_has_calls([mock.call({ k:k for k in recording[0]})])
    mock_writerows.assert_has_calls([mock.call(recording)])


@pytest.mark.parametrize('testinstance', [{'testsuite_kind': 'sample'}], indirect=True)
def test_testinstance_add_filter(testinstance):
    reason = 'dummy reason'
    filter_type = 'dummy type'

    testinstance.add_filter(reason, filter_type)

    assert {'type': filter_type, 'reason': reason} in testinstance.filters
    assert testinstance.status == TwisterStatus.FILTER
    assert testinstance.reason == reason
    assert testinstance.filter_type == filter_type


def test_testinstance_init_cases(all_testsuites_dict, class_testplan, platforms_list):
    testsuite_path = 'scripts/tests/twister/test_data/testsuites/tests/test_a/test_a.check_1'
    class_testplan.testsuites = all_testsuites_dict
    testsuite = class_testplan.testsuites.get(testsuite_path)
    class_testplan.platforms = platforms_list
    platform = class_testplan.get_platform("demo_board_2")

    testinstance = TestInstance(testsuite, platform, class_testplan.env.outdir)

    testinstance.init_cases()

    assert all(
        [
            any(
                [
                    tcc.name == tc.name and tcc.freeform == tc.freeform \
                        for tcc in testinstance.testsuite.testcases
                ]
            ) for tc in testsuite.testcases
        ]
    )


@pytest.mark.parametrize('testinstance', [{'testsuite_kind': 'sample'}], indirect=True)
def test_testinstance_get_run_id(testinstance):
    res = testinstance._get_run_id()

    assert isinstance(res, str)


TESTDATA_2 = [
    ('another reason', 'another reason'),
    (None, 'dummy reason'),
]

@pytest.mark.parametrize('reason, expected_reason', TESTDATA_2)
@pytest.mark.parametrize('testinstance', [{'testsuite_kind': 'tests'}], indirect=True)
def test_testinstance_add_missing_case_status(testinstance, reason, expected_reason):
    testinstance.reason = 'dummy reason'

    status = TwisterStatus.PASS

    assert len(testinstance.testcases) > 1, 'Selected testsuite does not have enough testcases.'

    testinstance.testcases[0].status = TwisterStatus.STARTED
    testinstance.testcases[-1].status = TwisterStatus.NONE

    testinstance.add_missing_case_status(status, reason)

    assert testinstance.testcases[0].status == TwisterStatus.FAIL
    assert testinstance.testcases[-1].status == TwisterStatus.PASS
    assert testinstance.testcases[-1].reason == expected_reason


def test_testinstance_dunders(all_testsuites_dict, class_testplan, platforms_list):
    testsuite_path = 'scripts/tests/twister/test_data/testsuites/samples/test_app/sample_test.app'
    class_testplan.testsuites = all_testsuites_dict
    testsuite = class_testplan.testsuites.get(testsuite_path)
    class_testplan.platforms = platforms_list
    platform = class_testplan.get_platform("demo_board_2")

    testinstance = TestInstance(testsuite, platform, class_testplan.env.outdir)
    testinstance_copy = TestInstance(testsuite, platform, class_testplan.env.outdir)

    d = testinstance.__getstate__()

    d['name'] = 'dummy name'
    testinstance_copy.__setstate__(d)

    d['name'] = 'another name'
    testinstance.__setstate__(d)

    assert testinstance < testinstance_copy

    testinstance_copy.__setstate__(d)

    assert not testinstance < testinstance_copy
    assert not testinstance_copy < testinstance

    assert testinstance.__repr__() == f'<TestSuite {testsuite_path} on demo_board_2/unit_testing>'


@pytest.mark.parametrize('testinstance', [{'testsuite_kind': 'tests'}], indirect=True)
def test_testinstance_set_case_status_by_name(testinstance):
    name = 'test_a.check_1.2a'
    status = TwisterStatus.PASS
    reason = 'dummy reason'

    tc = testinstance.set_case_status_by_name(name, status, reason)

    assert tc.name == name
    assert tc.status == status
    assert tc.reason == reason

    tc = testinstance.set_case_status_by_name(name, status, None)

    assert tc.reason == reason


@pytest.mark.parametrize('testinstance', [{'testsuite_kind': 'tests'}], indirect=True)
def test_testinstance_add_testcase(testinstance):
    name = 'test_a.check_1.3a'
    freeform = True

    tc = testinstance.add_testcase(name, freeform)

    assert tc in testinstance.testcases


@pytest.mark.parametrize('testinstance', [{'testsuite_kind': 'tests'}], indirect=True)
def test_testinstance_get_case_by_name(testinstance):
    name = 'test_a.check_1.2a'

    tc = testinstance.get_case_by_name(name)

    assert tc.name == name

    name = 'test_a.check_1.3a'

    tc = testinstance.get_case_by_name(name)

    assert tc is None


@pytest.mark.parametrize('testinstance', [{'testsuite_kind': 'tests'}], indirect=True)
def test_testinstance_get_case_or_create(caplog, testinstance):
    name = 'test_a.check_1.2a'

    tc = testinstance.get_case_or_create(name)

    assert tc.name == name

    name = 'test_a.check_1.3a'

    tc = testinstance.get_case_or_create(name)

    assert tc.name == name
    assert 'Could not find a matching testcase for test_a.check_1.3a' in caplog.text


TESTDATA_3 = [
    (None, 'nonexistent harness', False),
    ('nonexistent fixture', 'console', False),
    (None, 'console', True),
    ('dummy fixture', 'console', True),
]

@pytest.mark.parametrize(
    'fixture, harness, expected_can_run',
    TESTDATA_3,
    ids=['improper harness', 'fixture not in list', 'no fixture specified', 'fixture in list']
)
def test_testinstance_testsuite_runnable(
    all_testsuites_dict,
    class_testplan,
    fixture,
    harness,
    expected_can_run
):
    testsuite_path = 'scripts/tests/twister/test_data/testsuites/samples/test_app/sample_test.app'
    class_testplan.testsuites = all_testsuites_dict
    testsuite = class_testplan.testsuites.get(testsuite_path)

    testsuite.harness = harness
    testsuite.harness_config['fixture'] = fixture

    fixtures = ['dummy fixture']

    can_run = TestInstance.testsuite_runnable(testsuite, fixtures)\

    assert can_run == expected_can_run


TESTDATA_4 = [
    (True, mock.ANY, mock.ANY, mock.ANY, None, [], False),
    (False, True, mock.ANY, mock.ANY, 'device', [], True),
    (False, False, 'qemu', mock.ANY, 'qemu', ['QEMU_PIPE=1'], True),
    (False, False, 'dummy sim', mock.ANY, 'dummy sim', [], True),
    (False, False, 'na', 'unit', 'unit', ['COVERAGE=1'], True),
    (False, False, 'na', 'dummy type', '', [], False),
]

@pytest.mark.parametrize(
    'preexisting_handler, device_testing, platform_sim, testsuite_type,' \
    ' expected_handler_type, expected_handler_args, expected_handler_ready',
    TESTDATA_4,
    ids=['preexisting handler', 'device testing', 'qemu simulation',
         'non-qemu simulation with exec', 'unit teting', 'no handler']
)
@pytest.mark.parametrize('testinstance', [{'testsuite_kind': 'tests'}], indirect=True)
def test_testinstance_setup_handler(
    testinstance,
    preexisting_handler,
    device_testing,
    platform_sim,
    testsuite_type,
    expected_handler_type,
    expected_handler_args,
    expected_handler_ready
):
    testinstance.handler = mock.Mock() if preexisting_handler else None
    testinstance.platform.simulation = platform_sim
    testinstance.platform.simulation_exec = 'dummy exec'
    testinstance.testsuite.type = testsuite_type
    env = mock.Mock(
        options=mock.Mock(
            device_testing=device_testing,
            enable_coverage=True
        )
    )

    with mock.patch.object(QEMUHandler, 'get_fifo', return_value=1), \
         mock.patch('shutil.which', return_value=True):
        testinstance.setup_handler(env)

    if expected_handler_type:
        assert testinstance.handler.type_str == expected_handler_type
        assert testinstance.handler.ready == expected_handler_ready
    assert all([arg in testinstance.handler.args for arg in expected_handler_args])


TESTDATA_5 = [
    ('nt', 'renode', mock.ANY, mock.ANY,
     mock.ANY, mock.ANY, mock.ANY,
     mock.ANY, mock.ANY, mock.ANY, mock.ANY, False),
    ('linux', mock.ANY, mock.ANY, mock.ANY,
     True, mock.ANY, mock.ANY,
     mock.ANY, mock.ANY, mock.ANY, mock.ANY, False),
    ('linux', mock.ANY, mock.ANY, mock.ANY,
     False, True, mock.ANY,
     False, mock.ANY, mock.ANY, mock.ANY, False),
    ('linux', 'qemu', mock.ANY, mock.ANY,
     False, mock.ANY, 'pytest',
     mock.ANY, 'not runnable', mock.ANY, None, True),
    ('linux', 'renode', 'renode', True,
     False, mock.ANY, 'console',
     mock.ANY, 'not runnable', [], None, True),
    ('linux', 'renode', 'renode', False,
     False, mock.ANY, 'not pytest',
     mock.ANY, 'not runnable', mock.ANY, None, False),
    ('linux', 'qemu', mock.ANY, mock.ANY,
     False, mock.ANY, 'console',
     mock.ANY, 'not runnable', ['?'], mock.Mock(duts=[mock.Mock(platform='demo_board_2', fixtures=[])]), True),
]

@pytest.mark.parametrize(
    'os_name, platform_sim, platform_sim_exec, exec_exists,' \
    ' testsuite_build_only, testsuite_slow, testsuite_harness,' \
    ' enable_slow, filter, fixtures, hardware_map, expected',
    TESTDATA_5,
    ids=['windows', 'build only', 'skip slow', 'pytest harness', 'sim', 'no sim', 'hardware map']
)
@pytest.mark.parametrize('testinstance', [{'testsuite_kind': 'tests'}], indirect=True)
def test_testinstance_check_runnable(
    testinstance,
    os_name,
    platform_sim,
    platform_sim_exec,
    exec_exists,
    testsuite_build_only,
    testsuite_slow,
    testsuite_harness,
    enable_slow,
    filter,
    fixtures,
    hardware_map,
    expected
):
    testinstance.platform.simulation = platform_sim
    testinstance.platform.simulation_exec = platform_sim_exec
    testinstance.testsuite.build_only = testsuite_build_only
    testinstance.testsuite.slow = testsuite_slow
    testinstance.testsuite.harness = testsuite_harness

    env = mock.Mock(
        options=mock.Mock(
            device_testing=False,
            enable_slow=enable_slow,
            fixtures=fixtures,
            filter=filter
        )
    )
    with mock.patch('os.name', os_name), \
         mock.patch('shutil.which', return_value=exec_exists):
        res = testinstance.check_runnable(env.options, hardware_map)

    assert res == expected


TESTDATA_6 = [
    (True, 'build.log'),
    (False, ''),
]

@pytest.mark.parametrize('from_buildlog, expected_buildlog_filepath', TESTDATA_6)
@pytest.mark.parametrize('testinstance', [{'testsuite_kind': 'tests'}], indirect=True)
def test_testinstance_calculate_sizes(testinstance, from_buildlog, expected_buildlog_filepath):
    expected_elf_filepath = 'dummy.elf'
    expected_extra_sections = []
    expected_warning = True
    testinstance.get_elf_file = mock.Mock(return_value='dummy.elf')
    testinstance.get_buildlog_file = mock.Mock(return_value='build.log')

    sc_mock = mock.Mock()
    mock_sc = mock.Mock(return_value=sc_mock)

    with mock.patch('twisterlib.testinstance.SizeCalculator', mock_sc):
        res = testinstance.calculate_sizes(from_buildlog, expected_warning)

    assert res == sc_mock
    mock_sc.assert_called_once_with(
        elf_filename=expected_elf_filepath,
        extra_sections=expected_extra_sections,
        buildlog_filepath=expected_buildlog_filepath,
        generate_warning=expected_warning
    )


TESTDATA_7 = [
    (True, None),
    (False, BuildError),
]

@pytest.mark.parametrize('sysbuild, expected_error', TESTDATA_7)
@pytest.mark.parametrize('testinstance', [{'testsuite_kind': 'tests'}], indirect=True)
def test_testinstance_get_elf_file(caplog, tmp_path, testinstance, sysbuild, expected_error):
    sysbuild_dir = tmp_path / 'sysbuild'
    sysbuild_dir.mkdir()
    zephyr_dir = sysbuild_dir / 'zephyr'
    zephyr_dir.mkdir()
    sysbuild_elf = zephyr_dir / 'dummy.elf'
    sysbuild_elf.write_bytes(b'0')
    sysbuild_elf2 = zephyr_dir / 'dummy2.elf'
    sysbuild_elf2.write_bytes(b'0')

    testinstance.sysbuild = sysbuild
    testinstance.domains = mock.Mock(
        get_default_domain=mock.Mock(
            return_value=mock.Mock(
                build_dir=sysbuild_dir
            )
        )
    )

    with pytest.raises(expected_error) if expected_error else nullcontext():
        testinstance.get_elf_file()

    if expected_error is None:
        assert 'multiple ELF files detected: ' in caplog.text


TESTDATA_8 = [
    (True, None),
    (False, BuildError),
]

@pytest.mark.parametrize('create_build_log, expected_error', TESTDATA_8)
@pytest.mark.parametrize('testinstance', [{'testsuite_kind': 'tests'}], indirect=True)
def test_testinstance_get_buildlog_file(tmp_path, testinstance, create_build_log, expected_error):
    if create_build_log:
        build_dir = tmp_path / 'build'
        build_dir.mkdir()
        build_log = build_dir / 'build.log'
        build_log.write_text('')
        testinstance.build_dir = build_dir

    with pytest.raises(expected_error) if expected_error else nullcontext():
        res = testinstance.get_buildlog_file()

    if expected_error is None:
        assert res == str(build_log)
