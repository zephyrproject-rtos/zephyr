#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
# Copyright (c) 2024 Arm Limited (or its affiliates). All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for handlers.py classes' methods
"""

import itertools
import mock
import os
import pytest
import signal
import subprocess
import sys

from contextlib import nullcontext
from importlib import reload
from serial import SerialException
from subprocess import CalledProcessError, TimeoutExpired
from types import SimpleNamespace

import twisterlib.harness

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")

from twisterlib.error import TwisterException
from twisterlib.statuses import TwisterStatus
from twisterlib.handlers import (
    Handler,
    BinaryHandler,
    DeviceHandler,
    QEMUHandler,
    SimulationHandler
)
from twisterlib.hardwaremap import (
    DUT
)

@pytest.fixture
def mocked_instance(tmp_path):
    instance = mock.Mock()

    testsuite = mock.Mock()
    type(testsuite).source_dir = mock.PropertyMock(return_value='')
    instance.testsuite = testsuite

    build_dir = tmp_path / 'build_dir'
    os.makedirs(build_dir)
    type(instance).build_dir = mock.PropertyMock(return_value=str(build_dir))

    platform = mock.Mock()
    type(platform).binaries = mock.PropertyMock(return_value=[])
    instance.platform = platform

    type(instance.testsuite).timeout = mock.PropertyMock(return_value=60)
    type(instance.platform).timeout_multiplier = mock.PropertyMock(
        return_value=2
    )

    instance.status = TwisterStatus.NONE
    instance.reason = 'Unknown'

    return instance


@pytest.fixture
def faux_timer():
    class Counter:
        def __init__(self):
            self.t = 0

        def time(self):
            self.t += 1
            return self.t

    return Counter()


TESTDATA_1 = [
    (True, False, 'posix', ['Install pyserial python module with pip to use' \
     ' --device-testing option.'], None),
    (False, True, 'nt', [], None),
    (True, True, 'posix', ['Install pyserial python module with pip to use' \
     ' --device-testing option.'], ImportError),
]

@pytest.mark.parametrize(
    'fail_serial, fail_pty, os_name, expected_outs, expected_error',
    TESTDATA_1,
    ids=['import serial', 'import pty nt', 'import serial+pty posix']
)
def test_imports(
    capfd,
    fail_serial,
    fail_pty,
    os_name,
    expected_outs,
    expected_error
):
    class ImportRaiser:
        def find_spec(self, fullname, path, target=None):
            if fullname == 'serial' and fail_serial:
                raise ImportError()
            if fullname == 'pty' and fail_pty:
                raise ImportError()

    modules_mock = sys.modules.copy()
    modules_mock['serial'] = None if fail_serial else modules_mock['serial']
    modules_mock['pty'] = None if fail_pty else modules_mock['pty']

    meta_path_mock = sys.meta_path[:]
    meta_path_mock.insert(0, ImportRaiser())

    with mock.patch('os.name', os_name), \
         mock.patch.dict('sys.modules', modules_mock, clear=True), \
         mock.patch('sys.meta_path', meta_path_mock), \
         pytest.raises(expected_error) if expected_error else nullcontext():
        reload(twisterlib.handlers)

    out, _ = capfd.readouterr()
    assert all([expected_out in out for expected_out in expected_outs])


def test_handler_final_handle_actions(mocked_instance):
    instance = mocked_instance
    instance.testcases = [mock.Mock()]

    handler = Handler(mocked_instance, 'build', mock.Mock())
    handler.suite_name_check = True

    harness = twisterlib.harness.Test()
    harness.status = 'NONE'
    harness.detected_suite_names = mock.Mock()
    harness.matched_run_id = False
    harness.run_id_exists = True
    harness.recording = mock.Mock()

    handler_time = mock.Mock()

    handler._final_handle_actions(harness, handler_time)

    assert handler.instance.status == TwisterStatus.FAIL
    assert handler.instance.execution_time == handler_time
    assert handler.instance.reason == 'RunID mismatch'
    assert all(testcase.status == TwisterStatus.FAIL for \
     testcase in handler.instance.testcases)

    handler.instance.reason = 'This reason shan\'t be changed.'
    handler._final_handle_actions(harness, handler_time)

    instance.assert_has_calls([mock.call.record(harness.recording)])

    assert handler.instance.reason == 'This reason shan\'t be changed.'


TESTDATA_2 = [
    (['dummy_testsuite_name'], False),
    ([], True),
    (['another_dummy_name', 'yet_another_dummy_name'], True),
]

@pytest.mark.parametrize(
    'detected_suite_names, should_be_called',
    TESTDATA_2,
    ids=['detected one expected', 'detected none', 'detected two unexpected']
)
def test_handler_verify_ztest_suite_name(
    mocked_instance,
    detected_suite_names,
    should_be_called
):
    instance = mocked_instance
    type(instance.testsuite).ztest_suite_names = ['dummy_testsuite_name']

    harness_status = TwisterStatus.PASS

    handler_time = mock.Mock()

    with mock.patch.object(Handler, '_missing_suite_name') as _missing_mocked:
        handler = Handler(instance, 'build', mock.Mock())
        handler._verify_ztest_suite_name(
            harness_status,
            detected_suite_names,
            handler_time
        )

        if should_be_called:
            _missing_mocked.assert_called_once()
        else:
            _missing_mocked.assert_not_called()


def test_handler_missing_suite_name(mocked_instance):
    instance = mocked_instance
    instance.testcases = [mock.Mock()]

    handler = Handler(mocked_instance, 'build', mock.Mock())
    handler.suite_name_check = True

    expected_suite_names = ['dummy_testsuite_name']

    handler_time = mock.Mock()

    handler._missing_suite_name(expected_suite_names, handler_time)

    assert handler.instance.status == TwisterStatus.FAIL
    assert handler.instance.execution_time == handler_time
    assert handler.instance.reason == 'Testsuite mismatch'
    assert all(
        testcase.status == TwisterStatus.FAIL for testcase in handler.instance.testcases
    )


def test_handler_terminate(mocked_instance):
    def mock_kill_function(pid, sig):
        if pid < 0:
            raise ProcessLookupError

    instance = mocked_instance

    handler = Handler(instance, 'build', mock.Mock())

    mock_process = mock.Mock()
    mock_child1 = mock.Mock(pid=1)
    mock_child2 = mock.Mock(pid=2)
    mock_process.children = mock.Mock(return_value=[mock_child1, mock_child2])

    mock_proc = mock.Mock(pid=0)
    mock_proc.terminate = mock.Mock(return_value=None)
    mock_proc.kill = mock.Mock(return_value=None)

    with mock.patch('psutil.Process', return_value=mock_process), \
         mock.patch(
        'os.kill',
        mock.Mock(side_effect=mock_kill_function)
    ) as mock_kill:
        handler.terminate(mock_proc)

        assert handler.terminated
        mock_proc.terminate.assert_called_once()
        mock_proc.kill.assert_called_once()
        mock_kill.assert_has_calls(
            [mock.call(1, signal.SIGTERM), mock.call(2, signal.SIGTERM)]
        )

        mock_child_neg1 = mock.Mock(pid=-1)
        mock_process.children = mock.Mock(
            return_value=[mock_child_neg1, mock_child2]
        )
        handler.terminated = False
        mock_kill.reset_mock()

        handler.terminate(mock_proc)

    mock_kill.assert_has_calls(
        [mock.call(-1, signal.SIGTERM), mock.call(2, signal.SIGTERM)]
    )


def test_binaryhandler_try_kill_process_by_pid(mocked_instance):
    def mock_kill_function(pid, sig):
        if pid < 0:
            raise ProcessLookupError

    instance = mocked_instance

    handler = BinaryHandler(instance, 'build', mock.Mock())
    handler.pid_fn = os.path.join('dummy', 'path', 'to', 'pid.pid')

    with mock.patch(
        'os.kill',
        mock.Mock(side_effect=mock_kill_function)
    ) as mock_kill, \
         mock.patch('os.unlink', mock.Mock()) as mock_unlink:
        with mock.patch('builtins.open', mock.mock_open(read_data='1')):
            handler.try_kill_process_by_pid()

        mock_unlink.assert_called_once_with(
            os.path.join('dummy', 'path', 'to', 'pid.pid')
        )
        mock_kill.assert_called_once_with(1, signal.SIGKILL)

        mock_unlink.reset_mock()
        mock_kill.reset_mock()
        handler.pid_fn = os.path.join('dummy', 'path', 'to', 'pid.pid')

        with mock.patch('builtins.open', mock.mock_open(read_data='-1')):
            handler.try_kill_process_by_pid()

        mock_unlink.assert_called_once_with(
            os.path.join('dummy', 'path', 'to', 'pid.pid')
        )
        mock_kill.assert_called_once_with(-1, signal.SIGKILL)


TESTDATA_3 = [
    (
        [b'This\\r\\n\n', b'is\r', b'some \x1B[31mANSI\x1B[39m in\n', b'a short\n', b'file.'],
        mock.Mock(status=TwisterStatus.NONE, capture_coverage=False),
        [
            mock.call('This\\r\\n\n'),
            mock.call('is\r'),
            mock.call('some ANSI in\n'),
            mock.call('a short\n'),
            mock.call('file.')
        ],
        [
            mock.call('This'),
            mock.call('is'),
            mock.call('some \x1B[31mANSI\x1B[39m in'),
            mock.call('a short'),
            mock.call('file.')
        ],
        None,
        False
    ),
    (
        [b'Too much.'] * 120,  # Should be more than the timeout
        mock.Mock(status=TwisterStatus.PASS, capture_coverage=False),
        None,
        None,
        True,
        False
    ),
    (
        [b'Too much.'] * 120,  # Should be more than the timeout
        mock.Mock(status=TwisterStatus.PASS, capture_coverage=False),
        None,
        None,
        True,
        False
    ),
    (
        [b'Too much.'] * 120,  # Should be more than the timeout
        mock.Mock(status=TwisterStatus.PASS, capture_coverage=True),
        None,
        None,
        False,
        True
    ),
]

@pytest.mark.parametrize(
    'proc_stdout, harness, expected_handler_calls,'
    ' expected_harness_calls, should_be_less, timeout_wait',
    TESTDATA_3,
    ids=[
        'no timeout',
        'timeout',
        'timeout with harness status',
        'timeout with capture_coverage, wait timeout'
    ]
)
def test_binaryhandler_output_handler(
    mocked_instance,
    faux_timer,
    proc_stdout,
    harness,
    expected_handler_calls,
    expected_harness_calls,
    should_be_less,
    timeout_wait
):
    class MockStdout(mock.Mock):
        def __init__(self, text):
            super().__init__(text)
            self.text = text
            self.line_index = 0

        def readline(self):
            if self.line_index == len(self.text):
                self.line_index = 0
                return b''
            else:
                line = self.text[self.line_index]
                self.line_index += 1
                return line

    class MockProc(mock.Mock):
        def __init__(self, pid, stdout):
            super().__init__(pid, stdout)
            self.pid = mock.PropertyMock(return_value=pid)
            self.stdout = MockStdout(stdout)

        def wait(self, *args, **kwargs):
            if timeout_wait:
                raise TimeoutExpired('dummy cmd', 'dummyamount')

    handler = BinaryHandler(mocked_instance, 'build', mock.Mock(timeout_multiplier=1))
    handler.terminate = mock.Mock()

    proc = MockProc(1, proc_stdout)

    with mock.patch(
        'builtins.open',
        mock.mock_open(read_data='')
    ) as mock_file, \
         mock.patch('time.time', side_effect=faux_timer.time):
        handler._output_handler(proc, harness)

        mock_file.assert_called_with(handler.log, 'wt')

    if expected_handler_calls:
        mock_file.return_value.write.assert_has_calls(expected_handler_calls)
    if expected_harness_calls:
        harness.handle.assert_has_calls(expected_harness_calls)
    if should_be_less is not None:
        if should_be_less:
            assert mock_file.return_value.write.call_count < len(proc_stdout)
        else:
            assert mock_file.return_value.write.call_count == len(proc_stdout)
    if timeout_wait:
        handler.terminate.assert_called_once_with(proc)


TESTDATA_4 = [
    (True, False, True, None, None,
     ['valgrind', '--error-exitcode=2', '--leak-check=full',
      f'--suppressions={ZEPHYR_BASE}/scripts/valgrind.supp',
      '--log-file=build_dir/valgrind.log', '--track-origins=yes',
      'generator']),
    (False, True, False, 123, None, ['generator', '-C', 'build_dir', 'run', '--seed=123']),
    (False, False, False, None, ['ex1', 'ex2'], ['build_dir/zephyr/zephyr.exe', 'ex1', 'ex2']),
]

@pytest.mark.parametrize(
    'robot_test, call_make_run, enable_valgrind, seed,' \
    ' extra_args, expected',
    TESTDATA_4,
    ids=['robot, valgrind', 'make run, seed', 'binary, extra']
)
def test_binaryhandler_create_command(
    mocked_instance,
    robot_test,
    call_make_run,
    enable_valgrind,
    seed,
    extra_args,
    expected
):
    options = SimpleNamespace()
    options.enable_valgrind = enable_valgrind
    options.coverage_basedir = "coverage_basedir"
    handler = BinaryHandler(mocked_instance, 'build', options, 'generator', False)
    handler.binary = 'bin'
    handler.call_make_run = call_make_run
    handler.seed = seed
    handler.extra_test_args = extra_args
    handler.build_dir = 'build_dir'
    handler.instance.sysbuild = False
    handler.platform = SimpleNamespace()
    handler.platform.resc = "file.resc"
    handler.platform.uart = "uart"

    command = handler._create_command(robot_test)

    assert command == expected


TESTDATA_5 = [
    (False, False, False),
    (True, False, False),
    (True, True, False),
    (False, False, True),
]

@pytest.mark.parametrize(
    'enable_asan, enable_lsan, enable_ubsan',
    TESTDATA_5,
    ids=['none', 'asan', 'asan, lsan', 'ubsan']
)
def test_binaryhandler_create_env(
    mocked_instance,
    enable_asan,
    enable_lsan,
    enable_ubsan
):
    handler = BinaryHandler(mocked_instance, 'build', mock.Mock(
        enable_asan=enable_asan,
        enable_lsan=enable_lsan,
        enable_ubsan=enable_ubsan
    ))

    env = {
        'example_env_var': True,
        'ASAN_OPTIONS': 'dummy=dummy:',
        'UBSAN_OPTIONS': 'dummy=dummy:'
    }

    with mock.patch('os.environ', env):
        res = handler._create_env()

    assert env['example_env_var'] == res['example_env_var']

    if enable_ubsan:
        assert env['UBSAN_OPTIONS'] in res['UBSAN_OPTIONS']
        assert 'log_path=stdout:' in res['UBSAN_OPTIONS']
        assert 'halt_on_error=1:' in res['UBSAN_OPTIONS']

    if enable_asan:
        assert env['ASAN_OPTIONS'] in res['ASAN_OPTIONS']
        assert 'log_path=stdout:' in res['ASAN_OPTIONS']

        if not enable_lsan:
            assert 'detect_leaks=0' in res['ASAN_OPTIONS']


TESTDATA_6 = [
    (TwisterStatus.NONE, False, 2, True, TwisterStatus.FAIL, 'Valgrind error', False),
    (TwisterStatus.NONE, False, 1, False, TwisterStatus.FAIL, 'Failed', False),
    (TwisterStatus.FAIL, False, 0, False, TwisterStatus.FAIL, 'Failed', False),
    ('success', False, 0, False, 'success', 'Unknown', False),
    (TwisterStatus.NONE, True, 1, True, TwisterStatus.FAIL, 'Timeout', True),
]

@pytest.mark.parametrize(
    'harness_status, terminated, returncode, enable_valgrind,' \
    ' expected_status, expected_reason, do_add_missing',
    TESTDATA_6,
    ids=['valgrind error', 'failed', 'harness failed', 'custom success', 'no status']
)
def test_binaryhandler_update_instance_info(
    mocked_instance,
    harness_status,
    terminated,
    returncode,
    enable_valgrind,
    expected_status,
    expected_reason,
    do_add_missing
):
    handler = BinaryHandler(mocked_instance, 'build', mock.Mock(
        enable_valgrind=enable_valgrind
    ))
    handler_time = 59
    handler.terminated = terminated
    handler.returncode = returncode
    missing_mock = mock.Mock()
    handler.instance.add_missing_case_status = missing_mock

    handler._update_instance_info(harness_status, handler_time)

    assert handler.instance.execution_time == handler_time

    assert handler.instance.status == expected_status
    assert handler.instance.reason == expected_reason

    if do_add_missing:
        missing_mock.assert_called_once_with(TwisterStatus.BLOCK, expected_reason)


TESTDATA_7 = [
    (True, False, False),
    (False, True, False),
    (False, False, True),
]

@pytest.mark.parametrize(
    'is_robot_test, coverage, isatty',
    TESTDATA_7,
    ids=['robot test', 'coverage', 'isatty']
)
def test_binaryhandler_handle(
    mocked_instance,
    caplog,
    is_robot_test,
    coverage,
    isatty
):
    thread_mock_obj = mock.Mock()

    def mock_popen(command, *args, **kwargs,):
        return mock.Mock(
          __enter__=mock.Mock(return_value=mock.Mock(pid=0, returncode=0)),
          __exit__=mock.Mock(return_value=None)
        )

    def mock_thread(target, *args, **kwargs):
        return thread_mock_obj

    handler = BinaryHandler(mocked_instance, 'build', mock.Mock(coverage=coverage))
    handler.sourcedir = 'source_dir'
    handler.build_dir = 'build_dir'
    handler.name= 'Dummy Name'
    handler._create_command = mock.Mock(return_value=['dummy' , 'command'])
    handler._create_env = mock.Mock(return_value=[])
    handler._update_instance_info = mock.Mock()
    handler._final_handle_actions = mock.Mock()
    handler.terminate = mock.Mock()
    handler.try_kill_process_by_pid = mock.Mock()

    robot_mock = mock.Mock()
    harness = mock.Mock(is_robot_test=is_robot_test, run_robot_test=robot_mock)

    popen_mock = mock.Mock(side_effect=mock_popen)
    thread_mock = mock.Mock(side_effect=mock_thread)
    call_mock = mock.Mock()

    with mock.patch('subprocess.call', call_mock), \
         mock.patch('subprocess.Popen', popen_mock), \
         mock.patch('threading.Thread', thread_mock), \
         mock.patch('sys.stdout.isatty', return_value=isatty):
        handler.handle(harness)

    if is_robot_test:
        robot_mock.assert_called_once_with(['dummy', 'command'], mock.ANY)
        return

    assert 'Spawning BinaryHandler Thread for Dummy Name' in caplog.text

    thread_mock_obj.join.assert_called()
    handler._update_instance_info.assert_called_once()
    handler._final_handle_actions.assert_called_once()

    if isatty:
        call_mock.assert_any_call(['stty', 'sane'], stdin=mock.ANY)


TESTDATA_8 = [
    ('renode', True, True, False, False),
    ('native', False, False, False, True),
    ('build', False, True, False, False),
]

@pytest.mark.parametrize(
    'type_str, is_pid_fn, expected_call_make_run, is_binary, expected_ready',
    TESTDATA_8,
    ids=[t[0] for t in TESTDATA_8]
)
def test_simulationhandler_init(
    mocked_instance,
    type_str,
    is_pid_fn,
    expected_call_make_run,
    is_binary,
    expected_ready
):
    handler = SimulationHandler(mocked_instance, type_str, mock.Mock())

    assert handler.call_make_run == expected_call_make_run
    assert handler.ready == expected_ready

    if is_pid_fn:
        assert handler.pid_fn == os.path.join(mocked_instance.build_dir,
                                              'renode.pid')
    if is_binary:
        assert handler.pid_fn == os.path.join(mocked_instance.build_dir,
                                              'zephyr', 'zephyr.exe')


TESTDATA_9 = [
    (3, 2, 0, 0, 3, -1, True, False, False, 1),
    (4, 1, 0, 0, -1, -1, False, True, False, 0),
    (5, 0, 1, 2, -1, 4, False, False, True, 3)
]

@pytest.mark.parametrize(
    'success_count, in_waiting_count, oserror_count, readline_error_count,'
    ' haltless_count, statusless_count, end_by_halt, end_by_close,'
    ' end_by_status, expected_line_count',
    TESTDATA_9,
    ids=[
      'halt event',
      'serial closes',
      'harness status with errors'
    ]
)
def test_devicehandler_monitor_serial(
    mocked_instance,
    success_count,
    in_waiting_count,
    oserror_count,
    readline_error_count,
    haltless_count,
    statusless_count,
    end_by_halt,
    end_by_close,
    end_by_status,
    expected_line_count
):
    is_open_iter = iter(lambda: True, False)
    line_iter = [
        TypeError('dummy TypeError') if x % 2 else \
        SerialException('dummy SerialException') for x in range(
            readline_error_count
        )
    ] + [
        f'line no {idx}'.encode('utf-8') for idx in range(success_count)
    ]
    in_waiting_iter = [False] * in_waiting_count + [
        TypeError('dummy TypeError')
    ] if end_by_close else (
        [OSError('dummy OSError')] * oserror_count + [False] * in_waiting_count
    ) + [True] * (success_count + readline_error_count)

    is_set_iter = [False] * haltless_count + [True] \
        if end_by_halt else iter(lambda: False, True)

    status_iter = [TwisterStatus.NONE] * statusless_count + [TwisterStatus.PASS] \
        if end_by_status else iter(lambda: TwisterStatus.NONE, TwisterStatus.PASS)

    halt_event = mock.Mock(is_set=mock.Mock(side_effect=is_set_iter))
    ser = mock.Mock(
        isOpen=mock.Mock(side_effect=is_open_iter),
        readline=mock.Mock(side_effect=line_iter)
    )
    type(ser).in_waiting = mock.PropertyMock(
        side_effect=in_waiting_iter,
        return_value=False
    )
    harness = mock.Mock(capture_coverage=False)
    type(harness).status=mock.PropertyMock(side_effect=status_iter)

    handler = DeviceHandler(mocked_instance, 'build', mock.Mock(enable_coverage=not end_by_status))

    with mock.patch('builtins.open', mock.mock_open(read_data='')):
        handler.monitor_serial(ser, halt_event, harness)

    if not end_by_close:
        ser.close.assert_called_once()

    print(harness.call_args_list)

    harness.handle.assert_has_calls(
        [mock.call(f'line no {idx}') for idx in range(expected_line_count)]
    )


def test_devicehandler_monitor_serial_splitlines(mocked_instance):
    halt_event = mock.Mock(is_set=mock.Mock(return_value=False))
    ser = mock.Mock(
        isOpen=mock.Mock(side_effect=[True, True, False]),
        in_waiting=mock.Mock(return_value=False),
        readline=mock.Mock(return_value='\nline1\nline2\n'.encode('utf-8'))
    )
    harness = mock.Mock(status=TwisterStatus.PASS)

    handler = DeviceHandler(mocked_instance, 'build', mock.Mock(enable_coverage=False))

    with mock.patch('builtins.open', mock.mock_open(read_data='')):
        handler.monitor_serial(ser, halt_event, harness)

    assert harness.handle.call_count == 2


TESTDATA_10 = [
    (
        'dummy_platform',
        'dummy fixture',
        [
            mock.Mock(
                fixtures=[],
                platform='dummy_platform',
                available=1,
                failures=0,
                counter_increment=mock.Mock(),
                counter=0
            ),
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='another_platform',
                available=1,
                failures=0,
                counter_increment=mock.Mock(),
                counter=0
            ),
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='dummy_platform',
                serial_pty=None,
                serial=None,
                available=1,
                failures=0,
                counter_increment=mock.Mock(),
                counter=0
            ),
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='dummy_platform',
                serial_pty=mock.Mock(),
                available=1,
                failures=0,
                counter_increment=mock.Mock(),
                counter=0
            ),
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='dummy_platform',
                serial_pty=mock.Mock(),
                available=1,
                failures=0,
                counter_increment=mock.Mock(),
                counter=0
            )
        ],
        3
    ),
    (
        'dummy_platform',
        'dummy fixture',
        [
            mock.Mock(
                fixtures=[],
                platform='dummy_platform',
                available=1,
                failures=0,
                counter_increment=mock.Mock(),
                counter=0
            ),
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='another_platform',
                available=1,
                failures=0,
                counter_increment=mock.Mock(),
                counter=0
            ),
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='dummy_platform',
                serial_pty=None,
                serial=None,
                available=1,
                failures=0,
                counter_increment=mock.Mock(),
                counter=0
            ),
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='dummy_platform',
                serial_pty=mock.Mock(),
                available=1,
                failures=1,
                counter_increment=mock.Mock(),
                counter=0
            ),
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='dummy_platform',
                serial_pty=mock.Mock(),
                available=1,
                failures=0,
                counter_increment=mock.Mock(),
                counter=0
            )
        ],
        4
    ),
    (
        'dummy_platform',
        'dummy fixture',
        [],
        TwisterException
    ),
    (
        'dummy_platform',
        'dummy fixture',
        [
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='dummy_platform',
                serial_pty=mock.Mock(),
                counter_increment=mock.Mock(),
                failures=0,
                available=0
            ),
            mock.Mock(
                fixtures=['another fixture'],
                platform='dummy_platform',
                serial_pty=mock.Mock(),
                counter_increment=mock.Mock(),
                failures=0,
                available=0
            ),
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='dummy_platform',
                serial=mock.Mock(),
                counter_increment=mock.Mock(),
                failures=0,
                available=0
            ),
            mock.Mock(
                fixtures=['another fixture'],
                platform='dummy_platform',
                serial=mock.Mock(),
                counter_increment=mock.Mock(),
                failures=0,
                available=0
            )
        ],
        None
    )
]

@pytest.mark.parametrize(
    'platform_name, fixture, duts, expected',
    TESTDATA_10,
    ids=['two good duts, select the first one',
         'two duts, the first was failed once, select the second not failed',
         'exception - no duts', 'no available duts']
)
def test_devicehandler_device_is_available(
    mocked_instance,
    platform_name,
    fixture,
    duts,
    expected
):
    mocked_instance.platform.name = platform_name
    mocked_instance.testsuite.harness_config = {'fixture': fixture}

    handler = DeviceHandler(mocked_instance, 'build', mock.Mock())
    handler.duts = duts

    if isinstance(expected, int):
        device = handler.device_is_available(mocked_instance)

        assert device == duts[expected]
        assert device.available == 0
        device.counter_increment.assert_called_once()
    elif expected is None:
        device = handler.device_is_available(mocked_instance)

        assert device is None
    elif isinstance(expected, type):
        with pytest.raises(expected):
            device = handler.device_is_available(mocked_instance)
    else:
        assert False


def test_devicehandler_make_dut_available(mocked_instance):
    serial = mock.Mock(name='dummy_serial')
    duts = [
        mock.Mock(available=0, serial=serial, serial_pty=None),
        mock.Mock(available=0, serial=None, serial_pty=serial),
        mock.Mock(
            available=0,
            serial=mock.Mock('another_serial'),
            serial_pty=None
        )
    ]

    handler = DeviceHandler(mocked_instance, 'build', mock.Mock())
    handler.duts = duts

    handler.make_dut_available(duts[1])

    assert len([None for d in handler.duts if d.available == 1]) == 1
    assert handler.duts[0].available == 0
    assert handler.duts[2].available == 0

    handler.make_dut_available(duts[0])

    assert len([None for d in handler.duts if d.available == 1]) == 2
    assert handler.duts[2].available == 0


TESTDATA_11 = [
    (mock.Mock(pid=0, returncode=0), False),
    (mock.Mock(pid=0, returncode=1), False),
    (mock.Mock(pid=0, returncode=1), True)
]

@pytest.mark.parametrize(
    'mock_process, raise_timeout',
    TESTDATA_11,
    ids=['proper script', 'error', 'timeout']
)
def test_devicehandler_run_custom_script(caplog, mock_process, raise_timeout):
    def raise_timeout_fn(timeout=-1):
        if raise_timeout and timeout != -1:
            raise subprocess.TimeoutExpired(None, timeout)
        else:
            return mock.Mock(), mock.Mock()

    def assert_popen(command, *args, **kwargs):
        return mock.Mock(
            __enter__=mock.Mock(return_value=mock_process),
            __exit__=mock.Mock(return_value=None)
        )

    mock_process.communicate = mock.Mock(side_effect=raise_timeout_fn)

    script = [os.path.join('test','script', 'path'), 'arg']
    timeout = 60

    with mock.patch('subprocess.Popen', side_effect=assert_popen):
        DeviceHandler.run_custom_script(script, timeout)

    if raise_timeout:
        assert all(
            t in caplog.text.lower() for t in [str(script), 'timed out']
        )
        mock_process.assert_has_calls(
            [
                mock.call.communicate(timeout=timeout),
                mock.call.kill(),
                mock.call.communicate()
            ]
        )
    elif mock_process.returncode == 0:
        assert not any([r.levelname == 'ERROR' for r in caplog.records])
    else:
        assert 'timed out' not in caplog.text.lower()
        assert 'custom script failure' in caplog.text.lower()


TESTDATA_12 = [
    (0, False),
    (4, False),
    (0, True)
]

@pytest.mark.parametrize(
    'num_of_failures, raise_exception',
    TESTDATA_12,
    ids=['no failures', 'with failures', 'exception']
)
def test_devicehandler_get_hardware(
    mocked_instance,
    caplog,
    num_of_failures,
    raise_exception
):
    expected_hardware = mock.Mock()

    def mock_availability(handler, instance, no=num_of_failures):
        if raise_exception:
            raise TwisterException(f'dummy message')
        if handler.no:
            handler.no -= 1
            return None
        return expected_hardware

    handler = DeviceHandler(mocked_instance, 'build', mock.Mock())
    handler.no = num_of_failures

    with mock.patch.object(
        DeviceHandler,
        'device_is_available',
        mock_availability
    ):
        hardware = handler.get_hardware()

    if raise_exception:
        assert 'dummy message' in caplog.text.lower()
        assert mocked_instance.status == TwisterStatus.FAIL
        assert mocked_instance.reason == 'dummy message'
    else:
        assert hardware == expected_hardware


TESTDATA_13 = [
    (
        None,
        None,
        None,
        ['generator_cmd', '-C', '$build_dir', 'flash']
    ),
    (
        [],
        None,
        None,
        ['west', 'flash', '--skip-rebuild', '-d', '$build_dir']
    ),
    (
        '--dummy',
        None,
        None,
        ['west', 'flash', '--skip-rebuild', '-d', '$build_dir',
         '--', '--dummy']
    ),
    (
        '--dummy1,--dummy2',
        None,
        None,
        ['west', 'flash', '--skip-rebuild', '-d', '$build_dir',
         '--', '--dummy1', '--dummy2']
    ),

    (
        None,
        'runner',
        'product',
        ['west', 'flash', '--skip-rebuild', '-d', '$build_dir',
         '--runner', 'runner', 'param1', 'param2']
    ),

    (
        None,
        'pyocd',
        'product',
        ['west', 'flash', '--skip-rebuild', '-d', '$build_dir',
         '--runner', 'pyocd', 'param1', 'param2', '--', '--dev-id', 12345]
    ),
    (
        None,
        'nrfjprog',
        'product',
        ['west', 'flash', '--skip-rebuild', '-d', '$build_dir',
         '--runner', 'nrfjprog', 'param1', 'param2', '--', '--dev-id', 12345]
    ),
    (
        None,
        'openocd',
        'STM32 STLink',
        ['west', 'flash', '--skip-rebuild', '-d', '$build_dir',
         '--runner', 'openocd', 'param1', 'param2',
         '--', '--cmd-pre-init', 'hla_serial 12345']
    ),
    (
        None,
        'openocd',
        'STLINK-V3',
        ['west', 'flash', '--skip-rebuild', '-d', '$build_dir',
         '--runner', 'openocd', 'param1', 'param2',
         '--', '--cmd-pre-init', 'hla_serial 12345']
    ),
    (
        None,
        'openocd',
        'EDBG CMSIS-DAP',
        ['west', 'flash', '--skip-rebuild', '-d', '$build_dir',
         '--runner', 'openocd', 'param1', 'param2',
         '--', '--cmd-pre-init', 'cmsis_dap_serial 12345']
    ),
    (
        None,
        'jlink',
        'product',
        ['west', 'flash', '--skip-rebuild', '-d', '$build_dir',
         '--runner', 'jlink', '--dev-id', 12345,
         'param1', 'param2']
    ),
    (
        None,
        'stm32cubeprogrammer',
        'product',
        ['west', 'flash', '--skip-rebuild', '-d', '$build_dir',
         '--runner', 'stm32cubeprogrammer', '--tool-opt=sn=12345',
         'param1', 'param2']
    ),

]

TESTDATA_13_2 = [(True), (False)]

@pytest.mark.parametrize(
    'self_west_flash, runner,' \
    ' hardware_product_name, expected',
    TESTDATA_13,
    ids=['generator', '--west-flash', 'one west flash value',
         'multiple west flash values', 'generic runner', 'pyocd',
         'nrfjprog', 'openocd, STM32 STLink', 'openocd, STLINK-v3',
         'openocd, EDBG CMSIS-DAP', 'jlink', 'stm32cubeprogrammer']
)
@pytest.mark.parametrize('hardware_probe', TESTDATA_13_2, ids=['probe', 'id'])
def test_devicehandler_create_command(
    mocked_instance,
    self_west_flash,
    runner,
    hardware_probe,
    hardware_product_name,
    expected
):
    handler = DeviceHandler(mocked_instance, 'build', mock.Mock())
    handler.options = mock.Mock(west_flash=self_west_flash)
    handler.generator_cmd = 'generator_cmd'

    expected = [handler.build_dir if val == '$build_dir' else \
                val for val in expected]

    hardware = mock.Mock(
        product=hardware_product_name,
        probe_id=12345 if hardware_probe else None,
        id=12345 if not hardware_probe else None,
        runner_params=['param1', 'param2']
    )

    command = handler._create_command(runner, hardware)

    assert command == expected


TESTDATA_14 = [
    ('success', False, 'success', 'Unknown', False),
    (TwisterStatus.FAIL, False, TwisterStatus.FAIL, 'Failed', True),
    (TwisterStatus.ERROR, False, TwisterStatus.ERROR, 'Unknown', True),
    (TwisterStatus.NONE, True, TwisterStatus.NONE, 'Unknown', False),
    (TwisterStatus.NONE, False, TwisterStatus.FAIL, 'Timeout', True),
]

@pytest.mark.parametrize(
    'harness_status, flash_error,' \
    ' expected_status, expected_reason, do_add_missing',
    TESTDATA_14,
    ids=['custom success', 'failed', 'error', 'flash error', 'no status']
)
def test_devicehandler_update_instance_info(
        mocked_instance,
        harness_status,
        flash_error,
        expected_status,
        expected_reason,
        do_add_missing
        ):
    handler = DeviceHandler(mocked_instance, 'build', mock.Mock())
    handler_time = 59
    missing_mock = mock.Mock()
    handler.instance.add_missing_case_status = missing_mock

    handler._update_instance_info(harness_status, handler_time, flash_error)

    assert handler.instance.execution_time == handler_time

    assert handler.instance.status == expected_status
    assert handler.instance.reason == expected_reason

    if do_add_missing:
        missing_mock.assert_called_with('blocked', expected_reason)


TESTDATA_15 = [
    ('dummy device', 'dummy pty', None, None, True, False, False),
    (
        'dummy device',
        'dummy pty',
        mock.Mock(communicate=mock.Mock(return_value=('', ''))),
        SerialException,
        False,
        True,
        'dummy pty'
    ),
    (
        'dummy device',
        None,
        None,
        SerialException,
        False,
        False,
        'dummy device'
    )
]

@pytest.mark.parametrize(
    'serial_device, serial_pty, ser_pty_process, expected_exception,' \
    ' expected_result, terminate_ser_pty_process, make_available',
    TESTDATA_15,
    ids=['valid', 'serial pty process', 'no serial pty']
)
def test_devicehandler_create_serial_connection(
    mocked_instance,
    serial_device,
    serial_pty,
    ser_pty_process,
    expected_exception,
    expected_result,
    terminate_ser_pty_process,
    make_available
):
    def mock_serial(*args, **kwargs):
        if expected_exception:
            raise expected_exception('')
        return expected_result

    handler = DeviceHandler(mocked_instance, 'build', mock.Mock(timeout_multiplier=1))
    missing_mock = mock.Mock()
    handler.instance.add_missing_case_status = missing_mock
    twisterlib.handlers.terminate_process = mock.Mock()

    dut = DUT()
    dut.available = 0
    dut.failures = 0

    hardware_baud = 14400
    flash_timeout = 60
    serial_mock = mock.Mock(side_effect=mock_serial)

    with mock.patch('serial.Serial', serial_mock), \
         pytest.raises(expected_exception) if expected_exception else \
         nullcontext():
        result = handler._create_serial_connection(dut, serial_device, hardware_baud,
                                                   flash_timeout, serial_pty,
                                                   ser_pty_process)

    if expected_result:
        assert result is not None
        assert dut.failures == 0

    if expected_exception:
        assert handler.instance.status == TwisterStatus.FAIL
        assert handler.instance.reason == 'Serial Device Error'
        assert dut.available == 1
        assert dut.failures == 1
        missing_mock.assert_called_once_with('blocked', 'Serial Device Error')

    if terminate_ser_pty_process:
        twisterlib.handlers.terminate_process.assert_called_once()
        ser_pty_process.communicate.assert_called_once()


TESTDATA_16 = [
    ('dummy1 dummy2', None, 'slave name'),
    ('dummy1,dummy2', CalledProcessError, None),
    (None, None, 'dummy hardware serial'),
]

@pytest.mark.parametrize(
    'serial_pty, popen_exception, expected_device',
    TESTDATA_16,
    ids=['pty', 'pty process error', 'no pty']
)
def test_devicehandler_get_serial_device(
    mocked_instance,
    serial_pty,
    popen_exception,
    expected_device
):
    def mock_popen(command, *args, **kwargs):
        assert command == ['dummy1', 'dummy2']
        if popen_exception:
            raise popen_exception(command, 'Dummy error')
        return mock.Mock()

    handler = DeviceHandler(mocked_instance, 'build', mock.Mock())
    hardware_serial = 'dummy hardware serial'

    popen_mock = mock.Mock(side_effect=mock_popen)
    openpty_mock = mock.Mock(return_value=('master', 'slave'))
    ttyname_mock = mock.Mock(side_effect=lambda x: x + ' name')

    with mock.patch('subprocess.Popen', popen_mock), \
         mock.patch('pty.openpty', openpty_mock), \
         mock.patch('os.ttyname', ttyname_mock):
        result = handler._get_serial_device(serial_pty, hardware_serial)

    if popen_exception:
        assert result is None
    else:
        assert result[0] == expected_device

TESTDATA_17 = [
    (False, False, False, False, None, False, False,
     TwisterStatus.NONE, None, []),
    (True, True, False, False, None, False, False,
     TwisterStatus.NONE, None, []),
    (True, False, True, False, None, False, False,
     TwisterStatus.ERROR, 'Device issue (Flash error)', []),
    (True, False, False, True, None, False, False,
     TwisterStatus.ERROR, 'Device issue (Timeout)', ['Flash operation timed out.']),
    (True, False, False, False, 1, False, False,
     TwisterStatus.ERROR, 'Device issue (Flash error?)', []),
    (True, False, False, False, 0, True, False,
     TwisterStatus.NONE, None, ['Timed out while monitoring serial output on IPName']),
    (True, False, False, False, 0, False, True,
     TwisterStatus.NONE, None, ["Terminating serial-pty:'Serial PTY'",
                                "Terminated serial-pty:'Serial PTY', stdout:'', stderr:''"]),
]

@pytest.mark.parametrize(
    'has_hardware, raise_create_serial, raise_popen, raise_timeout,' \
    ' returncode, do_timeout_thread, use_pty,' \
    ' expected_status, expected_reason, expected_logs',
    TESTDATA_17,
    ids=['no hardware', 'create serial failure', 'popen called process error',
         'communicate timeout', 'nonzero returncode', 'valid pty', 'valid dev']
)
def test_devicehandler_handle(
    mocked_instance,
    caplog,
    has_hardware,
    raise_create_serial,
    raise_popen,
    raise_timeout,
    returncode,
    do_timeout_thread,
    use_pty,
    expected_status,
    expected_reason,
    expected_logs
):
    def mock_get_serial(serial_pty, hardware_serial):
        if serial_pty:
            serial_pty_process = mock.Mock(
                name='dummy serial PTY process',
                communicate=mock.Mock(
                    return_value=('', '')
                )
            )
            return 'dummy serial PTY device', serial_pty_process
        return 'dummy serial device', None

    def mock_create_serial(*args, **kwargs):
        if raise_create_serial:
            raise SerialException('dummy cmd', 'dummy msg')
        return mock.Mock(name='dummy serial')

    def mock_thread(*args, **kwargs):
        is_alive_mock = mock.Mock(return_value=bool(do_timeout_thread))

        return mock.Mock(is_alive=is_alive_mock)

    def mock_terminate(proc, *args, **kwargs):
        proc.communicate = mock.Mock(return_value=(mock.Mock(), mock.Mock()))

    def mock_communicate(*args, **kwargs):
        if raise_timeout:
            raise TimeoutExpired('dummy cmd', 'dummyamount')
        return mock.Mock(), mock.Mock()

    def mock_popen(command, *args, **kwargs):
        if raise_popen:
            raise CalledProcessError('dummy proc', 'dummy msg')

        mock_process = mock.Mock(
            pid=1,
            returncode=returncode,
            communicate=mock.Mock(side_effect=mock_communicate)
        )

        return mock.Mock(
            __enter__=mock.Mock(return_value=mock_process),
            __exit__=mock.Mock(return_value=None)
        )

    hardware = None if not has_hardware else mock.Mock(
        baud=14400,
        runner='dummy runner',
        serial_pty='Serial PTY' if use_pty else None,
        serial='dummy serial',
        pre_script='dummy pre script',
        post_script='dummy post script',
        post_flash_script='dummy post flash script',
        flash_timeout=60,
        flash_with_test=True
    )

    handler = DeviceHandler(mocked_instance, 'build', mock.Mock())
    handler.get_hardware = mock.Mock(return_value=hardware)
    handler.options = mock.Mock(
        timeout_multiplier=1,
        west_flash=None,
        west_runner=None
    )
    handler._get_serial_device = mock.Mock(side_effect=mock_get_serial)
    handler._create_command = mock.Mock(return_value=['dummy', 'command'])
    handler.run_custom_script = mock.Mock()
    handler._create_serial_connection = mock.Mock(
        side_effect=mock_create_serial
    )
    handler.monitor_serial = mock.Mock()
    handler.terminate = mock.Mock(side_effect=mock_terminate)
    handler._update_instance_info = mock.Mock()
    handler._final_handle_actions = mock.Mock()
    handler.make_dut_available = mock.Mock()
    twisterlib.handlers.terminate_process = mock.Mock()
    handler.instance.platform.name = 'IPName'

    harness = mock.Mock()

    with mock.patch('builtins.open', mock.mock_open(read_data='')), \
         mock.patch('subprocess.Popen', side_effect=mock_popen), \
         mock.patch('threading.Event', mock.Mock()), \
         mock.patch('threading.Thread', side_effect=mock_thread):
        handler.handle(harness)

    handler.get_hardware.assert_called_once()

    messages = [record.msg for record in caplog.records]
    assert all([msg in messages for msg in expected_logs])

    if not has_hardware:
        return

    handler.run_custom_script.assert_has_calls([
        mock.call('dummy pre script', mock.ANY)
    ])

    if raise_create_serial:
        return

    handler.run_custom_script.assert_has_calls([
        mock.call('dummy pre script', mock.ANY),
        mock.call('dummy post flash script', mock.ANY),
        mock.call('dummy post script', mock.ANY)
    ])

    if expected_reason:
        assert handler.instance.reason == expected_reason
    if expected_status:
        assert handler.instance.status == expected_status

    handler.make_dut_available.assert_called_once_with(hardware)


TESTDATA_18 = [
    (True, True, True),
    (False, False, False),
]

@pytest.mark.parametrize(
    'ignore_qemu_crash, expected_ignore_crash, expected_ignore_unexpected_eof',
    TESTDATA_18,
    ids=['ignore crash', 'qemu crash']
)
def test_qemuhandler_init(
    mocked_instance,
    ignore_qemu_crash,
    expected_ignore_crash,
    expected_ignore_unexpected_eof
):
    mocked_instance.testsuite.ignore_qemu_crash = ignore_qemu_crash

    handler = QEMUHandler(mocked_instance, 'build', mock.Mock())

    assert handler.ignore_qemu_crash == expected_ignore_crash
    assert handler.ignore_unexpected_eof == expected_ignore_unexpected_eof


def test_qemuhandler_get_cpu_time():
    def mock_process(pid):
        return mock.Mock(
            cpu_times=mock.Mock(
                return_value=mock.Mock(
                    user=20.0,
                    system=64.0
                )
            )
        )

    with mock.patch('psutil.Process', mock_process):
        res = QEMUHandler._get_cpu_time(0)

    assert res == pytest.approx(84.0)


TESTDATA_19 = [
    (
        True,
        os.path.join('self', 'dummy_dir', '1'),
        mock.PropertyMock(return_value=os.path.join('dummy_dir', '1')),
        os.path.join('dummy_dir', '1')
    ),
    (
        False,
        os.path.join('self', 'dummy_dir', '2'),
        mock.PropertyMock(return_value=os.path.join('dummy_dir', '2')),
        os.path.join('self', 'dummy_dir', '2')
    ),
]

@pytest.mark.parametrize(
    'self_sysbuild, self_build_dir, build_dir, expected',
    TESTDATA_19,
    ids=['domains build dir', 'self build dir']
)
def test_qemuhandler_get_default_domain_build_dir(
    mocked_instance,
    self_sysbuild,
    self_build_dir,
    build_dir,
    expected
):
    get_default_domain_mock = mock.Mock()
    type(get_default_domain_mock()).build_dir = build_dir
    domains_mock = mock.Mock(get_default_domain=get_default_domain_mock)
    from_file_mock = mock.Mock(return_value=domains_mock)

    handler = QEMUHandler(mocked_instance, 'build', mock.Mock())
    handler.instance.sysbuild = self_sysbuild
    handler.build_dir = self_build_dir

    with mock.patch('domains.Domains.from_file', from_file_mock):
        result = handler.get_default_domain_build_dir()

    assert result == expected


TESTDATA_20 = [
    (
        os.path.join('self', 'dummy_dir', 'log1'),
        os.path.join('self', 'dummy_dir', 'pid1'),
        os.path.join('sysbuild', 'dummy_dir', 'bd1'),
        True
    ),
    (
        os.path.join('self', 'dummy_dir', 'log2'),
        os.path.join('self', 'dummy_dir', 'pid2'),
        os.path.join('sysbuild', 'dummy_dir', 'bd2'),
        False
    ),
]

@pytest.mark.parametrize(
    'self_log, self_pid_fn, sysbuild_build_dir, exists_pid_fn',
    TESTDATA_20,
    ids=['pid exists', 'pid missing']
)
def test_qemuhandler_set_qemu_filenames(
    mocked_instance,
    self_log,
    self_pid_fn,
    sysbuild_build_dir,
    exists_pid_fn
):
    unlink_mock = mock.Mock()
    exists_mock = mock.Mock(return_value=exists_pid_fn)

    handler = QEMUHandler(mocked_instance, 'build', mock.Mock())
    handler.log = self_log
    handler.pid_fn = self_pid_fn

    with mock.patch('os.unlink', unlink_mock), \
         mock.patch('os.path.exists', exists_mock):
        handler._set_qemu_filenames(sysbuild_build_dir)

    assert handler.fifo_fn == mocked_instance.build_dir + \
                              os.path.sep + 'qemu-fifo'

    assert handler.pid_fn ==  sysbuild_build_dir + os.path.sep + 'qemu.pid'

    assert handler.log_fn == self_log

    if exists_pid_fn:
        unlink_mock.assert_called_once_with(sysbuild_build_dir + \
                                            os.path.sep + 'qemu.pid')


def test_qemuhandler_create_command(mocked_instance):
    sysbuild_build_dir = os.path.join('sysbuild', 'dummy_dir')

    handler = QEMUHandler(mocked_instance, 'build', mock.Mock(), 'dummy_cmd', False)

    result = handler._create_command(sysbuild_build_dir)

    assert result == ['dummy_cmd', '-C', 'sysbuild' + os.path.sep + 'dummy_dir',
                      'run']


TESTDATA_21 = [
    (
        0,
        False,
        None,
        'good dummy status',
        False,
        TwisterStatus.NONE,
        None,
        False
    ),
    (
        1,
        True,
        None,
        'good dummy status',
        False,
        TwisterStatus.NONE,
        None,
        False
    ),
    (
        0,
        False,
        None,
        TwisterStatus.NONE,
        True,
        TwisterStatus.FAIL,
        'Timeout',
        True
    ),
    (
        1,
        False,
        None,
        TwisterStatus.NONE,
        False,
        TwisterStatus.FAIL,
        'Exited with 1',
        True
    ),
    (
        1,
        False,
        'preexisting reason',
        'good dummy status',
        False,
        TwisterStatus.FAIL,
        'preexisting reason',
        True
    ),
]

@pytest.mark.parametrize(
    'self_returncode, self_ignore_qemu_crash,' \
    ' self_instance_reason, harness_status, is_timeout,' \
    ' expected_status, expected_reason, expected_called_missing_case',
    TESTDATA_21,
    ids=['not failed', 'qemu ignore', 'timeout', 'bad returncode', 'other fail']
)
def test_qemuhandler_update_instance_info(
    mocked_instance,
    self_returncode,
    self_ignore_qemu_crash,
    self_instance_reason,
    harness_status,
    is_timeout,
    expected_status,
    expected_reason,
    expected_called_missing_case
):
    mocked_instance.add_missing_case_status = mock.Mock()
    mocked_instance.reason = self_instance_reason

    handler = QEMUHandler(mocked_instance, 'build', mock.Mock())
    handler.returncode = self_returncode
    handler.ignore_qemu_crash = self_ignore_qemu_crash

    handler._update_instance_info(harness_status, is_timeout)

    assert handler.instance.status == expected_status
    assert handler.instance.reason == expected_reason

    if expected_called_missing_case:
        mocked_instance.add_missing_case_status.assert_called_once_with(
            TwisterStatus.BLOCK
        )


def test_qemuhandler_thread_get_fifo_names():
    fifo_fn = 'dummy'

    fifo_in, fifo_out = QEMUHandler._thread_get_fifo_names(fifo_fn)

    assert fifo_in ==  'dummy.in'
    assert fifo_out ==  'dummy.out'

TESTDATA_22 = [
    (False, False),
    (False, True),
    (True, False),
    (True, True),
]

@pytest.mark.parametrize(
    'fifo_in_exists, fifo_out_exists',
    TESTDATA_22,
    ids=['both missing', 'out exists', 'in exists', 'both exist']
)
def test_qemuhandler_thread_open_files(fifo_in_exists, fifo_out_exists):
    def mock_exists(path):
        if path == 'fifo.in':
            return fifo_in_exists
        elif path == 'fifo.out':
            return fifo_out_exists
        else:
            raise ValueError('Unexpected path in mock of os.path.exists')

    unlink_mock = mock.Mock()
    exists_mock = mock.Mock(side_effect=mock_exists)
    mkfifo_mock = mock.Mock()

    fifo_in = 'fifo.in'
    fifo_out = 'fifo.out'
    logfile = 'log.file'

    with mock.patch('os.unlink', unlink_mock), \
         mock.patch('os.mkfifo', mkfifo_mock), \
         mock.patch('os.path.exists', exists_mock), \
         mock.patch('builtins.open', mock.mock_open()) as open_mock:
        _, _, _ = QEMUHandler._thread_open_files(fifo_in, fifo_out, logfile)

    open_mock.assert_has_calls([
        mock.call('fifo.in', 'wb'),
        mock.call('fifo.out', 'rb', buffering=0),
        mock.call('log.file', 'wt'),
    ])

    if fifo_in_exists:
        unlink_mock.assert_any_call('fifo.in')

    if fifo_out_exists:
        unlink_mock.assert_any_call('fifo.out')


TESTDATA_23 = [
    (False, False),
    (True, True),
    (True, False)
]

@pytest.mark.parametrize(
    'is_pid, is_lookup_error',
    TESTDATA_23,
    ids=['pid missing', 'pid lookup error', 'pid ok']
)
def test_qemuhandler_thread_close_files(is_pid, is_lookup_error):
    is_process_killed = {}

    def mock_kill(pid, sig):
        if is_lookup_error:
            raise ProcessLookupError(f'Couldn\'t find pid: {pid}.')
        elif sig == signal.SIGTERM:
            is_process_killed[pid] = True

    unlink_mock = mock.Mock()
    kill_mock = mock.Mock(side_effect=mock_kill)

    fifo_in = 'fifo.in'
    fifo_out = 'fifo.out'
    pid = 12345 if is_pid else None
    out_fp = mock.Mock()
    in_fp = mock.Mock()
    log_out_fp = mock.Mock()

    with mock.patch('os.unlink', unlink_mock), \
         mock.patch('os.kill', kill_mock):
        QEMUHandler._thread_close_files(fifo_in, fifo_out, pid, out_fp,
                                        in_fp, log_out_fp)

    out_fp.close.assert_called_once()
    in_fp.close.assert_called_once()
    log_out_fp.close.assert_called_once()

    unlink_mock.assert_has_calls([mock.call('fifo.in'), mock.call('fifo.out')])

    if is_pid and not is_lookup_error:
        assert is_process_killed[pid]


TESTDATA_24 = [
    (TwisterStatus.FAIL, 'timeout', TwisterStatus.FAIL, 'timeout'),
    (TwisterStatus.FAIL, 'Execution error', TwisterStatus.FAIL, 'Execution error'),
    (TwisterStatus.FAIL, 'unexpected eof', TwisterStatus.FAIL, 'unexpected eof'),
    (TwisterStatus.FAIL, 'unexpected byte', TwisterStatus.FAIL, 'unexpected byte'),
    (TwisterStatus.NONE, None, TwisterStatus.NONE, 'Unknown'),
]

@pytest.mark.parametrize(
    '_status, _reason, expected_status, expected_reason',
    TESTDATA_24,
    ids=['timeout', 'failed', 'unexpected eof', 'unexpected byte', 'unknown']
)
def test_qemuhandler_thread_update_instance_info(
    mocked_instance,
    _status,
    _reason,
    expected_status,
    expected_reason
):
    handler = QEMUHandler(mocked_instance, 'build', mock.Mock())
    handler_time = 59

    QEMUHandler._thread_update_instance_info(handler, handler_time, _status, _reason)

    assert handler.instance.execution_time == handler_time

    assert handler.instance.status == expected_status
    assert handler.instance.reason == expected_reason


TESTDATA_25 = [
    (
        ('1\n' * 60).encode('utf-8'),
        60,
        1,
        [TwisterStatus.NONE] * 60 + [TwisterStatus.PASS] * 6,
        1000,
        False,
        TwisterStatus.FAIL,
        'timeout',
        [mock.call('1\n'), mock.call('1\n')]
    ),
    (
        ('1\n' * 60).encode('utf-8'),
        60,
        -1,
        [TwisterStatus.NONE] * 60 + [TwisterStatus.PASS] * 30,
        100,
        False,
        TwisterStatus.FAIL,
        None,
        [mock.call('1\n'), mock.call('1\n')]
    ),
    (
        b'',
        60,
        1,
        [TwisterStatus.PASS] * 3,
        100,
        False,
        TwisterStatus.FAIL,
        'unexpected eof',
        []
    ),
    (
        b'\x81',
        60,
        1,
        [TwisterStatus.PASS] * 3,
        100,
        False,
        TwisterStatus.FAIL,
        'unexpected byte',
        []
    ),
    (
        '1\n2\n3\n4\n5\n'.encode('utf-8'),
        600,
        1,
        [TwisterStatus.NONE] * 3 + [TwisterStatus.PASS] * 7,
        100,
        False,
        TwisterStatus.PASS,
        None,
        [mock.call('1\n'), mock.call('2\n'), mock.call('3\n'), mock.call('4\n')]
    ),
    (
        '1\n2\n3\n4\n5\n'.encode('utf-8'),
        600,
        0,
        [TwisterStatus.NONE] * 3 + [TwisterStatus.PASS] * 7,
        100,
        False,
        TwisterStatus.FAIL,
        'timeout',
        [mock.call('1\n'), mock.call('2\n')]
    ),
    (
        '1\n2\n3\n4\n5\n'.encode('utf-8'),
        60,
        1,
        [TwisterStatus.NONE] * 3 + [TwisterStatus.PASS] * 7,
        (n for n in [100, 100, 10000]),
        True,
        TwisterStatus.PASS,
        None,
        [mock.call('1\n'), mock.call('2\n'), mock.call('3\n'), mock.call('4\n')]
    ),
]

@pytest.mark.parametrize(
    'content, timeout, pid, harness_statuses, cputime, capture_coverage,' \
    ' expected_status, expected_reason, expected_log_calls',
    TESTDATA_25,
    ids=[
        'timeout',
        'harness failed',
        'unexpected eof',
        'unexpected byte',
        'harness success',
        'timeout by pid=0',
        'capture_coverage'
    ]
)
def test_qemuhandler_thread(
    mocked_instance,
    faux_timer,
    content,
    timeout,
    pid,
    harness_statuses,
    cputime,
    capture_coverage,
    expected_status,
    expected_reason,
    expected_log_calls
):
    def mock_cputime(pid):
        if pid > 0:
            return cputime if isinstance(cputime, int) else next(cputime)
        else:
            raise ProcessLookupError()

    type(mocked_instance.testsuite).timeout = mock.PropertyMock(return_value=timeout)
    handler = QEMUHandler(mocked_instance, 'build', mock.Mock(timeout_multiplier=1))
    handler.ignore_unexpected_eof = False
    handler.pid_fn = 'pid_fn'
    handler.fifo_fn = 'fifo_fn'

    def mocked_open(filename, *args, **kwargs):
        if filename == handler.pid_fn:
            contents = str(pid).encode('utf-8')
        elif filename == handler.fifo_fn + '.out':
            contents = content
        else:
            contents = b''

        file_object = mock.mock_open(read_data=contents).return_value
        file_object.__iter__.return_value = contents.splitlines(True)
        return file_object

    harness = mock.Mock(capture_coverage=capture_coverage, handle=print)
    type(harness).status = mock.PropertyMock(side_effect=harness_statuses)

    p = mock.Mock()
    p.poll = mock.Mock(
        side_effect=itertools.cycle([True, True, True, True, False])
    )

    mock_thread_get_fifo_names = mock.Mock(
        return_value=('fifo_fn.in', 'fifo_fn.out')
    )
    log_fp_mock = mock.Mock()
    in_fp_mock = mocked_open('fifo_fn.out')
    out_fp_mock = mock.Mock()
    mock_thread_open_files = mock.Mock(
        return_value=(out_fp_mock, in_fp_mock, log_fp_mock)
    )
    mock_thread_close_files = mock.Mock()
    mock_thread_update_instance_info = mock.Mock()

    with mock.patch('time.time', side_effect=faux_timer.time), \
         mock.patch('builtins.open', new=mocked_open), \
         mock.patch('select.poll', return_value=p), \
         mock.patch('os.path.exists', return_value=True), \
         mock.patch('twisterlib.handlers.QEMUHandler._get_cpu_time',
                    mock_cputime), \
         mock.patch('twisterlib.handlers.QEMUHandler._thread_get_fifo_names',
                    mock_thread_get_fifo_names), \
         mock.patch('twisterlib.handlers.QEMUHandler._thread_open_files',
                    mock_thread_open_files), \
         mock.patch('twisterlib.handlers.QEMUHandler._thread_close_files',
                    mock_thread_close_files), \
         mock.patch('twisterlib.handlers.QEMUHandler.' \
                    '_thread_update_instance_info',
                    mock_thread_update_instance_info):
        QEMUHandler._thread(
            handler,
            handler.get_test_timeout(),
            handler.build_dir,
            handler.log,
            handler.fifo_fn,
            handler.pid_fn,
            harness,
            handler.ignore_unexpected_eof
        )

    print(mock_thread_update_instance_info.call_args_list)

    mock_thread_update_instance_info.assert_called_once_with(
        handler,
        mock.ANY,
        expected_status,
        mock.ANY
    )

    log_fp_mock.write.assert_has_calls(expected_log_calls)


TESTDATA_26 = [
    (True, False, TwisterStatus.NONE, True,
     ['No timeout, return code from QEMU (1): 1',
      'return code from QEMU (1): 1']),
    (False, True, TwisterStatus.PASS, True, ['return code from QEMU (1): 0']),
    (False, True, TwisterStatus.FAIL, False, ['return code from QEMU (None): 1']),
]

@pytest.mark.parametrize(
    'isatty, do_timeout, harness_status, exists_pid_fn, expected_logs',
    TESTDATA_26,
    ids=['no timeout, isatty', 'timeout passed', 'timeout, no pid_fn']
)
def test_qemuhandler_handle(
    mocked_instance,
    caplog,
    tmp_path,
    isatty,
    do_timeout,
    harness_status,
    exists_pid_fn,
    expected_logs
):
    def mock_wait(*args, **kwargs):
        if do_timeout:
            raise TimeoutExpired('dummy cmd', 'dummyamount')

    mock_process = mock.Mock(pid=0, returncode=1)
    mock_process.communicate = mock.Mock(
        return_value=(mock.Mock(), mock.Mock())
    )
    mock_process.wait = mock.Mock(side_effect=mock_wait)

    handler = QEMUHandler(mocked_instance, 'build', mock.Mock(timeout_multiplier=1))

    def mock_path_exists(name, *args, **kwargs):
        return exists_pid_fn

    def mock_popen(command, stdout=None, stdin=None, stderr=None, cwd=None):
        return mock.Mock(
            __enter__=mock.Mock(return_value=mock_process),
            __exit__=mock.Mock(return_value=None),
            communicate=mock.Mock(return_value=(mock.Mock(), mock.Mock()))
        )

    def mock_thread(name=None, target=None, daemon=None, args=None):
        return mock.Mock()

    def mock_filenames(sysbuild_build_dir):
        handler.fifo_fn = os.path.join('dummy', 'qemu-fifo')
        handler.pid_fn = os.path.join(sysbuild_build_dir, 'qemu.pid')
        handler.log_fn = os.path.join('dummy', 'log')

    harness = mock.Mock(status=harness_status)
    handler_options_west_flash = []

    domain_build_dir = os.path.join('sysbuild', 'dummydir')
    command = ['generator_cmd', '-C', os.path.join('cmd', 'path'), 'run']

    handler.options = mock.Mock(
        timeout_multiplier=1,
        west_flash=handler_options_west_flash,
        west_runner=None
    )
    handler.run_custom_script = mock.Mock(return_value=None)
    handler.make_device_available = mock.Mock(return_value=None)
    handler._final_handle_actions = mock.Mock(return_value=None)
    handler._create_command = mock.Mock(return_value=command)
    handler._set_qemu_filenames = mock.Mock(side_effect=mock_filenames)
    handler.get_default_domain_build_dir = mock.Mock(return_value=domain_build_dir)
    handler.terminate = mock.Mock()

    unlink_mock = mock.Mock()

    with mock.patch('subprocess.Popen', side_effect=mock_popen), \
         mock.patch('builtins.open', mock.mock_open(read_data='1')), \
         mock.patch('threading.Thread', side_effect=mock_thread), \
         mock.patch('os.path.exists', side_effect=mock_path_exists), \
         mock.patch('os.unlink', unlink_mock), \
         mock.patch('sys.stdout.isatty', return_value=isatty):
        handler.handle(harness)

    assert all([expected_log in caplog.text for expected_log in expected_logs])


def test_qemuhandler_get_fifo(mocked_instance):
    handler = QEMUHandler(mocked_instance, 'build', mock.Mock(timeout_multiplier=1))
    handler.fifo_fn = 'fifo_fn'

    result = handler.get_fifo()

    assert result == 'fifo_fn'
