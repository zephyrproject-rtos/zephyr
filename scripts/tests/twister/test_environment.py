#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
# Copyright (c) 2024 Arm Limited (or its affiliates). All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for environment.py classes' methods
"""

from unittest import mock
import os
import pytest
import shutil

from contextlib import nullcontext

import twisterlib.environment


TESTDATA_1 = [
    (
        None,
        None,
        None,
        ['--short-build-path', '-k'],
        '--short-build-path requires Ninja to be enabled'
    ),
    (
        'nt',
        None,
        None,
        ['--device-serial-pty', 'dummy'],
        '--device-serial-pty is not supported on Windows OS'
    ),
    (
        None,
        {
            'exist': [],
            'missing': ['valgrind']
        },
        None,
        ['--enable-valgrind'],
        'valgrind enabled but valgrind executable not found'
    ),
    (
        None,
        None,
        None,
        [
            '--device-testing',
            '--device-serial',
            'dummy',
        ],
        'When --device-testing is used with --device-serial' \
        ' or --device-serial-pty, exactly one platform must' \
        ' be specified'
    ),
    (
        None,
        None,
        None,
        [
            '--device-testing',
            '--device-serial',
            'dummy',
            '--platform',
            'dummy_platform1',
            '--platform',
            'dummy_platform2'
        ],
        'When --device-testing is used with --device-serial' \
        ' or --device-serial-pty, exactly one platform must' \
        ' be specified'
    ),
# Note the underscore.
    (
        None,
        None,
        None,
        ['--device-flash-with-test'],
        '--device-flash-with-test requires --device_testing'
    ),
    (
        None,
        None,
        None,
        ['--shuffle-tests'],
        '--shuffle-tests requires --subset'
    ),
    (
        None,
        None,
        None,
        ['--shuffle-tests-seed', '0'],
        '--shuffle-tests-seed requires --shuffle-tests'
    ),
    (
        None,
        None,
        None,
        ['/dummy/unrecognised/arg'],
        'Unrecognized arguments found: \'/dummy/unrecognised/arg\'.' \
        ' Use -- to delineate extra arguments for test binary' \
        ' or pass -h for help.'
    ),
    (
        None,
        None,
        True,
        [],
        'By default Twister should work without pytest-twister-harness' \
        ' plugin being installed, so please, uninstall it by' \
        ' `pip uninstall pytest-twister-harness` and' \
        ' `git clean -dxf scripts/pylib/pytest-twister-harness`.'
    ),
]


@pytest.mark.parametrize(
    'os_name, which_dict, pytest_plugin, args, expected_error',
    TESTDATA_1,
    ids=[
        'short build path without ninja',
        'device-serial-pty on Windows',
        'valgrind without executable',
        'device serial without platform',
        'device serial with multiple platforms',
        'device flash with test without device testing',
        'shuffle-tests without subset',
        'shuffle-tests-seed without shuffle-tests',
        'unrecognised argument',
        'pytest-twister-harness installed'
    ]
)
def test_parse_arguments_errors(
    caplog,
    os_name,
    which_dict,
    pytest_plugin,
    args,
    expected_error
):
    def mock_which(name):
        if name in which_dict['missing']:
            return False
        elif name in which_dict['exist']:
            return which_dict['path'][which_dict['exist']] \
                if which_dict['path'][which_dict['exist']] \
                else f'dummy/path/{name}'
        else:
            return f'dummy/path/{name}'

    with mock.patch('sys.argv', ['twister'] + args):
        parser = twisterlib.environment.add_parse_arguments()

    if which_dict:
        which_dict['path'] = {name: shutil.which(name) \
            for name in which_dict['exist']}
        which_mock = mock.Mock(side_effect=mock_which)

    with mock.patch('os.name', os_name) \
            if os_name is not None else nullcontext(), \
         mock.patch('shutil.which', which_mock) \
            if which_dict else nullcontext(), \
         mock.patch('twisterlib.environment' \
                    '.PYTEST_PLUGIN_INSTALLED', pytest_plugin) \
            if pytest_plugin is not None else nullcontext():
        with pytest.raises(SystemExit) as exit_info:
            twisterlib.environment.parse_arguments(parser, args)

    assert exit_info.value.code == 1
    assert expected_error in ' '.join(caplog.text.split())


def test_parse_arguments_errors_size():
    """`options.size` is not an error, rather a different functionality."""

    args = ['--size', 'dummy.elf']

    with mock.patch('sys.argv', ['twister'] + args):
        parser = twisterlib.environment.add_parse_arguments()

    mock_calc_parent = mock.Mock()
    mock_calc_parent.child = mock.Mock(return_value=mock.Mock())

    def mock_calc(*args, **kwargs):
        return mock_calc_parent.child(args, kwargs)

    with mock.patch('twisterlib.size_calc.SizeCalculator', mock_calc):
        with pytest.raises(SystemExit) as exit_info:
            twisterlib.environment.parse_arguments(parser, args)

    assert exit_info.value.code == 0

    mock_calc_parent.child.assert_has_calls([mock.call(('dummy.elf', []), {})])
    mock_calc_parent.child().size_report.assert_has_calls([mock.call()])


def test_parse_arguments_warnings(caplog):
    args = ['--allow-installed-plugin']

    with mock.patch('sys.argv', ['twister'] + args):
        parser = twisterlib.environment.add_parse_arguments()

    with mock.patch('twisterlib.environment.PYTEST_PLUGIN_INSTALLED', True):
        twisterlib.environment.parse_arguments(parser, args)

    assert 'You work with installed version of' \
           ' pytest-twister-harness plugin.' in ' '.join(caplog.text.split())


TESTDATA_2 = [
    (['--enable-size-report']),
    (['--compare-report', 'dummy']),
]


@pytest.mark.parametrize(
    'additional_args',
    TESTDATA_2,
    ids=['show footprint', 'compare report']
)
def test_parse_arguments(zephyr_base, additional_args):
    args = ['--coverage', '--platform', 'dummy_platform'] + \
           additional_args + ['--', 'dummy_extra_1', 'dummy_extra_2']

    with mock.patch('sys.argv', ['twister'] + args):
        parser = twisterlib.environment.add_parse_arguments()

    options = twisterlib.environment.parse_arguments(parser, args)

    assert os.path.join(zephyr_base, 'tests') in options.testsuite_root
    assert os.path.join(zephyr_base, 'samples') in options.testsuite_root

    assert options.enable_size_report

    assert options.enable_coverage

    assert options.coverage_platform == ['dummy_platform']

    assert options.extra_test_args == ['dummy_extra_1', 'dummy_extra_2']


TESTDATA_3 = [
    (
        mock.Mock(
            ninja=True,
            board_root=['dummy1', 'dummy2'],
            testsuite_root=[
                os.path.join('dummy', 'path', "tests"),
                os.path.join('dummy', 'path', "samples")
            ],
            outdir='dummy_abspath',
        ),
        mock.Mock(
            generator_cmd='ninja',
            generator='Ninja',
            test_roots=[
                os.path.join('dummy', 'path', "tests"),
                os.path.join('dummy', 'path', "samples")
            ],
            board_roots=['dummy1', 'dummy2'],
            outdir='dummy_abspath',
        )
    ),
    (
        mock.Mock(
            ninja=False,
            board_root='dummy0',
            testsuite_root=[
                os.path.join('dummy', 'path', "tests"),
                os.path.join('dummy', 'path', "samples")
            ],
            outdir='dummy_abspath',
        ),
        mock.Mock(
            generator_cmd='make',
            generator='Unix Makefiles',
            test_roots=[
                os.path.join('dummy', 'path', "tests"),
                os.path.join('dummy', 'path', "samples")
            ],
            board_roots=['dummy0'],
            outdir='dummy_abspath',
        )
    ),
]


@pytest.mark.parametrize(
    'options, expected_env',
    TESTDATA_3,
    ids=[
        'ninja',
        'make'
    ]
)
def test_twisterenv_init(options, expected_env):
    original_abspath = os.path.abspath

    def mocked_abspath(path):
        if path == 'dummy_abspath':
            return 'dummy_abspath'
        elif isinstance(path, mock.Mock):
            return None
        else:
            return original_abspath(path)

    with mock.patch('os.path.abspath', side_effect=mocked_abspath):
        twister_env = twisterlib.environment.TwisterEnv(options=options)

    assert twister_env.generator_cmd == expected_env.generator_cmd
    assert twister_env.generator == expected_env.generator

    assert twister_env.test_roots == expected_env.test_roots

    assert twister_env.board_roots == expected_env.board_roots
    assert twister_env.outdir == expected_env.outdir


def test_twisterenv_discover():
    options = mock.Mock(
        ninja=True
    )

    original_abspath = os.path.abspath

    def mocked_abspath(path):
        if path == 'dummy_abspath':
            return 'dummy_abspath'
        elif isinstance(path, mock.Mock):
            return None
        else:
            return original_abspath(path)

    with mock.patch('os.path.abspath', side_effect=mocked_abspath):
        twister_env = twisterlib.environment.TwisterEnv(options=options)

    mock_datetime = mock.Mock(
        now=mock.Mock(
            return_value=mock.Mock(
                isoformat=mock.Mock(return_value='dummy_time')
            )
        )
    )

    with mock.patch.object(
            twisterlib.environment.TwisterEnv,
            'check_zephyr_version',
            mock.Mock()) as mock_czv, \
         mock.patch.object(
            twisterlib.environment.TwisterEnv,
            'get_toolchain',
            mock.Mock()) as mock_gt, \
         mock.patch('twisterlib.environment.datetime', mock_datetime):
        twister_env.discover()

    mock_czv.assert_called_once()
    mock_gt.assert_called_once()
    assert twister_env.run_date == 'dummy_time'


TESTDATA_4 = [
    (
        mock.Mock(returncode=0, stdout='dummy stdout version'),
        mock.Mock(returncode=0, stdout='dummy stdout date'),
        ['Zephyr version: dummy stdout version'],
        'dummy stdout version',
        'dummy stdout date'
    ),
    (
        mock.Mock(returncode=0, stdout=''),
        mock.Mock(returncode=0, stdout='dummy stdout date'),
        ['Could not determine version'],
        'Unknown',
        'dummy stdout date'
    ),
    (
        mock.Mock(returncode=1, stdout='dummy stdout version'),
        mock.Mock(returncode=0, stdout='dummy stdout date'),
        ['Could not determine version'],
        'Unknown',
        'dummy stdout date'
    ),
    (
        OSError,
        mock.Mock(returncode=1),
        ['Could not determine version'],
        'Unknown',
        'Unknown'
    ),
]


@pytest.mark.parametrize(
    'git_describe_return, git_show_return, expected_logs,' \
    ' expected_version, expected_commit_date',
    TESTDATA_4,
    ids=[
        'valid',
        'no zephyr version on describe',
        'error on git describe',
        'execution error on git describe',
    ]
)
def test_twisterenv_check_zephyr_version(
    caplog,
    git_describe_return,
    git_show_return,
    expected_logs,
    expected_version,
    expected_commit_date
):
    def mock_run(command, *args, **kwargs):
        if all([keyword in command for keyword in ['git', 'describe']]):
            if isinstance(git_describe_return, type) and \
               issubclass(git_describe_return, Exception):
                raise git_describe_return()
            return git_describe_return
        if all([keyword in command for keyword in ['git', 'show']]):
            if isinstance(git_show_return, type) and \
               issubclass(git_show_return, Exception):
                raise git_show_return()
            return git_show_return

    options = mock.Mock(
        ninja=True
    )

    original_abspath = os.path.abspath

    def mocked_abspath(path):
        if path == 'dummy_abspath':
            return 'dummy_abspath'
        elif isinstance(path, mock.Mock):
            return None
        else:
            return original_abspath(path)

    with mock.patch('os.path.abspath', side_effect=mocked_abspath):
        twister_env = twisterlib.environment.TwisterEnv(options=options)

    with mock.patch('subprocess.run', mock.Mock(side_effect=mock_run)):
        twister_env.check_zephyr_version()
    print(expected_logs)
    print(caplog.text)
    assert twister_env.version == expected_version
    assert twister_env.commit_date == expected_commit_date
    assert all([expected_log in caplog.text for expected_log in expected_logs])


TESTDATA_5 = [
    (
        False,
        None,
        None,
        'Unable to find `cmake` in path',
        None
    ),
    (
        True,
        0,
        b'somedummy\x1B[123-@d1770',
        'Finished running dummy/script/path',
        {
            'returncode': 0,
            'msg': 'Finished running dummy/script/path',
            'stdout': 'somedummyd1770',
        }
    ),
    (
        True,
        1,
        b'another\x1B_dummy',
        'CMake script failure: dummy/script/path',
        {
            'returncode': 1,
            'returnmsg': 'anotherdummy'
        }
    ),
]


@pytest.mark.parametrize(
    'find_cmake, return_code, out, expected_log, expected_result',
    TESTDATA_5,
    ids=[
        'cmake not found',
        'regex sanitation 1',
        'regex sanitation 2'
    ]
)
def test_twisterenv_run_cmake_script(
    caplog,
    find_cmake,
    return_code,
    out,
    expected_log,
    expected_result
):
    def mock_which(name, *args, **kwargs):
        return 'dummy/cmake/path' if find_cmake else None

    def mock_popen(command, *args, **kwargs):
        return mock.Mock(
            pid=0,
            returncode=return_code,
            communicate=mock.Mock(
                return_value=(out, '')
            )
        )

    args = ['dummy/script/path', 'var1=val1']

    with mock.patch('shutil.which', mock_which), \
         mock.patch('subprocess.Popen', mock.Mock(side_effect=mock_popen)), \
         pytest.raises(Exception) \
            if not find_cmake else nullcontext() as exception:
        results = twisterlib.environment.TwisterEnv.run_cmake_script(args)

    assert 'Running cmake script dummy/script/path' in caplog.text

    assert expected_log in caplog.text

    if exception is not None:
        return

    assert expected_result.items() <= results.items()


TESTDATA_6 = [
    (
        {
            'returncode': 0,
            'stdout': '{\"ZEPHYR_TOOLCHAIN_VARIANT\": \"dummy toolchain\"}'
        },
        None,
        'Using \'dummy toolchain\' toolchain.'
    ),
    (
        {'returncode': 1},
        2,
        None
    ),
]


@pytest.mark.parametrize(
    'script_result, exit_value, expected_log',
    TESTDATA_6,
    ids=['valid', 'error']
)
def test_get_toolchain(caplog, script_result, exit_value, expected_log):
    options = mock.Mock(
        ninja=True
    )

    original_abspath = os.path.abspath

    def mocked_abspath(path):
        if path == 'dummy_abspath':
            return 'dummy_abspath'
        elif isinstance(path, mock.Mock):
            return None
        else:
            return original_abspath(path)

    with mock.patch('os.path.abspath', side_effect=mocked_abspath):
        twister_env = twisterlib.environment.TwisterEnv(options=options)

    with mock.patch.object(
            twisterlib.environment.TwisterEnv,
            'run_cmake_script',
            mock.Mock(return_value=script_result)), \
         pytest.raises(SystemExit) \
            if exit_value is not None else nullcontext() as exit_info:
        twister_env.get_toolchain()

    if exit_info is not None:
        assert exit_info.value.code == exit_value
    else:
        assert expected_log in caplog.text
