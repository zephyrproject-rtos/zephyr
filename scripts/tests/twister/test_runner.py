#!/usr/bin/env python3
# Copyright (c) 2023 Google LLC
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for runner.py classes
"""

import errno
import mock
import os
import pathlib
import pytest
import queue
import re
import subprocess
import sys
import yaml

from contextlib import nullcontext
from elftools.elf.sections import SymbolTableSection
from typing import List

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))

from twisterlib.statuses import TwisterStatus
from twisterlib.error import BuildError
from twisterlib.harness import Pytest

from twisterlib.runner import (
    CMake,
    ExecutionCounter,
    FilterBuilder,
    ProjectBuilder,
    TwisterRunner
)

@pytest.fixture
def mocked_instance(tmp_path):
    instance = mock.Mock()
    testsuite = mock.Mock()
    testsuite.source_dir: str = ''
    instance.testsuite = testsuite
    platform = mock.Mock()
    platform.sysbuild = False
    platform.binaries: List[str] = []
    instance.platform = platform
    build_dir = tmp_path / 'build_dir'
    os.makedirs(build_dir)
    instance.build_dir: str = str(build_dir)
    return instance


@pytest.fixture
def mocked_env():
    env = mock.Mock()
    options = mock.Mock()
    env.options = options
    return env


@pytest.fixture
def mocked_jobserver():
    jobserver = mock.Mock()
    return jobserver


@pytest.fixture
def project_builder(mocked_instance, mocked_env, mocked_jobserver) -> ProjectBuilder:
    project_builder = ProjectBuilder(mocked_instance, mocked_env, mocked_jobserver)
    return project_builder


@pytest.fixture
def runners(project_builder: ProjectBuilder) -> dict:
    """
    Create runners.yaml file in build_dir/zephyr directory and return file
    content as dict.
    """
    build_dir_zephyr_path = os.path.join(project_builder.instance.build_dir, 'zephyr')
    os.makedirs(build_dir_zephyr_path)
    runners_file_path = os.path.join(build_dir_zephyr_path, 'runners.yaml')
    runners_content: dict = {
        'config': {
            'elf_file': 'zephyr.elf',
            'hex_file': os.path.join(build_dir_zephyr_path, 'zephyr.elf'),
            'bin_file': 'zephyr.bin',
        }
    }
    with open(runners_file_path, 'w') as file:
        yaml.dump(runners_content, file)

    return runners_content


@mock.patch("os.path.exists")
def test_projectbuilder_cmake_assemble_args_single(m):
    # Causes the additional_overlay_path to be appended
    m.return_value = True

    class MockHandler:
        pass

    handler = MockHandler()
    handler.args = ["handler_arg1", "handler_arg2"]
    handler.ready = True

    assert(ProjectBuilder.cmake_assemble_args(
        ["basearg1", "CONFIG_t=\"test\"", "SNIPPET_t=\"test\""],
        handler,
        ["a.conf;b.conf", "c.conf"],
        ["extra_overlay.conf"],
        ["x.overlay;y.overlay", "z.overlay"],
        ["cmake1=foo", "cmake2=bar"],
        "/builddir/",
    ) == [
        "-DCONFIG_t=\"test\"",
        "-Dcmake1=foo", "-Dcmake2=bar",
        "-Dbasearg1", "-DSNIPPET_t=test",
        "-Dhandler_arg1", "-Dhandler_arg2",
        "-DCONF_FILE=a.conf;b.conf;c.conf",
        "-DDTC_OVERLAY_FILE=x.overlay;y.overlay;z.overlay",
        "-DOVERLAY_CONFIG=extra_overlay.conf "
        "/builddir/twister/testsuite_extra.conf",
    ])


def test_if_default_binaries_are_taken_properly(project_builder: ProjectBuilder):
    default_binaries = [
        os.path.join('zephyr', 'zephyr.hex'),
        os.path.join('zephyr', 'zephyr.bin'),
        os.path.join('zephyr', 'zephyr.elf'),
        os.path.join('zephyr', 'zephyr.exe'),
    ]
    project_builder.instance.sysbuild = False
    binaries = project_builder._get_binaries()
    assert sorted(binaries) == sorted(default_binaries)


def test_if_binaries_from_platform_are_taken_properly(project_builder: ProjectBuilder):
    platform_binaries = ['spi_image.bin']
    project_builder.platform.binaries = platform_binaries
    project_builder.instance.sysbuild = False
    platform_binaries_expected = [os.path.join('zephyr', bin) for bin in platform_binaries]
    binaries = project_builder._get_binaries()
    assert sorted(binaries) == sorted(platform_binaries_expected)


def test_if_binaries_from_runners_are_taken_properly(runners, project_builder: ProjectBuilder):
    runners_binaries = list(runners['config'].values())
    runners_binaries_expected = [bin if os.path.isabs(bin) else os.path.join('zephyr', bin) for bin in runners_binaries]
    binaries = project_builder._get_binaries_from_runners()
    assert sorted(binaries) == sorted(runners_binaries_expected)


def test_if_runners_file_is_sanitized_properly(runners, project_builder: ProjectBuilder):
    runners_file_path = os.path.join(project_builder.instance.build_dir, 'zephyr', 'runners.yaml')
    with open(runners_file_path, 'r') as file:
        unsanitized_runners_content = yaml.safe_load(file)
    unsanitized_runners_binaries = list(unsanitized_runners_content['config'].values())
    abs_paths = [bin for bin in unsanitized_runners_binaries if os.path.isabs(bin)]
    assert len(abs_paths) > 0

    project_builder._sanitize_runners_file()

    with open(runners_file_path, 'r') as file:
        sanitized_runners_content = yaml.safe_load(file)
    sanitized_runners_binaries = list(sanitized_runners_content['config'].values())
    abs_paths = [bin for bin in sanitized_runners_binaries if os.path.isabs(bin)]
    assert len(abs_paths) == 0


def test_if_zephyr_base_is_sanitized_properly(project_builder: ProjectBuilder):
    sanitized_path_expected = os.path.join('sanitized', 'path')
    path_to_sanitize = os.path.join(os.path.realpath(ZEPHYR_BASE), sanitized_path_expected)
    cmakecache_file_path = os.path.join(project_builder.instance.build_dir, 'CMakeCache.txt')
    with open(cmakecache_file_path, 'w') as file:
        file.write(path_to_sanitize)

    project_builder._sanitize_zephyr_base_from_files()

    with open(cmakecache_file_path, 'r') as file:
        sanitized_path = file.read()
    assert sanitized_path == sanitized_path_expected


def test_executioncounter(capfd):
    ec = ExecutionCounter(total=12)

    ec.cases = 25
    ec.skipped_cases = 6
    ec.error = 2
    ec.iteration = 2
    ec.done = 9
    ec.passed = 6
    ec.skipped_configs = 3
    ec.skipped_runtime = 1
    ec.skipped_filter = 2
    ec.failed = 1

    ec.summary()

    out, err = capfd.readouterr()
    sys.stdout.write(out)
    sys.stderr.write(err)

    assert (
        '--------------------------------------------------\n'
        'Total test suites:     12\n'
        'Processed test suites:  9\n'
        '├─ Filtered test suites (static):       2\n'
        '└─ Completed test suites:               7\n'
        '   ├─ Filtered test suites (at runtime):   1\n'
        '   ├─ Passed test suites:                  6\n'
        '   ├─ Built only test suites:              0\n'
        '   ├─ Failed test suites:                  1\n'
        '   └─ Errors in test suites:               2\n'
        '\n'
        'Filtered test suites: 3\n'
        '├─ Filtered test suites (static):       2\n'
        '└─ Filtered test suites (at runtime):   1\n'
        '----------------------      ----------------------\n'
        'Total test cases: 25\n'
        '├─ Filtered test cases:  0\n'
        '└─ Selected test cases: 25\n'
        '   ├─ Passed test cases:        0\n'
        '   ├─ Skipped test cases:       6\n'
        '   ├─ Built only test cases:    0\n'
        '   ├─ Blocked test cases:       0\n'
        '   ├─ Failed test cases:        0\n'
        '   └─ Errors in test cases:     0\n'
        '--------------------------------------------------\n'
    ) in out

    assert ec.cases == 25
    assert ec.skipped_cases == 6
    assert ec.error == 2
    assert ec.iteration == 2
    assert ec.done == 9
    assert ec.passed == 6
    assert ec.skipped_configs == 3
    assert ec.skipped_runtime == 1
    assert ec.skipped_filter == 2
    assert ec.failed == 1


def test_cmake_parse_generated(mocked_jobserver):
    testsuite_mock = mock.Mock()
    platform_mock = mock.Mock()
    source_dir = os.path.join('source', 'dir')
    build_dir = os.path.join('build', 'dir')

    cmake = CMake(testsuite_mock, platform_mock, source_dir, build_dir,
                  mocked_jobserver)

    result = cmake.parse_generated()

    assert cmake.defconfig == {}
    assert result == {}


TESTDATA_1_1 = [
    ('linux'),
    ('nt')
]
TESTDATA_1_2 = [
    (0, False, 'dummy out',
     True, True, TwisterStatus.NOTRUN, None, False, True),
    (0, True, '',
     False, False, TwisterStatus.PASS, None, False, False),
    (1, True, 'ERROR: region `FLASH\' overflowed by 123 MB',
     True,  True, TwisterStatus.SKIP, 'FLASH overflow', True, False),
    (1, True, 'Error: Image size (99 B) + trailer (1 B) exceeds requested size',
     True, True, TwisterStatus.SKIP, 'imgtool overflow', True, False),
    (1, True, 'mock.ANY',
     True, True, TwisterStatus.ERROR, 'Build failure', False, False)
]

@pytest.mark.parametrize(
    'return_code, is_instance_run, p_out, expect_returncode,' \
    ' expect_writes, expected_status, expected_reason,' \
    ' expected_change_skip, expected_add_missing',
    TESTDATA_1_2,
    ids=['no error, no instance run', 'no error, instance run',
         'error - region overflow', 'error - image size exceed', 'error']
)
@pytest.mark.parametrize('sys_platform', TESTDATA_1_1)
def test_cmake_run_build(
    sys_platform,
    return_code,
    is_instance_run,
    p_out,
    expect_returncode,
    expect_writes,
    expected_status,
    expected_reason,
    expected_change_skip,
    expected_add_missing
):
    process_mock = mock.Mock(
        returncode=return_code,
        communicate=mock.Mock(
            return_value=(p_out.encode(sys.getdefaultencoding()), None)
        )
    )

    def mock_popen(*args, **kwargs):
        return process_mock

    testsuite_mock = mock.Mock()
    platform_mock = mock.Mock()
    platform_mock.name = '<platform name>'
    source_dir = os.path.join('source', 'dir')
    build_dir = os.path.join('build', 'dir')
    jobserver_mock = mock.Mock(
        popen=mock.Mock(side_effect=mock_popen)
    )
    instance_mock = mock.Mock(add_missing_case_status=mock.Mock())
    instance_mock.build_time = 0
    instance_mock.run = is_instance_run
    instance_mock.status = TwisterStatus.NONE
    instance_mock.reason = None

    cmake = CMake(testsuite_mock, platform_mock, source_dir, build_dir,
                  jobserver_mock)
    cmake.cwd = os.path.join('dummy', 'working', 'dir')
    cmake.instance = instance_mock
    cmake.options = mock.Mock()
    cmake.options.overflow_as_errors = False

    cmake_path = os.path.join('dummy', 'cmake')

    popen_mock = mock.Mock(side_effect=mock_popen)
    change_mock = mock.Mock()

    with mock.patch('sys.platform', sys_platform), \
         mock.patch('shutil.which', return_value=cmake_path), \
         mock.patch('twisterlib.runner.change_skip_to_error_if_integration',
                    change_mock), \
         mock.patch('builtins.open', mock.mock_open()), \
         mock.patch('subprocess.Popen', popen_mock):
        result = cmake.run_build(args=['arg1', 'arg2'])

    expected_results = {}
    if expect_returncode:
        expected_results['returncode'] = return_code
    if expected_results == {}:
        expected_results = None

    assert expected_results == result

    popen_caller = cmake.jobserver.popen if sys_platform == 'linux' else \
                   popen_mock
    popen_caller.assert_called_once_with(
        [os.path.join('dummy', 'cmake'), 'arg1', 'arg2'],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=os.path.join('dummy', 'working', 'dir')
    )

    assert cmake.instance.status == expected_status
    assert cmake.instance.reason == expected_reason

    if expected_change_skip:
        change_mock.assert_called_once()

    if expected_add_missing:
        cmake.instance.add_missing_case_status.assert_called_once_with(
            TwisterStatus.NOTRUN, 'Test was built only'
        )


TESTDATA_2_1 = [
    ('linux'),
    ('nt')
]
TESTDATA_2_2 = [
    (True, ['dummy_stage_1', 'ds2'],
     0, False, '',
     True, True, False,
     TwisterStatus.NONE, None,
     [os.path.join('dummy', 'cmake'),
      '-B' + os.path.join('build', 'dir'), '-DTC_RUNID=1', '-DTC_NAME=testcase',
      '-DSB_CONFIG_COMPILER_WARNINGS_AS_ERRORS=y',
      '-DEXTRA_GEN_EDT_ARGS=--edtlib-Werror', '-Gdummy_generator',
      f'-DPython3_EXECUTABLE={pathlib.Path(sys.executable).as_posix()}',
      '-S' + os.path.join('source', 'dir'),
      'arg1', 'arg2',
      '-DBOARD=<platform name>',
      '-DSNIPPET=dummy snippet 1;ds2',
      '-DMODULES=dummy_stage_1,ds2',
      '-Pzephyr_base/cmake/package_helper.cmake']),
    (False, [],
     1, True, 'ERROR: region `FLASH\' overflowed by 123 MB',
     True, False, True,
     TwisterStatus.ERROR, 'CMake build failure',
     [os.path.join('dummy', 'cmake'),
      '-B' + os.path.join('build', 'dir'), '-DTC_RUNID=1', '-DTC_NAME=testcase',
      '-DSB_CONFIG_COMPILER_WARNINGS_AS_ERRORS=n',
      '-DEXTRA_GEN_EDT_ARGS=', '-Gdummy_generator',
      f'-DPython3_EXECUTABLE={pathlib.Path(sys.executable).as_posix()}',
      '-Szephyr_base/share/sysbuild',
      '-DAPP_DIR=' + os.path.join('source', 'dir'),
      'arg1', 'arg2',
      '-DBOARD=<platform name>',
      '-DSNIPPET=dummy snippet 1;ds2']),
]

@pytest.mark.parametrize(
    'error_warns, f_stages,' \
    ' return_code, is_instance_run, p_out, expect_returncode,' \
    ' expect_filter, expect_writes, expected_status, expected_reason,' \
    ' expected_cmd',
    TESTDATA_2_2,
    ids=['filter_stages with success', 'no stages with error']
)
@pytest.mark.parametrize('sys_platform', TESTDATA_2_1)
def test_cmake_run_cmake(
    sys_platform,
    error_warns,
    f_stages,
    return_code,
    is_instance_run,
    p_out,
    expect_returncode,
    expect_filter,
    expect_writes,
    expected_status,
    expected_reason,
    expected_cmd
):
    process_mock = mock.Mock(
        returncode=return_code,
        communicate=mock.Mock(
            return_value=(p_out.encode(sys.getdefaultencoding()), None)
        )
    )

    def mock_popen(*args, **kwargs):
        return process_mock

    testsuite_mock = mock.Mock()
    testsuite_mock.sysbuild = True
    platform_mock = mock.Mock()
    platform_mock.name = '<platform name>'
    source_dir = os.path.join('source', 'dir')
    build_dir = os.path.join('build', 'dir')
    jobserver_mock = mock.Mock(
        popen=mock.Mock(side_effect=mock_popen)
    )
    instance_mock = mock.Mock(add_missing_case_status=mock.Mock())
    instance_mock.run = is_instance_run
    instance_mock.run_id = 1
    instance_mock.build_time = 0
    instance_mock.status = TwisterStatus.NONE
    instance_mock.reason = None
    instance_mock.testsuite = mock.Mock()
    instance_mock.testsuite.name = 'testcase'
    instance_mock.testsuite.required_snippets = ['dummy snippet 1', 'ds2']
    instance_mock.testcases = [mock.Mock(), mock.Mock()]
    instance_mock.testcases[0].status = TwisterStatus.NONE
    instance_mock.testcases[1].status = TwisterStatus.NONE

    cmake = CMake(testsuite_mock, platform_mock, source_dir, build_dir,
                  jobserver_mock)
    cmake.cwd = os.path.join('dummy', 'working', 'dir')
    cmake.instance = instance_mock
    cmake.options = mock.Mock()
    cmake.options.disable_warnings_as_errors = not error_warns
    cmake.options.overflow_as_errors = False
    cmake.env = mock.Mock()
    cmake.env.generator = 'dummy_generator'

    cmake_path = os.path.join('dummy', 'cmake')

    popen_mock = mock.Mock(side_effect=mock_popen)
    change_mock = mock.Mock()

    with mock.patch('sys.platform', sys_platform), \
         mock.patch('shutil.which', return_value=cmake_path), \
         mock.patch('twisterlib.runner.change_skip_to_error_if_integration',
                    change_mock), \
         mock.patch('twisterlib.runner.canonical_zephyr_base',
                    'zephyr_base'), \
         mock.patch('builtins.open', mock.mock_open()), \
         mock.patch('subprocess.Popen', popen_mock):
        result = cmake.run_cmake(args=['arg1', 'arg2'], filter_stages=f_stages)

    expected_results = {}
    if expect_returncode:
        expected_results['returncode'] = return_code
    if expect_filter:
        expected_results['filter'] = {}
    if expected_results == {}:
        expected_results = None

    assert expected_results == result

    popen_caller = cmake.jobserver.popen if sys_platform == 'linux' else \
                   popen_mock
    popen_caller.assert_called_once_with(
        expected_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=os.path.join('dummy', 'working', 'dir')
    )

    assert cmake.instance.status == expected_status
    assert cmake.instance.reason == expected_reason

    for tc in cmake.instance.testcases:
        assert tc.status == cmake.instance.status


TESTDATA_3 = [
    ('unit_testing', [], False, True, None, True, None, True,
     None, None, {}, {}, None, None, [], {}),
    (
        'other', [], True,
        True, ['dummy', 'west', 'options'], True,
        None, True,
        os.path.join('domain', 'build', 'dir', 'zephyr', '.config'),
        os.path.join('domain', 'build', 'dir', 'zephyr', 'edt.pickle'),
        {'CONFIG_FOO': 'no'},
        {'dummy cache elem': 1},
        {'ARCH': 'dummy arch', 'PLATFORM': 'other', 'env_dummy': True,
         'CONFIG_FOO': 'no', 'dummy cache elem': 1},
        b'dummy edt pickle contents',
        [f'Loaded sysbuild domain data from' \
         f' {os.path.join("build", "dir", "domains.yaml")}'],
        {os.path.join('other', 'dummy.testsuite.name'): True}
    ),
    (
        'other', ['kconfig'], True,
        True, ['dummy', 'west', 'options'], True,
        'Dummy parse results', True,
        os.path.join('build', 'dir', 'zephyr', '.config'),
        os.path.join('build', 'dir', 'zephyr', 'edt.pickle'),
        {'CONFIG_FOO': 'no'},
        {'dummy cache elem': 1},
        {'ARCH': 'dummy arch', 'PLATFORM': 'other', 'env_dummy': True,
         'CONFIG_FOO': 'no', 'dummy cache elem': 1},
        b'dummy edt pickle contents',
        [],
        {os.path.join('other', 'dummy.testsuite.name'): False}
    ),
    (
        'other', ['other'], False,
        False, None, True,
        'Dummy parse results', True,
        None,
        os.path.join('build', 'dir', 'zephyr', 'edt.pickle'),
        {},
        {},
        {'ARCH': 'dummy arch', 'PLATFORM': 'other', 'env_dummy': True},
        b'dummy edt pickle contents',
        [],
        {os.path.join('other', 'dummy.testsuite.name'): False}
    ),
    (
        'other', ['other'], True,
        False, None, True,
        'Dummy parse results', True,
        None,
        None,
        {},
        {},
        {},
        None,
        ['Sysbuild test will be skipped. West must be used for flashing.'],
        {os.path.join('other', 'dummy.testsuite.name'): True}
    ),
    (
        'other', ['other'], False,
        True, None, False,
        'Dummy parse results', True,
        None,
        None,
        {},
        {'dummy cache elem': 1},
        {'ARCH': 'dummy arch', 'PLATFORM': 'other', 'env_dummy': True,
         'dummy cache elem': 1},
        None,
        [],
        {os.path.join('other', 'dummy.testsuite.name'): False}
    ),
    (
        'other', ['other'], False,
        True, None, True,
        'Dummy parse results', True,
        None,
        os.path.join('build', 'dir', 'zephyr', 'edt.pickle'),
        {},
        {'dummy cache elem': 1},
        {'ARCH': 'dummy arch', 'PLATFORM': 'other', 'env_dummy': True,
         'dummy cache elem': 1},
        b'dummy edt pickle contents',
        [],
        {os.path.join('other', 'dummy.testsuite.name'): False}
    ),
    (
        'other', ['other'], False,
        True, None, True,
        None, True,
        None,
        os.path.join('build', 'dir', 'zephyr', 'edt.pickle'),
        {},
        {'dummy cache elem': 1},
        {'ARCH': 'dummy arch', 'PLATFORM': 'other', 'env_dummy': True,
         'dummy cache elem': 1},
        b'dummy edt pickle contents',
        [],
        {os.path.join('other', 'dummy.testsuite.name'): True}
    ),
    (
        'other', ['other'], False,
        True, None, True,
        'Dummy parse results', False,
        None,
        os.path.join('build', 'dir', 'zephyr', 'edt.pickle'),
        {},
        {'dummy cache elem': 1},
        {'ARCH': 'dummy arch', 'PLATFORM': 'other', 'env_dummy': True,
         'dummy cache elem': 1},
        b'dummy edt pickle contents',
        [],
        {'ARCH': 'dummy arch', 'PLATFORM': 'other', 'env_dummy': True,
         'dummy cache elem': 1}
    ),
    (
        'other', ['other'], False,
        True, None, True,
        SyntaxError, True,
        None,
        os.path.join('build', 'dir', 'zephyr', 'edt.pickle'),
        {},
        {'dummy cache elem': 1},
        {'ARCH': 'dummy arch', 'PLATFORM': 'other', 'env_dummy': True,
         'dummy cache elem': 1},
        b'dummy edt pickle contents',
        ['Failed processing testsuite.yaml'],
        SyntaxError
    ),
]

@pytest.mark.parametrize(
    'platform_name, filter_stages, sysbuild,' \
    ' do_find_cache, west_flash_options, edt_exists,' \
    ' parse_results, testsuite_filter,' \
    ' expected_defconfig_path, expected_edt_pickle_path,' \
    ' expected_defconfig, expected_cmakecache, expected_filter_data,' \
    ' expected_edt,' \
    ' expected_logs, expected_return',
    TESTDATA_3,
    ids=['unit testing', 'domain', 'kconfig', 'no cache',
         'no west options', 'no edt',
         'parse result', 'no parse result', 'no testsuite filter', 'parse err']
)
def test_filterbuilder_parse_generated(
    caplog,
    mocked_jobserver,
    platform_name,
    filter_stages,
    sysbuild,
    do_find_cache,
    west_flash_options,
    edt_exists,
    parse_results,
    testsuite_filter,
    expected_defconfig_path,
    expected_edt_pickle_path,
    expected_defconfig,
    expected_cmakecache,
    expected_filter_data,
    expected_edt,
    expected_logs,
    expected_return
):
    def mock_domains_from_file(*args, **kwargs):
        dom = mock.Mock()
        dom.build_dir = os.path.join('domain', 'build', 'dir')
        res = mock.Mock(get_default_domain=mock.Mock(return_value=dom))
        return res

    def mock_cmakecache_from_file(*args, **kwargs):
        if not do_find_cache:
            raise FileNotFoundError(errno.ENOENT, 'Cache not found')
        cache_elem = mock.Mock()
        cache_elem.name = 'dummy cache elem'
        cache_elem.value = 1
        cache = [cache_elem]
        return cache

    def mock_open(filepath, type, *args, **kwargs):
        if filepath == expected_defconfig_path:
            rd = 'I am not a proper line\n' \
                 'CONFIG_FOO="no"'
        elif filepath == expected_edt_pickle_path:
            rd = b'dummy edt pickle contents'
        else:
            raise FileNotFoundError(errno.ENOENT,
                                    f'File {filepath} not mocked.')
        return mock.mock_open(read_data=rd)()

    def mock_parser(filter, filter_data, edt):
        assert filter_data == expected_filter_data
        if isinstance(parse_results, type) and \
           issubclass(parse_results, Exception):
            raise parse_results
        return parse_results

    def mock_pickle(datafile):
        assert datafile.read() == expected_edt
        return mock.Mock()

    testsuite_mock = mock.Mock()
    testsuite_mock.name = 'dummy.testsuite.name'
    testsuite_mock.filter = testsuite_filter
    platform_mock = mock.Mock()
    platform_mock.name = platform_name
    platform_mock.arch = 'dummy arch'
    source_dir = os.path.join('source', 'dir')
    build_dir = os.path.join('build', 'dir')

    fb = FilterBuilder(testsuite_mock, platform_mock, source_dir, build_dir,
                       mocked_jobserver)
    instance_mock = mock.Mock()
    instance_mock.sysbuild = 'sysbuild' if sysbuild else None
    fb.instance = instance_mock
    fb.env = mock.Mock()
    fb.env.options = mock.Mock()
    fb.env.options.west_flash = west_flash_options
    fb.env.options.device_testing = True

    environ_mock = {'env_dummy': True}

    with mock.patch('twisterlib.runner.Domains.from_file',
                    mock_domains_from_file), \
         mock.patch('twisterlib.runner.CMakeCache.from_file',
                    mock_cmakecache_from_file), \
         mock.patch('builtins.open', mock_open), \
         mock.patch('expr_parser.parse', mock_parser), \
         mock.patch('pickle.load', mock_pickle), \
         mock.patch('os.path.exists', return_value=edt_exists), \
         mock.patch('os.environ', environ_mock), \
         pytest.raises(expected_return) if \
             isinstance(parse_results, type) and \
             issubclass(parse_results, Exception) else nullcontext() as err:
        result = fb.parse_generated(filter_stages)

    if err:
        assert True
        return

    assert all([log in caplog.text for log in expected_logs])

    assert fb.defconfig == expected_defconfig

    assert fb.cmake_cache == expected_cmakecache

    assert result == expected_return


TESTDATA_4 = [
    (False, False, [f"see: {os.path.join('dummy', 'path', 'dummy_file.log')}"]),
    (True, False, [os.path.join('dummy', 'path', 'dummy_file.log'),
                    'file contents',
                    os.path.join('dummy', 'path', 'dummy_file.log')]),
    (True, True, [os.path.join('dummy', 'path', 'dummy_file.log'),
                   'Unable to read log data ([Errno 2] ERROR: dummy_file.log)',
                   os.path.join('dummy', 'path', 'dummy_file.log')]),
]

@pytest.mark.parametrize(
    'inline_logs, read_exception, expected_logs',
    TESTDATA_4,
    ids=['basic', 'inline logs', 'inline logs+read_exception']
)
def test_projectbuilder_log_info(
    caplog,
    mocked_jobserver,
    inline_logs,
    read_exception,
    expected_logs
):
    def mock_open(filename, *args, **kwargs):
        if read_exception:
            raise OSError(errno.ENOENT, f'ERROR: {os.path.basename(filename)}')
        return mock.mock_open(read_data='file contents')()

    def mock_realpath(filename, *args, **kwargs):
        return os.path.join('path', filename)

    def mock_abspath(filename, *args, **kwargs):
        return os.path.join('dummy', filename)

    filename = 'dummy_file.log'

    env_mock = mock.Mock()
    instance_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)
    with mock.patch('builtins.open', mock_open), \
         mock.patch('os.path.realpath', mock_realpath), \
         mock.patch('os.path.abspath', mock_abspath):
        pb.log_info(filename, inline_logs)

    assert all([log in caplog.text for log in expected_logs])


TESTDATA_5 = [
    (True, False, False, "Valgrind error", 0, 0, 'build_dir/valgrind.log'),
    (True, False, False, "Error", 0, 0, 'build_dir/build.log'),
    (False, True, False, None, 1024, 0, 'build_dir/handler.log'),
    (False, True, False, None, 0, 0, 'build_dir/build.log'),
    (False, False, True, None, 0, 1024, 'build_dir/device.log'),
    (False, False, True, None, 0, 0, 'build_dir/build.log'),
    (False, False, False, None, 0, 0, 'build_dir/build.log'),
]

@pytest.mark.parametrize(
    'valgrind_log_exists, handler_log_exists, device_log_exists,' \
    ' instance_reason, handler_log_getsize, device_log_getsize, expected_log',
    TESTDATA_5,
    ids=['valgrind log', 'valgrind log unused',
         'handler log', 'handler log unused',
         'device log', 'device log unused',
         'no logs']
)
def test_projectbuilder_log_info_file(
    caplog,
    mocked_jobserver,
    valgrind_log_exists,
    handler_log_exists,
    device_log_exists,
    instance_reason,
    handler_log_getsize,
    device_log_getsize,
    expected_log
):
    def mock_exists(filename, *args, **kwargs):
        if filename == 'build_dir/handler.log':
            return handler_log_exists
        if filename == 'build_dir/valgrind.log':
            return valgrind_log_exists
        if filename == 'build_dir/device.log':
            return device_log_exists
        return False

    def mock_getsize(filename, *args, **kwargs):
        if filename == 'build_dir/handler.log':
            return handler_log_getsize
        if filename == 'build_dir/device.log':
            return device_log_getsize
        return 0

    env_mock = mock.Mock()
    instance_mock = mock.Mock()
    instance_mock.reason = instance_reason
    instance_mock.build_dir = 'build_dir'

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)

    log_info_mock = mock.Mock()

    with mock.patch('os.path.exists', mock_exists), \
         mock.patch('os.path.getsize', mock_getsize), \
         mock.patch('twisterlib.runner.ProjectBuilder.log_info', log_info_mock):
        pb.log_info_file(None)

    log_info_mock.assert_called_with(expected_log, mock.ANY)


TESTDATA_6 = [
    (
        {'op': 'filter'},
        TwisterStatus.FAIL,
        'Failed',
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        {'op': 'report', 'test': mock.ANY},
        TwisterStatus.FAIL,
        'Failed',
        0,
        None
    ),
    (
        {'op': 'filter'},
        TwisterStatus.PASS,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        {'filter': { 'dummy instance name': True }},
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        ['filtering dummy instance name'],
        {'op': 'report', 'test': mock.ANY},
        TwisterStatus.FILTER,
        'runtime filter',
        0,
        (TwisterStatus.FILTER,)
    ),
    (
        {'op': 'filter'},
        TwisterStatus.PASS,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        {'filter': { 'another dummy instance name': True }},
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        {'op': 'cmake', 'test': mock.ANY},
        TwisterStatus.PASS,
        mock.ANY,
        0,
        None
    ),
    (
        {'op': 'cmake'},
        TwisterStatus.ERROR,
        'dummy error',
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        {'op': 'report', 'test': mock.ANY},
        TwisterStatus.ERROR,
        'dummy error',
        0,
        None
    ),
    (
        {'op': 'cmake'},
        TwisterStatus.NONE,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        True,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        {'op': 'report', 'test': mock.ANY},
        TwisterStatus.NOTRUN,
        mock.ANY,
        0,
        None
    ),
    (
        {'op': 'cmake'},
        'success',
        mock.ANY,
        mock.ANY,
        mock.ANY,
        True,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        {'op': 'report', 'test': mock.ANY},
        'success',
        mock.ANY,
        0,
        None
    ),
    (
        {'op': 'cmake'},
        'success',
        mock.ANY,
        mock.ANY,
        mock.ANY,
        False,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        {'filter': {'dummy instance name': True}},
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        ['filtering dummy instance name'],
        {'op': 'report', 'test': mock.ANY},
        TwisterStatus.FILTER,
        'runtime filter',
        1,
        (TwisterStatus.FILTER,) # this is a tuple
    ),
    (
        {'op': 'cmake'},
        'success',
        mock.ANY,
        mock.ANY,
        mock.ANY,
        False,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        {'filter': {}},
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        {'op': 'build', 'test': mock.ANY},
        'success',
        mock.ANY,
        0,
        None
    ),
    (
        {'op': 'build'},
        mock.ANY,
        None,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        None,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        ['build test: dummy instance name'],
        {'op': 'report', 'test': mock.ANY},
        TwisterStatus.ERROR,
        'Build Failure',
        0,
        None
    ),
    (
        {'op': 'build'},
        TwisterStatus.SKIP,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        {'returncode': 0},
        mock.ANY,
        mock.ANY,
        mock.ANY,
        ['build test: dummy instance name',
         'Determine test cases for test instance: dummy instance name'],
        {'op': 'gather_metrics', 'test': mock.ANY},
        mock.ANY,
        mock.ANY,
        1,
        (TwisterStatus.SKIP, mock.ANY)
    ),
    (
        {'op': 'build'},
        TwisterStatus.PASS,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        {'dummy': 'dummy'},
        mock.ANY,
        mock.ANY,
        mock.ANY,
        ['build test: dummy instance name'],
        {'op': 'report', 'test': mock.ANY},
        TwisterStatus.PASS,
        mock.ANY,
        0,
        (TwisterStatus.BLOCK, mock.ANY)
    ),
    (
        {'op': 'build'},
        'success',
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        {'returncode': 0},
        mock.ANY,
        mock.ANY,
        mock.ANY,
        ['build test: dummy instance name',
         'Determine test cases for test instance: dummy instance name'],
        {'op': 'gather_metrics', 'test': mock.ANY},
        mock.ANY,
        mock.ANY,
        0,
        None
    ),
    (
        {'op': 'build'},
        'success',
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        {'returncode': 0},
        mock.ANY,
        mock.ANY,
        BuildError,
        ['build test: dummy instance name',
         'Determine test cases for test instance: dummy instance name'],
        {'op': 'report', 'test': mock.ANY},
        TwisterStatus.ERROR,
        'Determine Testcases Error!',
        0,
        None
    ),
    (
        {'op': 'gather_metrics'},
        mock.ANY,
        mock.ANY,
        True,
        True,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        {'returncode': 0},  # metrics_res
        mock.ANY,
        mock.ANY,
        [],
        {'op': 'run', 'test': mock.ANY},
        mock.ANY,
        mock.ANY,
        0,
        None
    ),  # 'gather metrics, run and ready handler'
    (
        {'op': 'gather_metrics'},
        mock.ANY,
        mock.ANY,
        False,
        True,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        {'returncode': 0},  # metrics_res
        mock.ANY,
        mock.ANY,
        [],
        {'op': 'report', 'test': mock.ANY},
        mock.ANY,
        mock.ANY,
        0,
        None
    ),  # 'gather metrics'
    (
        {'op': 'gather_metrics'},
        mock.ANY,
        mock.ANY,
        False,
        True,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        {'returncode': 0},  # build_res
        {'returncode': 1},  # metrics_res
        mock.ANY,
        mock.ANY,
        [],
        {'op': 'report', 'test': mock.ANY},
        'error',
        'Build Failure at gather_metrics.',
        0,
        None
    ),  # 'build ok, gather metrics fail',
    (
        {'op': 'run'},
        'success',
        'OK',
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        None,
        mock.ANY,
        ['run test: dummy instance name',
         'run status: dummy instance name success'],
        {'op': 'report', 'test': mock.ANY, 'status': 'success', 'reason': 'OK'},
        'success',
        'OK',
        0,
        None
    ),
    (
        {'op': 'run'},
        TwisterStatus.FAIL,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        RuntimeError,
        mock.ANY,
        ['run test: dummy instance name',
         'run status: dummy instance name failed',
         'RuntimeError: Pipeline Error!'],
        None,
        TwisterStatus.FAIL,
        mock.ANY,
        0,
        None
    ),
    (
        {'op': 'report'},
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        False,
        True,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        {'op': 'cleanup', 'mode': 'device', 'test': mock.ANY},
        mock.ANY,
        mock.ANY,
        0,
        None
    ),
    (
        {'op': 'report'},
        TwisterStatus.PASS,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        False,
        False,
        'pass',
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        {'op': 'cleanup', 'mode': 'passed', 'test': mock.ANY},
        mock.ANY,
        mock.ANY,
        0,
        None
    ),
    (
        {'op': 'report'},
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        False,
        False,
        'all',
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        {'op': 'cleanup', 'mode': 'all', 'test': mock.ANY},
        mock.ANY,
        mock.ANY,
        0,
        None
    ),
    (
        {'op': 'report'},
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        False,
        False,
        'other',
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        None,
        mock.ANY,
        mock.ANY,
        0,
        None
    ),
    (
        {'op': 'cleanup', 'mode': 'device'},
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        None,
        mock.ANY,
        mock.ANY,
        0,
        None
    ),
    (
        {'op': 'cleanup', 'mode': 'passed'},
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        None,
        mock.ANY,
        mock.ANY,
        0,
        None
    ),
    (
        {'op': 'cleanup', 'mode': 'all'},
        mock.ANY,
        'Valgrind error',
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        None,
        mock.ANY,
        mock.ANY,
        0,
        None
    ),
    (
        {'op': 'cleanup', 'mode': 'all'},
        mock.ANY,
        'CMake build failure',
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        mock.ANY,
        [],
        None,
        mock.ANY,
        mock.ANY,
        0,
        None
    ),
]

@pytest.mark.parametrize(
    'message,' \
    ' instance_status, instance_reason, instance_run, instance_handler_ready,' \
    ' options_cmake_only,' \
    ' options_coverage, options_prep_artifacts, options_runtime_artifacts,' \
    ' cmake_res, build_res, metrics_res,' \
    ' pipeline_runtime_error, determine_testcases_build_error,' \
    ' expected_logs, resulting_message,' \
    ' expected_status, expected_reason, expected_skipped, expected_missing',
    TESTDATA_6,
    ids=[
        'filter, failed', 'filter, cmake res', 'filter, no cmake res',
        'cmake, failed', 'cmake, cmake_only, no status', 'cmake, cmake_only',
        'cmake, no cmake_only, cmake res', 'cmake, no cmake_only, no cmake res',
        'build, no build res', 'build, skipped', 'build, blocked',
        'build, determine testcases', 'build, determine testcases Error',
        'gather metrics, run and ready handler', 'gather metrics',
        'build ok, gather metrics fail',
        'run', 'run, Pipeline Runtime Error',
        'report, prep artifacts for testing',
        'report, runtime artifact cleanup pass, status passed',
        'report, runtime artifact cleanup all', 'report, no message put',
        'cleanup, device', 'cleanup, mode passed', 'cleanup, mode all',
        'cleanup, mode all, cmake build failure'
    ]
)
def test_projectbuilder_process(
    caplog,
    mocked_jobserver,
    message,
    instance_status,
    instance_reason,
    instance_run,
    instance_handler_ready,
    options_cmake_only,
    options_coverage,
    options_prep_artifacts,
    options_runtime_artifacts,
    cmake_res,
    build_res,
    metrics_res,
    pipeline_runtime_error,
    determine_testcases_build_error,
    expected_logs,
    resulting_message,
    expected_status,
    expected_reason,
    expected_skipped,
    expected_missing
):
    def mock_pipeline_put(msg):
        if isinstance(pipeline_runtime_error, type) and \
           issubclass(pipeline_runtime_error, Exception):
            raise RuntimeError('Pipeline Error!')

    def mock_determine_testcases(res):
        if isinstance(determine_testcases_build_error, type) and \
           issubclass(determine_testcases_build_error, Exception):
            raise BuildError('Determine Testcases Error!')

    instance_mock = mock.Mock()
    instance_mock.name = 'dummy instance name'
    instance_mock.status = instance_status
    instance_mock.reason = instance_reason
    instance_mock.run = instance_run
    instance_mock.handler = mock.Mock()
    instance_mock.handler.ready = instance_handler_ready
    instance_mock.testsuite.harness = 'test'
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)
    pb.options = mock.Mock()
    pb.options.coverage = options_coverage
    pb.options.prep_artifacts_for_testing = options_prep_artifacts
    pb.options.runtime_artifact_cleanup = options_runtime_artifacts
    pb.options.cmake_only = options_cmake_only

    pb.cmake = mock.Mock(return_value=cmake_res)
    pb.build = mock.Mock(return_value=build_res)
    pb.determine_testcases = mock.Mock(side_effect=mock_determine_testcases)

    pb.report_out = mock.Mock()
    pb.cleanup_artifacts = mock.Mock()
    pb.cleanup_device_testing_artifacts = mock.Mock()
    pb.run = mock.Mock()
    pb.gather_metrics = mock.Mock(return_value=metrics_res)

    pipeline_mock = mock.Mock(put=mock.Mock(side_effect=mock_pipeline_put))
    done_mock = mock.Mock()
    lock_mock = mock.Mock(
        __enter__=mock.Mock(return_value=(mock.Mock(), mock.Mock())),
        __exit__=mock.Mock(return_value=None)
    )
    results_mock = mock.Mock()
    results_mock.skipped_runtime = 0

    pb.process(pipeline_mock, done_mock, message, lock_mock, results_mock)

    assert all([log in caplog.text for log in expected_logs])

    if resulting_message:
        pipeline_mock.put.assert_called_with(resulting_message)

    assert pb.instance.status == expected_status
    assert pb.instance.reason == expected_reason
    assert results_mock.skipped_runtime_increment.call_args_list == [mock.call()] * expected_skipped

    if expected_missing:
        pb.instance.add_missing_case_status.assert_called_with(*expected_missing)


TESTDATA_7 = [
    (
        [
            'z_ztest_unit_test__dummy_suite_name__dummy_test_name',
            'z_ztest_unit_test__dummy_suite_name__test_dummy_name',
            'no match'
        ],
        ['dummy_id.dummy_name', 'dummy_id.dummy_name']
    ),
    (
        ['no match'],
        []
    ),
]

@pytest.mark.parametrize(
    'symbols_names, added_tcs',
    TESTDATA_7,
    ids=['two hits, one miss', 'nothing']
)
def test_projectbuilder_determine_testcases(
    mocked_jobserver,
    symbols_names,
    added_tcs
):
    symbols_mock = [mock.Mock(n=name) for name in symbols_names]
    for m in symbols_mock:
        m.configure_mock(name=m.n)

    sections_mock = [mock.Mock(spec=SymbolTableSection)]
    sections_mock[0].iter_symbols = mock.Mock(return_value=symbols_mock)

    elf_mock = mock.Mock()
    elf_mock().iter_sections = mock.Mock(return_value=sections_mock)

    results_mock = mock.Mock()

    instance_mock = mock.Mock()
    instance_mock.testcases = []
    instance_mock.testsuite.id = 'dummy_id'
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)

    with mock.patch('twisterlib.runner.ELFFile', elf_mock), \
         mock.patch('builtins.open', mock.mock_open()):
        pb.determine_testcases(results_mock)

    pb.instance.add_testcase.assert_has_calls(
        [mock.call(name=x) for x in added_tcs]
    )
    pb.instance.testsuite.add_testcase.assert_has_calls(
        [mock.call(name=x) for x in added_tcs]
    )


TESTDATA_8 = [
    (
        ['addition.al'],
        'dummy',
        ['addition.al', '.config', 'zephyr']
    ),
    (
        [],
        'all',
        ['.config', 'zephyr', 'testsuite_extra.conf', 'twister']
    ),
]

@pytest.mark.parametrize(
    'additional_keep, runtime_artifact_cleanup, expected_files',
    TESTDATA_8,
    ids=['additional keep', 'all cleanup']
)
def test_projectbuilder_cleanup_artifacts(
    tmpdir,
    mocked_jobserver,
    additional_keep,
    runtime_artifact_cleanup,
    expected_files
):
    # tmpdir
    # ┣ twister
    # ┃ ┗ testsuite_extra.conf
    # ┣ dummy_dir
    # ┃ ┗ dummy.del
    # ┣ dummy_link_dir -> zephyr
    # ┣ zephyr
    # ┃ ┗ .config
    # ┗ addition.al
    twister_dir = tmpdir.mkdir('twister')
    testsuite_extra_conf = twister_dir.join('testsuite_extra.conf')
    testsuite_extra_conf.write_text('dummy', 'utf-8')

    dummy_dir = tmpdir.mkdir('dummy_dir')
    dummy_del = dummy_dir.join('dummy.del')
    dummy_del.write_text('dummy', 'utf-8')

    zephyr = tmpdir.mkdir('zephyr')
    config = zephyr.join('.config')
    config.write_text('dummy', 'utf-8')

    dummy_link_dir = tmpdir.join('dummy_link_dir')
    os.symlink(zephyr, dummy_link_dir)

    addition_al = tmpdir.join('addition.al')
    addition_al.write_text('dummy', 'utf-8')

    instance_mock = mock.Mock()
    instance_mock.build_dir = tmpdir
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)
    pb.options = mock.Mock(runtime_artifact_cleanup=runtime_artifact_cleanup)

    pb.cleanup_artifacts(additional_keep)

    files_left = [p.name for p in list(pathlib.Path(tmpdir).glob('**/*'))]

    assert sorted(files_left) == sorted(expected_files)


def test_projectbuilder_cleanup_device_testing_artifacts(
    caplog,
    mocked_jobserver
):
    bins = [os.path.join('zephyr', 'file.bin')]

    instance_mock = mock.Mock()
    instance_mock.sysbuild = False
    build_dir = os.path.join('build', 'dir')
    instance_mock.build_dir = build_dir
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)
    pb._get_binaries = mock.Mock(return_value=bins)
    pb.cleanup_artifacts = mock.Mock()
    pb._sanitize_files = mock.Mock()

    pb.cleanup_device_testing_artifacts()

    assert f'Cleaning up for Device Testing {build_dir}' in caplog.text

    pb.cleanup_artifacts.assert_called_once_with(
        [os.path.join('zephyr', 'file.bin'),
         os.path.join('zephyr', 'runners.yaml')]
    )
    pb._sanitize_files.assert_called_once()


TESTDATA_9 = [
    (
        None,
        [],
        [os.path.join('zephyr', 'zephyr.hex'),
         os.path.join('zephyr', 'zephyr.bin'),
         os.path.join('zephyr', 'zephyr.elf'),
         os.path.join('zephyr', 'zephyr.exe')]
    ),
    (
        [os.path.join('dummy.bin'), os.path.join('dummy.hex')],
        [os.path.join('dir2', 'dummy.elf')],
        [os.path.join('zephyr', 'dummy.bin'),
         os.path.join('zephyr', 'dummy.hex'),
         os.path.join('dir2', 'dummy.elf')]
    ),
]

@pytest.mark.parametrize(
    'platform_binaries, runner_binaries, expected_binaries',
    TESTDATA_9,
    ids=['default', 'valid']
)
def test_projectbuilder_get_binaries(
    mocked_jobserver,
    platform_binaries,
    runner_binaries,
    expected_binaries
):
    def mock_get_domains(*args, **kwargs):
        return []

    instance_mock = mock.Mock()
    instance_mock.build_dir = os.path.join('build', 'dir')
    instance_mock.domains.get_domains.side_effect = mock_get_domains
    instance_mock.platform = mock.Mock()
    instance_mock.platform.binaries = platform_binaries
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)
    pb._get_binaries_from_runners = mock.Mock(return_value=runner_binaries)

    bins = pb._get_binaries()

    assert all(bin in expected_binaries for bin in bins)
    assert all(bin in bins for bin in expected_binaries)


TESTDATA_10 = [
    (None, None, []),
    (None, {'dummy': 'dummy'}, []),
    (   None,
        {
            'config': {
                'elf_file': '/absolute/path/dummy.elf',
                'bin_file': 'path/dummy.bin'
            }
        },
        ['/absolute/path/dummy.elf', os.path.join('zephyr', 'path/dummy.bin')]
    ),
    (   'test_domain',
        {
            'config': {
                'elf_file': '/absolute/path/dummy.elf',
                'bin_file': 'path/dummy.bin'
            }
        },
        ['/absolute/path/dummy.elf', os.path.join('test_domain', 'zephyr', 'path/dummy.bin')]
    ),
]

@pytest.mark.parametrize(
    'domain, runners_content, expected_binaries',
    TESTDATA_10,
    ids=['no file', 'no config', 'valid', 'with domain']
)
def test_projectbuilder_get_binaries_from_runners(
    mocked_jobserver,
    domain,
    runners_content,
    expected_binaries
):
    def mock_exists(fname):
        assert fname == os.path.join('build', 'dir', domain if domain else '',
                                     'zephyr', 'runners.yaml')
        return runners_content is not None

    instance_mock = mock.Mock()
    instance_mock.build_dir = os.path.join('build', 'dir')
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)

    with mock.patch('os.path.exists', mock_exists), \
         mock.patch('builtins.open', mock.mock_open()), \
         mock.patch('yaml.load', return_value=runners_content):
        if domain:
            bins = pb._get_binaries_from_runners(domain)
        else:
            bins = pb._get_binaries_from_runners()

    assert all(bin in expected_binaries for bin in bins)
    assert all(bin in bins for bin in expected_binaries)


def test_projectbuilder_sanitize_files(mocked_jobserver):
    instance_mock = mock.Mock()
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)
    pb._sanitize_runners_file = mock.Mock()
    pb._sanitize_zephyr_base_from_files = mock.Mock()

    pb._sanitize_files()

    pb._sanitize_runners_file.assert_called_once()
    pb._sanitize_zephyr_base_from_files.assert_called_once()



TESTDATA_11 = [
    (None, None),
    ('dummy: []', None),
    (
"""
config:
  elf_file: relative/path/dummy.elf
  hex_file: /absolute/path/build_dir/zephyr/dummy.hex
""",
"""
config:
  elf_file: relative/path/dummy.elf
  hex_file: dummy.hex
"""
    ),
]

@pytest.mark.parametrize(
    'runners_text, expected_write_text',
    TESTDATA_11,
    ids=['no file', 'no config', 'valid']
)
def test_projectbuilder_sanitize_runners_file(
    mocked_jobserver,
    runners_text,
    expected_write_text
):
    def mock_exists(fname):
        return runners_text is not None

    instance_mock = mock.Mock()
    instance_mock.build_dir = '/absolute/path/build_dir'
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)

    with mock.patch('os.path.exists', mock_exists), \
         mock.patch('builtins.open',
                    mock.mock_open(read_data=runners_text)) as f:
        pb._sanitize_runners_file()

    if expected_write_text is not None:
        f().write.assert_called_with(expected_write_text)
    else:
        f().write.assert_not_called()


TESTDATA_12 = [
    (
        {
            'CMakeCache.txt': mock.mock_open(
                read_data='canonical/zephyr/base/dummy.file: ERROR'
            )
        },
        {
            'CMakeCache.txt': 'dummy.file: ERROR'
        }
    ),
    (
        {
            os.path.join('zephyr', 'runners.yaml'): mock.mock_open(
                read_data='There was canonical/zephyr/base/dummy.file here'
            )
        },
        {
            os.path.join('zephyr', 'runners.yaml'): 'There was dummy.file here'
        }
    ),
]

@pytest.mark.parametrize(
    'text_mocks, expected_write_texts',
    TESTDATA_12,
    ids=['CMakeCache file', 'runners.yaml file']
)
def test_projectbuilder_sanitize_zephyr_base_from_files(
    mocked_jobserver,
    text_mocks,
    expected_write_texts
):
    build_dir_path = 'canonical/zephyr/base/build_dir/'

    def mock_exists(fname):
        if not fname.startswith(build_dir_path):
            return False
        return fname[len(build_dir_path):] in text_mocks

    def mock_open(fname, *args, **kwargs):
        if not fname.startswith(build_dir_path):
            raise FileNotFoundError(errno.ENOENT, f'File {fname} not found.')
        return text_mocks[fname[len(build_dir_path):]]()

    instance_mock = mock.Mock()
    instance_mock.build_dir = build_dir_path
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)

    with mock.patch('os.path.exists', mock_exists), \
         mock.patch('builtins.open', mock_open), \
         mock.patch('twisterlib.runner.canonical_zephyr_base',
                    'canonical/zephyr/base'):
        pb._sanitize_zephyr_base_from_files()

    for fname, fhandler in text_mocks.items():
        fhandler().write.assert_called_with(expected_write_texts[fname])


TESTDATA_13 = [
    (
        TwisterStatus.ERROR, True, True, False,
        ['INFO      20/25 dummy platform' \
         '            dummy.testsuite.name' \
         '                               ERROR dummy reason (cmake)'],
        None
    ),
    (
        TwisterStatus.FAIL, False, False, False,
        ['ERROR     dummy platform' \
         '            dummy.testsuite.name' \
         '                               FAILED: dummy reason'],
        'INFO    - Total complete:   20/  25  80%' \
        '  built (not run):    0, filtered:    3, failed:    3, error:    1'
    ),
    (
        TwisterStatus.SKIP, True, False, False,
        ['INFO      20/25 dummy platform' \
         '            dummy.testsuite.name' \
         '                               SKIPPED (dummy reason)'],
        None
    ),
    (
        TwisterStatus.FILTER, False, False, False,
        [],
        'INFO    - Total complete:   20/  25  80%' \
        '  built (not run):    0, filtered:    4, failed:    2, error:    1'
    ),
    (
        TwisterStatus.PASS, True, False, True,
        ['INFO      20/25 dummy platform' \
         '            dummy.testsuite.name' \
         '                               PASSED' \
         ' (dummy handler type: dummy dut, 60.000s)'],
        None
    ),
    (
        TwisterStatus.PASS, True, False, False,
        ['INFO      20/25 dummy platform' \
         '            dummy.testsuite.name' \
         '                               PASSED (build)'],
        None
    ),
    (
        'unknown status', False, False, False,
        ['Unknown status = unknown status'],
        'INFO    - Total complete:   20/  25  80%'
        '  built (not run):    0, filtered:    3, failed:    2, error:    1\r'
    )
]

@pytest.mark.parametrize(
    'status, verbose, cmake_only, ready_run, expected_logs, expected_out',
    TESTDATA_13,
    ids=['verbose error cmake only', 'failed', 'verbose skipped', 'filtered',
         'verbose passed ready run', 'verbose passed', 'unknown status']
)
def test_projectbuilder_report_out(
    capfd,
    caplog,
    mocked_jobserver,
    status,
    verbose,
    cmake_only,
    ready_run,
    expected_logs,
    expected_out
):
    instance_mock = mock.Mock()
    instance_mock.handler.type_str = 'dummy handler type'
    instance_mock.handler.seed = 123
    instance_mock.handler.ready = ready_run
    instance_mock.run = ready_run
    instance_mock.dut = 'dummy dut'
    instance_mock.execution_time = 60
    instance_mock.platform.name = 'dummy platform'
    instance_mock.status = status
    instance_mock.reason = 'dummy reason'
    instance_mock.testsuite.name = 'dummy.testsuite.name'
    skip_mock_tc = mock.Mock(status=TwisterStatus.SKIP, reason=None)
    skip_mock_tc.name = 'mocked_testcase_to_skip'
    unknown_mock_tc = mock.Mock(status=mock.Mock(value='dummystatus'), reason=None)
    unknown_mock_tc.name = 'mocked_testcase_unknown'
    instance_mock.testsuite.testcases = [unknown_mock_tc for _ in range(25)]
    instance_mock.testcases = [unknown_mock_tc for _ in range(24)] + \
                              [skip_mock_tc]
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)
    pb.options.verbose = verbose
    pb.options.cmake_only = cmake_only
    pb.options.seed = 123
    pb.log_info_file = mock.Mock()

    results_mock = mock.Mock(
        total = 25,
        done = 19,
        passed = 17,
        notrun = 0,
        failed = 2,
        skipped_configs = 3,
        skipped_runtime = 0,
        skipped_filter = 0,
        error = 1,
        cases = 0,
        filtered_cases = 0,
        skipped_cases = 4,
        failed_cases = 0,
        error_cases = 0,
        blocked_cases = 0,
        passed_cases = 0,
        none_cases = 0,
        started_cases = 0
    )
    results_mock.iteration = 1
    def results_done_increment(value=1, decrement=False):
        results_mock.done += value * (-1 if decrement else 1)
    results_mock.done_increment = results_done_increment
    def skipped_configs_increment(value=1, decrement=False):
        results_mock.skipped_configs += value * (-1 if decrement else 1)
    results_mock.skipped_configs_increment = skipped_configs_increment
    def skipped_filter_increment(value=1, decrement=False):
        results_mock.skipped_filter += value * (-1 if decrement else 1)
    results_mock.skipped_filter_increment = skipped_filter_increment
    def skipped_runtime_increment(value=1, decrement=False):
        results_mock.skipped_runtime += value * (-1 if decrement else 1)
    results_mock.skipped_runtime_increment = skipped_runtime_increment
    def failed_increment(value=1, decrement=False):
        results_mock.failed += value * (-1 if decrement else 1)
    results_mock.failed_increment = failed_increment
    def notrun_increment(value=1, decrement=False):
        results_mock.notrun += value * (-1 if decrement else 1)
    results_mock.notrun_increment = notrun_increment

    pb.report_out(results_mock)

    assert results_mock.cases_increment.call_args_list == [mock.call(25)]

    trim_actual_log = re.sub(
        r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])',
        '',
        caplog.text
    )
    trim_actual_log = re.sub(r'twister:runner.py:\d+', '', trim_actual_log)

    assert all([log in trim_actual_log for log in expected_logs])

    print(trim_actual_log)
    if expected_out:
        out, err = capfd.readouterr()
        sys.stdout.write(out)
        sys.stderr.write(err)

        # Remove 7b ANSI C1 escape sequences (colours)
        out = re.sub(
            r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])',
            '',
            out
        )

        assert expected_out in out


def test_projectbuilder_cmake_assemble_args():
    extra_args = ['CONFIG_FOO=y', 'DUMMY_EXTRA="yes"']
    handler = mock.Mock(ready=True, args=['dummy_handler'])
    extra_conf_files = ['extrafile1.conf', 'extrafile2.conf']
    extra_overlay_confs = ['extra_overlay_conf']
    extra_dtc_overlay_files = ['overlay1.dtc', 'overlay2.dtc']
    cmake_extra_args = ['CMAKE1="yes"', 'CMAKE2=n']
    build_dir = os.path.join('build', 'dir')

    with mock.patch('os.path.exists', return_value=True):
        results = ProjectBuilder.cmake_assemble_args(extra_args, handler,
                                                     extra_conf_files,
                                                     extra_overlay_confs,
                                                     extra_dtc_overlay_files,
                                                     cmake_extra_args,
                                                     build_dir)

    expected_results = [
        '-DCONFIG_FOO=y',
        '-DCMAKE1=\"yes\"',
        '-DCMAKE2=n',
        '-DDUMMY_EXTRA=yes',
        '-Ddummy_handler',
        '-DCONF_FILE=extrafile1.conf;extrafile2.conf',
        '-DDTC_OVERLAY_FILE=overlay1.dtc;overlay2.dtc',
        f'-DOVERLAY_CONFIG=extra_overlay_conf ' \
        f'{os.path.join("build", "dir", "twister", "testsuite_extra.conf")}'
    ]

    assert results == expected_results


def test_projectbuilder_cmake():
    instance_mock = mock.Mock()
    instance_mock.handler = 'dummy handler'
    instance_mock.build_dir = os.path.join('build', 'dir')
    instance_mock.platform.name = 'frdm_k64f'
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)
    pb.build_dir = 'build_dir'
    pb.testsuite.platform = instance_mock.platform
    pb.testsuite.extra_args = ['some', 'platform:frdm_k64f:args']
    pb.testsuite.extra_conf_files = ['some', 'files1']
    pb.testsuite.extra_overlay_confs = ['some', 'files2']
    pb.testsuite.extra_dtc_overlay_files = ['some', 'files3']
    pb.options.extra_args = ['other', 'args']
    pb.cmake_assemble_args = mock.Mock(return_value=['dummy'])
    cmake_res_mock = mock.Mock()
    pb.run_cmake = mock.Mock(return_value=cmake_res_mock)

    res = pb.cmake(['dummy filter'])

    assert res == cmake_res_mock
    pb.cmake_assemble_args.assert_called_once_with(
        ['some', 'args'],
        pb.instance.handler,
        pb.testsuite.extra_conf_files,
        pb.testsuite.extra_overlay_confs,
        pb.testsuite.extra_dtc_overlay_files,
        pb.options.extra_args,
        pb.instance.build_dir
    )
    pb.run_cmake.assert_called_once_with(['dummy'], ['dummy filter'])


def test_projectbuilder_build(mocked_jobserver):
    instance_mock = mock.Mock()
    instance_mock.testsuite.harness = 'test'
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)

    pb.build_dir = 'build_dir'
    pb.run_build = mock.Mock(return_value={'dummy': 'dummy'})

    res = pb.build()

    pb.run_build.assert_called_once_with(['--build', 'build_dir'])
    assert res == {'dummy': 'dummy'}


TESTDATA_14 = [
    (
        True,
        'device',
        234,
        'native_sim',
        'posix',
        {'CONFIG_FAKE_ENTROPY_NATIVE_POSIX': 'y'},
        'pytest',
        True,
        True,
        True,
        True,
        True,
        False
    ),
    (
        True,
        'not device',
        None,
        'native_sim',
        'not posix',
        {'CONFIG_FAKE_ENTROPY_NATIVE_POSIX': 'y'},
        'not pytest',
        False,
        False,
        False,
        False,
        False,
        True
    ),
    (
        False,
        'device',
        234,
        'native_sim',
        'posix',
        {'CONFIG_FAKE_ENTROPY_NATIVE_POSIX': 'y'},
        'pytest',
        False,
        False,
        False,
        False,
        False,
        False
    ),
]

@pytest.mark.parametrize(
    'ready, type_str, seed, platform_name, platform_arch, defconfig, harness,' \
    ' expect_duts, expect_parse_generated, expect_seed,' \
    ' expect_extra_test_args, expect_pytest, expect_handle',
    TESTDATA_14,
    ids=['pytest full', 'not pytest minimal', 'not ready']
)
def test_projectbuilder_run(
    mocked_jobserver,
    ready,
    type_str,
    seed,
    platform_name,
    platform_arch,
    defconfig,
    harness,
    expect_duts,
    expect_parse_generated,
    expect_seed,
    expect_extra_test_args,
    expect_pytest,
    expect_handle
):
    pytest_mock = mock.Mock(spec=Pytest)
    harness_mock = mock.Mock()

    def mock_harness(name):
        if name == 'Pytest':
            return pytest_mock
        else:
            return harness_mock

    instance_mock = mock.Mock()
    instance_mock.handler.get_test_timeout = mock.Mock(return_value=60)
    instance_mock.handler.seed = 123
    instance_mock.handler.ready = ready
    instance_mock.handler.type_str = type_str
    instance_mock.handler.duts = [mock.Mock(name='dummy dut')]
    instance_mock.platform.name = platform_name
    instance_mock.platform.arch = platform_arch
    instance_mock.testsuite.harness = harness
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)
    pb.options.extra_test_args = ['dummy_arg1', 'dummy_arg2']
    pb.duts = ['another dut']
    pb.options.seed = seed
    pb.defconfig = defconfig
    pb.parse_generated = mock.Mock()

    with mock.patch('twisterlib.runner.HarnessImporter.get_harness',
                    mock_harness):
        pb.run()

    if expect_duts:
        assert pb.instance.handler.duts == ['another dut']

    if expect_parse_generated:
        pb.parse_generated.assert_called_once()

    if expect_seed:
        assert pb.instance.handler.seed == seed

    if expect_extra_test_args:
        assert pb.instance.handler.extra_test_args == ['dummy_arg1',
                                                       'dummy_arg2']

    if expect_pytest:
        pytest_mock.pytest_run.assert_called_once_with(60)

    if expect_handle:
        pb.instance.handler.handle.assert_called_once_with(harness_mock)


TESTDATA_15 = [
    (False, False, False, True),
    (True, False, True, False),
    (False, True, False, True),
    (True, True, False, True),
]

@pytest.mark.parametrize(
    'enable_size_report, cmake_only, expect_calc_size, expect_zeroes',
    TESTDATA_15,
    ids=['none', 'size_report', 'cmake', 'size_report+cmake']
)
def test_projectbuilder_gather_metrics(
    mocked_jobserver,
    enable_size_report,
    cmake_only,
    expect_calc_size,
    expect_zeroes
):
    instance_mock = mock.Mock()
    instance_mock.metrics = {}
    env_mock = mock.Mock()

    pb = ProjectBuilder(instance_mock, env_mock, mocked_jobserver)
    pb.options.enable_size_report = enable_size_report
    pb.options.create_rom_ram_report = False
    pb.options.cmake_only = cmake_only
    pb.calc_size = mock.Mock()

    pb.gather_metrics(instance_mock)

    if expect_calc_size:
        pb.calc_size.assert_called_once()

    if expect_zeroes:
        assert instance_mock.metrics['used_ram'] == 0
        assert instance_mock.metrics['used_rom'] == 0
        assert instance_mock.metrics['available_rom'] == 0
        assert instance_mock.metrics['available_ram'] == 0
        assert instance_mock.metrics['unrecognized'] == []


TESTDATA_16 = [
    (TwisterStatus.ERROR, mock.ANY, False, False, False),
    (TwisterStatus.FAIL, mock.ANY, False, False, False),
    (TwisterStatus.SKIP, mock.ANY, False, False, False),
    (TwisterStatus.FILTER, 'native', False, False, True),
    (TwisterStatus.PASS, 'qemu', False, False, True),
    (TwisterStatus.FILTER, 'unit', False, False, True),
    (TwisterStatus.FILTER, 'mcu', True, True, False),
    (TwisterStatus.PASS, 'frdm_k64f', False, True, False),
]

@pytest.mark.parametrize(
    'status, platform_type, expect_warnings, expect_calcs, expect_zeroes',
    TESTDATA_16,
    ids=[x[0] + (', ' + x[1]) if x[1] != mock.ANY else '' for x in TESTDATA_16]
)
def test_projectbuilder_calc_size(
    status,
    platform_type,
    expect_warnings,
    expect_calcs,
    expect_zeroes
):
    size_calc_mock = mock.Mock()

    instance_mock = mock.Mock()
    instance_mock.status = status
    instance_mock.platform.type = platform_type
    instance_mock.metrics = {}
    instance_mock.calculate_sizes = mock.Mock(return_value=size_calc_mock)

    from_buildlog = True

    ProjectBuilder.calc_size(instance_mock, from_buildlog)

    if expect_calcs:
        instance_mock.calculate_sizes.assert_called_once_with(
            from_buildlog=from_buildlog,
            generate_warning=expect_warnings
        )

        assert instance_mock.metrics['used_ram'] == \
               size_calc_mock.get_used_ram()
        assert instance_mock.metrics['used_rom'] == \
               size_calc_mock.get_used_rom()
        assert instance_mock.metrics['available_rom'] == \
               size_calc_mock.get_available_rom()
        assert instance_mock.metrics['available_ram'] == \
               size_calc_mock.get_available_ram()
        assert instance_mock.metrics['unrecognized'] == \
               size_calc_mock.unrecognized_sections()

    if expect_zeroes:
        assert instance_mock.metrics['used_ram'] == 0
        assert instance_mock.metrics['used_rom'] == 0
        assert instance_mock.metrics['available_rom'] == 0
        assert instance_mock.metrics['available_ram'] == 0
        assert instance_mock.metrics['unrecognized'] == []

    if expect_calcs or expect_zeroes:
        assert instance_mock.metrics['handler_time'] == \
               instance_mock.execution_time
    else:
        assert instance_mock.metrics == {}


TESTDATA_17 = [
    ('linux', 'posix', {'jobs': 4}, True, 32, 'GNUMakeJobClient'),
    ('linux', 'posix', {'build_only': True}, False, 16, 'GNUMakeJobServer'),
    ('linux', '???', {}, False, 8, 'JobClient'),
    ('linux', '???', {'jobs': 4}, False, 4, 'JobClient'),
]

@pytest.mark.parametrize(
    'platform, os_name, options, jobclient_from_environ, expected_jobs,' \
    ' expected_jobserver',
    TESTDATA_17,
    ids=['GNUMakeJobClient', 'GNUMakeJobServer',
         'JobClient', 'Jobclient+options']
)
def test_twisterrunner_run(
    caplog,
    platform,
    os_name,
    options,
    jobclient_from_environ,
    expected_jobs,
    expected_jobserver
):
    def mock_client_from_environ(jobs):
        if jobclient_from_environ:
            jobclient_mock = mock.Mock(jobs=32)
            jobclient_mock.name = 'GNUMakeJobClient'
            return jobclient_mock
        return None

    instances = {'dummy instance': mock.Mock(metrics={'k': 'v'})}
    suites = [mock.Mock()]
    env_mock = mock.Mock()

    tr = TwisterRunner(instances, suites, env=env_mock)
    tr.options.retry_failed = 2
    tr.options.retry_interval = 10
    tr.options.retry_build_errors = True
    tr.options.jobs = None
    tr.options.build_only = None
    for k, v in options.items():
        setattr(tr.options, k, v)
    tr.update_counting_before_pipeline = mock.Mock()
    tr.execute = mock.Mock()
    tr.show_brief = mock.Mock()

    gnumakejobserver_mock = mock.Mock()
    gnumakejobserver_mock().name='GNUMakeJobServer'
    jobclient_mock = mock.Mock()
    jobclient_mock().name='JobClient'

    pipeline_q = queue.LifoQueue()
    done_q = queue.LifoQueue()
    done_instance = mock.Mock(
        metrics={'k2': 'v2'},
        execution_time=30
    )
    done_instance.name='dummy instance'
    done_q.put(done_instance)
    manager_mock = mock.Mock()
    manager_mock().LifoQueue = mock.Mock(
        side_effect=iter([pipeline_q, done_q])
    )

    results_mock = mock.Mock()
    results_mock().error = 1
    results_mock().iteration = 0
    results_mock().failed = 2
    results_mock().total = 9

    def iteration_increment(value=1, decrement=False):
        results_mock().iteration += value * (-1 if decrement else 1)
    results_mock().iteration_increment = iteration_increment

    with mock.patch('twisterlib.runner.ExecutionCounter', results_mock), \
         mock.patch('twisterlib.runner.BaseManager', manager_mock), \
         mock.patch('twisterlib.runner.GNUMakeJobClient.from_environ',
                    mock_client_from_environ), \
         mock.patch('twisterlib.runner.GNUMakeJobServer',
                    gnumakejobserver_mock), \
         mock.patch('twisterlib.runner.JobClient', jobclient_mock), \
         mock.patch('multiprocessing.cpu_count', return_value=8), \
         mock.patch('sys.platform', platform), \
         mock.patch('time.sleep', mock.Mock()), \
         mock.patch('os.name', os_name):
        tr.run()

    assert f'JOBS: {expected_jobs}' in caplog.text

    assert tr.jobserver.name == expected_jobserver

    assert tr.instances['dummy instance'].metrics == {
        'k': 'v',
        'k2': 'v2',
        'handler_time': 30,
        'unrecognized': []
    }

    assert results_mock().error == 0


def test_twisterrunner_update_counting_before_pipeline():
    instances = {
        'dummy1': mock.Mock(
            status=TwisterStatus.FILTER,
            reason='runtime filter',
            testsuite=mock.Mock(
                testcases=[mock.Mock()]
            )
        ),
        'dummy2': mock.Mock(
            status=TwisterStatus.FILTER,
            reason='static filter',
            testsuite=mock.Mock(
                testcases=[mock.Mock(), mock.Mock(), mock.Mock(), mock.Mock()]
            )
        ),
        'dummy3': mock.Mock(
            status=TwisterStatus.ERROR,
            reason='error',
            testsuite=mock.Mock(
                testcases=[mock.Mock()]
            )
        ),
        'dummy4': mock.Mock(
            status=TwisterStatus.PASS,
            reason='OK',
            testsuite=mock.Mock(
                testcases=[mock.Mock()]
            )
        ),
        'dummy5': mock.Mock(
            status=TwisterStatus.SKIP,
            reason=None,
            testsuite=mock.Mock(
                testcases=[mock.Mock()]
            )
        )
    }
    suites = [mock.Mock()]
    env_mock = mock.Mock()

    tr = TwisterRunner(instances, suites, env=env_mock)
    tr.results = mock.Mock(
        total = 0,
        done = 0,
        passed = 0,
        failed = 0,
        skipped_configs = 0,
        skipped_runtime = 0,
        skipped_filter = 0,
        error = 0,
        cases = 0,
        filtered_cases = 0,
        skipped_cases = 0,
        failed_cases = 0,
        error_cases = 0,
        blocked_cases = 0,
        passed_cases = 0,
        none_cases = 0,
        started_cases = 0
    )
    def skipped_configs_increment(value=1, decrement=False):
        tr.results.skipped_configs += value * (-1 if decrement else 1)
    tr.results.skipped_configs_increment = skipped_configs_increment
    def skipped_filter_increment(value=1, decrement=False):
        tr.results.skipped_filter += value * (-1 if decrement else 1)
    tr.results.skipped_filter_increment = skipped_filter_increment
    def error_increment(value=1, decrement=False):
        tr.results.error += value * (-1 if decrement else 1)
    tr.results.error_increment = error_increment
    def cases_increment(value=1, decrement=False):
        tr.results.cases += value * (-1 if decrement else 1)
    tr.results.cases_increment = cases_increment
    def filtered_cases_increment(value=1, decrement=False):
        tr.results.filtered_cases += value * (-1 if decrement else 1)
    tr.results.filtered_cases_increment = filtered_cases_increment

    tr.update_counting_before_pipeline()

    assert tr.results.skipped_filter == 1
    assert tr.results.skipped_configs == 1
    assert tr.results.filtered_cases == 4
    assert tr.results.cases == 4
    assert tr.results.error == 1


def test_twisterrunner_show_brief(caplog):
    instances = {
        'dummy1': mock.Mock(),
        'dummy2': mock.Mock(),
        'dummy3': mock.Mock(),
        'dummy4': mock.Mock(),
        'dummy5': mock.Mock()
    }
    suites = [mock.Mock(), mock.Mock()]
    env_mock = mock.Mock()

    tr = TwisterRunner(instances, suites, env=env_mock)
    tr.results = mock.Mock(
        skipped_filter = 3,
        skipped_configs = 4,
        skipped_cases = 0,
        cases = 0,
        error = 0
    )

    tr.show_brief()

    log = '2 test scenarios (5 configurations) selected,' \
          ' 4 configurations filtered (3 by static filter, 1 at runtime).'

    assert log in caplog.text


TESTDATA_18 = [
    (False, False, False, [{'op': 'cmake', 'test': mock.ANY}]),
    (False, False, True, [{'op': 'filter', 'test': mock.ANY},
                          {'op': 'cmake', 'test': mock.ANY}]),
    (False, True, True, [{'op': 'run', 'test': mock.ANY},
                         {'op': 'run', 'test': mock.ANY}]),
    (False, True, False, [{'op': 'run', 'test': mock.ANY}]),
    (True, True, False, [{'op': 'cmake', 'test': mock.ANY}]),
    (True, True, True, [{'op': 'filter', 'test': mock.ANY},
                        {'op': 'cmake', 'test': mock.ANY}]),
    (True, False, True, [{'op': 'filter', 'test': mock.ANY},
                         {'op': 'cmake', 'test': mock.ANY}]),
    (True, False, False, [{'op': 'cmake', 'test': mock.ANY}]),
]

@pytest.mark.parametrize(
    'build_only, test_only, retry_build_errors, expected_pipeline_elements',
    TESTDATA_18,
    ids=['none', 'retry', 'test+retry', 'test', 'build+test',
         'build+test+retry', 'build+retry', 'build']
)
def test_twisterrunner_add_tasks_to_queue(
    build_only,
    test_only,
    retry_build_errors,
    expected_pipeline_elements
):
    def mock_get_cmake_filter_stages(filter, keys):
        return [filter]

    instances = {
        'dummy1': mock.Mock(run=True, retries=0, status=TwisterStatus.PASS, build_dir="/tmp"),
        'dummy2': mock.Mock(run=True, retries=0, status=TwisterStatus.SKIP, build_dir="/tmp"),
        'dummy3': mock.Mock(run=True, retries=0, status=TwisterStatus.FILTER, build_dir="/tmp"),
        'dummy4': mock.Mock(run=True, retries=0, status=TwisterStatus.ERROR, build_dir="/tmp"),
        'dummy5': mock.Mock(run=True, retries=0, status=TwisterStatus.FAIL, build_dir="/tmp")
    }
    instances['dummy4'].testsuite.filter = 'some'
    instances['dummy5'].testsuite.filter = 'full'
    suites = [mock.Mock(), mock.Mock()]
    env_mock = mock.Mock()

    tr = TwisterRunner(instances, suites, env=env_mock)
    tr.get_cmake_filter_stages = mock.Mock(
        side_effect=mock_get_cmake_filter_stages
    )
    tr.results = mock.Mock(iteration=0)

    pipeline_mock = mock.Mock()

    tr.add_tasks_to_queue(
        pipeline_mock,
        build_only,
        test_only,
        retry_build_errors
    )

    assert all(
        [build_only != instance.run for instance in instances.values()]
    )

    tr.get_cmake_filter_stages.assert_any_call('full', mock.ANY)
    if retry_build_errors:
        tr.get_cmake_filter_stages.assert_any_call('some', mock.ANY)

    print(pipeline_mock.put.call_args_list)
    print([mock.call(el) for el in expected_pipeline_elements])

    assert pipeline_mock.put.call_args_list == \
           [mock.call(el) for el in expected_pipeline_elements]


TESTDATA_19 = [
    ('linux'),
    ('nt')
]

@pytest.mark.parametrize(
    'platform',
    TESTDATA_19,
)
def test_twisterrunner_pipeline_mgr(mocked_jobserver, platform):
    counter = 0
    def mock_get_nowait():
        nonlocal counter
        counter += 1
        if counter > 5:
            raise queue.Empty()
        return {'test': 'dummy'}

    instances = {}
    suites = []
    env_mock = mock.Mock()

    tr = TwisterRunner(instances, suites, env=env_mock)
    tr.jobserver = mock.Mock(
        get_job=mock.Mock(
            return_value=nullcontext()
        )
    )

    pipeline_mock = mock.Mock()
    pipeline_mock.get_nowait = mock.Mock(side_effect=mock_get_nowait)
    done_queue_mock = mock.Mock()
    lock_mock = mock.Mock()
    results_mock = mock.Mock()

    with mock.patch('sys.platform', platform), \
         mock.patch('twisterlib.runner.ProjectBuilder',\
                    return_value=mock.Mock()) as pb:
        tr.pipeline_mgr(pipeline_mock, done_queue_mock, lock_mock, results_mock)

    assert len(pb().process.call_args_list) == 5

    if platform == 'linux':
        tr.jobserver.get_job.assert_called_once()


def test_twisterrunner_execute(caplog):
    counter = 0
    def mock_join():
        nonlocal counter
        counter += 1
        if counter > 3:
            raise KeyboardInterrupt()

    instances = {}
    suites = []
    env_mock = mock.Mock()

    tr = TwisterRunner(instances, suites, env=env_mock)
    tr.add_tasks_to_queue = mock.Mock()
    tr.jobs = 5

    process_mock = mock.Mock()
    process_mock().join = mock.Mock(side_effect=mock_join)
    process_mock().exitcode = 0
    pipeline_mock = mock.Mock()
    done_mock = mock.Mock()

    with mock.patch('twisterlib.runner.Process', process_mock):
        tr.execute(pipeline_mock, done_mock)

    assert 'Execution interrupted' in caplog.text

    assert len(process_mock().start.call_args_list) == 5
    assert len(process_mock().join.call_args_list) == 4
    assert len(process_mock().terminate.call_args_list) == 5



TESTDATA_20 = [
    ('', []),
    ('not ARCH in ["x86", "arc"]', ['full']),
    ('dt_dummy(x, y)', ['dts']),
    ('not CONFIG_FOO', ['kconfig']),
    ('dt_dummy and CONFIG_FOO', ['dts', 'kconfig']),
]

@pytest.mark.parametrize(
    'filter, expected_result',
    TESTDATA_20,
    ids=['none', 'full', 'dts', 'kconfig', 'dts+kconfig']
)
def test_twisterrunner_get_cmake_filter_stages(filter, expected_result):
    result = TwisterRunner.get_cmake_filter_stages(filter, ['not', 'and'])

    assert sorted(result) == sorted(expected_result)
