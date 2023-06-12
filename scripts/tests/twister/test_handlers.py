#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
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

from serial import SerialException

import twisterlib.harness

from twisterlib.error import TwisterException
from twisterlib.handlers import (
    Handler,
    BinaryHandler,
    DeviceHandler,
    QEMUHandler
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

    instance.status = None
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


def test_handler_final_handle_actions(mocked_instance):
    instance = mocked_instance
    instance.testcases = [mock.Mock()]

    handler = Handler(mocked_instance)
    handler.suite_name_check = True

    harness = twisterlib.harness.Test()
    harness.state = mock.Mock()
    harness.detected_suite_names = mock.Mock()
    harness.matched_run_id = False
    harness.run_id_exists = True

    handler_time = mock.Mock()

    handler._final_handle_actions(harness, handler_time)

    assert handler.instance.status == 'failed'
    assert handler.instance.execution_time == handler_time
    assert handler.instance.reason == 'RunID mismatch'
    assert all(testcase.status == 'failed' for \
     testcase in handler.instance.testcases)

    handler.instance.reason = 'This reason shan\'t be changed.'
    handler._final_handle_actions(harness, handler_time)

    assert handler.instance.reason == 'This reason shan\'t be changed.'


TESTDATA_1 = [
    (['dummy_testsuite_name'], False),
    ([], True),
    (['another_dummy_name', 'yet_another_dummy_name'], True),
]


@pytest.mark.parametrize(
    'detected_suite_names, should_be_called',
    TESTDATA_1,
    ids=['detected one expected', 'detected none', 'detected two unexpected']
)
def test_handler_verify_ztest_suite_name(
    mocked_instance,
    detected_suite_names,
    should_be_called
):
    instance = mocked_instance
    type(instance.testsuite).ztest_suite_names = ['dummy_testsuite_name']

    harness_state = 'passed'

    handler_time = mock.Mock()

    with mock.patch.object(Handler, '_missing_suite_name') as _missing_mocked:
        handler = Handler(instance)
        handler._verify_ztest_suite_name(
            harness_state,
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

    handler = Handler(mocked_instance)
    handler.suite_name_check = True

    expected_suite_names = ['dummy_testsuite_name']

    handler_time = mock.Mock()

    handler._missing_suite_name(expected_suite_names, handler_time)

    assert handler.instance.status == 'failed'
    assert handler.instance.execution_time == handler_time
    assert handler.instance.reason == 'Testsuite mismatch'
    assert all(
        testcase.status == 'failed' for testcase in handler.instance.testcases
    )


def test_handler_record(mocked_instance):
    instance = mocked_instance
    instance.testcases = [mock.Mock()]

    handler = Handler(instance)
    handler.suite_name_check = True

    harness = twisterlib.harness.Test()
    harness.recording = ['dummy recording']
    type(harness).fieldnames = mock.PropertyMock(return_value=[])

    mock_writerow = mock.Mock()
    mock_writer = mock.Mock(writerow=mock_writerow)

    with mock.patch(
        'builtins.open',
        mock.mock_open(read_data='')
    ) as mock_file, \
         mock.patch(
        'csv.writer',
        mock.Mock(return_value=mock_writer)
    ) as mock_writer_constructor:
        handler.record(harness)

    mock_file.assert_called_with(
        os.path.join(instance.build_dir, 'recording.csv'),
        'at'
    )

    mock_writer_constructor.assert_called_with(
        mock_file(),
        harness.fieldnames,
        lineterminator=os.linesep
    )

    mock_writerow.assert_has_calls(
        [mock.call(harness.fieldnames)] + \
        [mock.call(recording) for recording in harness.recording]
    )


def test_handler_terminate(mocked_instance):
    def mock_kill_function(pid, sig):
        if pid < 0:
            raise ProcessLookupError

    instance = mocked_instance

    handler = Handler(instance)

    mock_process = mock.Mock()
    mock_child1 = mock.Mock(pid=1)
    mock_child2 = mock.Mock(pid=2)
    mock_process.children = mock.Mock(return_value=[mock_child1, mock_child2])

    mock_proc = mock.Mock(pid=0)
    mock_proc.terminate = mock.Mock(return_value=None)
    mock_proc.kill = mock.Mock(return_value=None)

    with mock.patch('psutil.Process', mock.Mock(return_value=mock_process)), \
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

    handler = BinaryHandler(instance, 'build')
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


TESTDATA_2 = [
    (
        [b'This\\r\\n', b'is\r', b'a short', b'file.'],
        mock.Mock(state=False, capture_coverage=False),
        [
            mock.call('This\\r\\n'),
            mock.call('is\r'),
            mock.call('a short'),
            mock.call('file.')
        ],
        [
            mock.call('This'),
            mock.call('is'),
            mock.call('a short'),
            mock.call('file.')
        ],
        None
    ),
    (
        [b'Too much.'] * 120,  # Should be more than the timeout
        mock.Mock(state=False, capture_coverage=False),
        None,
        None,
        True
    ),
    (
        [b'Too much.'] * 120,  # Should be more than the timeout
        mock.Mock(state=True, capture_coverage=False),
        None,
        None,
        True
    ),
    (
        [b'Too much.'] * 120,  # Should be more than the timeout
        mock.Mock(state=True, capture_coverage=True),
        None,
        None,
        False
    ),
]


@pytest.mark.parametrize(
    'proc_stdout, harness, expected_handler_calls,'
    ' expected_harness_calls, should_be_less',
    TESTDATA_2,
    ids=[
        'no timeout',
        'timeout',
        'timeout with harness state',
        'timeout with capture_coverage'
    ]
)
def test_binaryhandler_output_handler(
    mocked_instance,
    faux_timer,
    proc_stdout,
    harness,
    expected_handler_calls,
    expected_harness_calls,
    should_be_less
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

    handler = BinaryHandler(mocked_instance, 'build')

    proc = MockProc(1, proc_stdout)
    proc.wait = mock.Mock(return_value = None)

    with mock.patch(
        'builtins.open',
        mock.mock_open(read_data='')
    ) as mock_file, \
         mock.patch('time.time', mock.Mock(side_effect=faux_timer.time)):
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


TESTDATA_3 = [
    (
        {
            'call_make_run': False,
            'call_west_flash': False,
            'options': mock.Mock(
                enable_valgrind=True,
                enable_asan=True,
                enable_lsan=True,
                enable_ubsan=True,
                coverage=True
            ),
        },
        mock.Mock(is_robot_test=True),
        False,
        1,
        'success',
        ['generator_cmd', 'run_renode_test', 'valgrind']
    ),
    (
        {
            'call_make_run': True,
            'call_west_flash': False,
            'seed': 0,
            'options': mock.Mock(
                enable_valgrind=True,
                enable_asan=True,
                enable_lsan=False,
                enable_ubsan=True,
                coverage=False
            ),
        },
        mock.Mock(is_robot_test=False),
        False,
        2,
        'success',
        ['generator_cmd', 'run', '--seed=0']
    ),
    (
        {
            'call_make_run': False,
            'call_west_flash': True,
            'extra_test_args': ['extra_arg'],
            'options': mock.Mock(
                enable_valgrind=True,
                enable_asan=False,
                enable_lsan=False,
                enable_ubsan=False,
                coverage=True
            ),
        },
        mock.Mock(is_robot_test=False),
        False,
        1,
        'failed',
        ['west', 'flash', 'extra_arg']
    ),
    (
        {
            'call_make_run': False,
            'call_west_flash': False,
            'extra_test_args': None,
            'options': mock.Mock(
                enable_valgrind=False,
                enable_asan=False,
                enable_lsan=False,
                enable_ubsan=True,
                coverage=False
            ),
        },
        mock.Mock(is_robot_test=False),
        True,
        0,
        'failed',
        ['binary']
    ),
    (
        {
            'call_make_run': False,
            'call_west_flash': False,
            'extra_test_args': None,
            'options': mock.Mock(
                enable_valgrind=False,
                enable_asan=False,
                enable_lsan=False,
                enable_ubsan=True,
                coverage=False
            ),
        },
        mock.Mock(is_robot_test=False),
        True,
        0,
        'passed',
        ['binary']
    ),
    (
        {
            'call_make_run': False,
            'call_west_flash': False,
            'extra_test_args': None,
            'options': mock.Mock(
                enable_valgrind=False,
                enable_asan=False,
                enable_lsan=False,
                enable_ubsan=True,
                coverage=False
            ),
        },
        mock.Mock(is_robot_test=False),
        True,
        0,
        None,
        ['binary']
    )
]

@pytest.mark.parametrize(
    'handler_attrs, harness, isatty, return_code, harness_state, expected',
    TESTDATA_3,
    ids=[
        'robot_test, valgrind, asan, lsan, ubsan, coverage,' \
        ' harness_state=success, return_code=1',
        'make_run, valgrind, asan, ubsan, seed,' \
        ' harness_state=success, return_code=2',
        'west_flash, valgrind, coverage, extra_args,' \
        ' harness_state=failed, return_code=1',
        'binary, ubsan, isatty, harness_state=failed',
        'binary, ubsan, isatty, harness_state=passed',
        'binary, ubsan, isatty, harness_state=None'
    ]
)
def test_binaryhandler_handle(
    mocked_instance,
    handler_attrs,
    harness,
    isatty,
    return_code,
    harness_state,
    expected
):
    def assert_run_robot_test(command, handler):
        assert all(w in command for w in expected)

    def assert_popen(command, *args, **kwargs,):
        assert all(w in command for w in expected)
        return mock.Mock(
          __enter__=mock.Mock(
              return_value=mock.Mock(pid=0, returncode=return_code)
          ),
          __exit__=mock.Mock(return_value=None)
        )

    def mock_isatty():
        return isatty

    def assert_calls(command, *args, **kwargs):
        if handler_attrs['options'].coverage and 'gcov' in command:
            return
        elif mock_isatty() and 'stty' in command:
            return
        assert False

    def mock_thread(target=None, args=None, daemon=None):
        harness.state = harness_state
        return mock.Mock()

    handler = BinaryHandler(mocked_instance, 'build')
    handler.generator_cmd = 'generator_cmd'
    handler.binary = 'binary'
    handler.extra_test_args = handler_attrs.get(
        'extra_test_args',
        handler.extra_test_args
    )
    handler.seed = handler_attrs.get('seed', handler.seed)
    handler.call_make_run = handler_attrs.get(
        'call_make_run',
        handler.call_make_run
    )
    handler.call_west_flash = handler_attrs.get(
        'call_west_flash',
        handler.call_west_flash
    )
    handler.options = handler_attrs.get('options', handler.options)

    mock_popen = mock.Mock(side_effect=assert_popen)
    mock_call = mock.Mock(side_effect=assert_calls)

    with mock.patch('threading.Thread', mock.Mock(side_effect=mock_thread)), \
         mock.patch.object(
        BinaryHandler,
        '_final_handle_actions',
        lambda s, h, ht: None
    ), \
         mock.patch.object(BinaryHandler, 'terminate', lambda s, p: None), \
         mock.patch.object(
        twisterlib.harness.Robot,
        'run_robot_test',
        assert_run_robot_test
    ), \
         mock.patch('subprocess.Popen', mock_popen), \
         mock.patch('subprocess.call', mock_call), \
         mock.patch('sys.stdout.isatty', mock.Mock(side_effect=mock_isatty)):
        handler.handle(harness)

    if handler.returncode == 0 and harness.state and harness.state != 'failed':
        assert handler.instance.status != 'failed'
    else:
        assert handler.instance.status == 'failed'

    assert harness.run_robot_test.called or mock_popen.called

TESTDATA_4 = [
    (3, 2, 0, 0, 3, -1, True, False, False, 1),
    (4, 1, 0, 0, -1, -1, False, True, False, 0),
    (5, 0, 1, 2, -1, 4, False, False, True, 3)
]

@pytest.mark.parametrize(
    'success_count, in_waiting_count, oserror_count, readline_error_count,'
    ' haltless_count, stateless_count, end_by_halt, end_by_close,'
    ' end_by_state, expected_line_count',
    TESTDATA_4,
    ids=[
      'halt event',
      'serial closes',
      'harness state with errors'
    ]
)
def test_devicehandler_monitor_serial(
    mocked_instance,
    success_count,
    in_waiting_count,
    oserror_count,
    readline_error_count,
    haltless_count,
    stateless_count,
    end_by_halt,
    end_by_close,
    end_by_state,
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

    state_iter = [False] * stateless_count + [True] \
        if end_by_state else iter(lambda: False, True)

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
    type(harness).state=mock.PropertyMock(side_effect=state_iter)

    handler = DeviceHandler(mocked_instance, 'build')
    handler.options = mock.Mock(coverage=not end_by_state)

    with mock.patch('builtins.open', mock.mock_open(read_data='')):
        handler.monitor_serial(ser, halt_event, harness)

    if not end_by_close:
        ser.close.assert_called_once()

    harness.handle.assert_has_calls(
        [mock.call(f'line no {idx}') for idx in range(expected_line_count)]
    )


TESTDATA_5 = [
    (
        'dummy_platform',
        'dummy fixture',
        [
            mock.Mock(
                fixtures=[],
                platform='dummy_platform',
                available=1,
                counter=0
            ),
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='another_platform',
                available=1,
                counter=0
            ),
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='dummy_platform',
                serial_pty=None,
                serial=None,
                available=1,
                counter=0
            ),
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='dummy_platform',
                serial_pty=mock.Mock(),
                available=1,
                counter=0
            )
        ],
        3
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
                available=0
            ),
            mock.Mock(
                fixtures=['another fixture'],
                platform='dummy_platform',
                serial_pty=mock.Mock(),
                available=0
            ),
            mock.Mock(
                fixtures=['dummy fixture'],
                platform='dummy_platform',
                serial=mock.Mock(),
                available=0
            ),
            mock.Mock(
                fixtures=['another fixture'],
                platform='dummy_platform',
                serial=mock.Mock(),
                available=0
            )
        ],
        None
    )
]


@pytest.mark.parametrize(
    'platform_name, fixture, duts, expected',
    TESTDATA_5,
    ids=['one good dut', 'exception - no duts', 'no available duts']
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

    handler = DeviceHandler(mocked_instance, 'build')
    handler.duts = duts

    if isinstance(expected, int):
        device = handler.device_is_available(mocked_instance)

        assert device == duts[expected]
        assert device.available == 0
        assert device.counter == 1
    elif expected is None:
        device = handler.device_is_available(mocked_instance)

        assert device is None
    elif isinstance(expected, type):
        with pytest.raises(expected):
            device = handler.device_is_available(mocked_instance)
    else:
        assert False


def test_devicehandler_make_device_available(mocked_instance):
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

    handler = DeviceHandler(mocked_instance, 'build')
    handler.duts = duts

    handler.make_device_available(serial)

    assert len([None for d in handler.duts if d.available == 1]) == 2
    assert handler.duts[2].available == 0


TESTDATA_6 = [
    (mock.Mock(pid=0, returncode=0), False),
    (mock.Mock(pid=0, returncode=1), False),
    (mock.Mock(pid=0, returncode=1), True)
]


@pytest.mark.parametrize(
    'mock_process, raise_timeout',
    TESTDATA_6,
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

    with mock.patch('subprocess.Popen', mock.Mock(side_effect=assert_popen)):
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


TESTDATA_7 = [
    (0, False),
    (4, False),
    (0, True)
]


@pytest.mark.parametrize(
    'num_of_failures, raise_exception',
    TESTDATA_7,
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

    handler = DeviceHandler(mocked_instance, 'build')
    handler.no = num_of_failures

    with mock.patch.object(
        DeviceHandler,
        'device_is_available',
        mock_availability
    ):
        hardware = handler.get_hardware()

    if raise_exception:
        assert 'dummy message' in caplog.text.lower()
        assert mocked_instance.status == 'failed'
        assert mocked_instance.reason == 'dummy message'
    else:
        assert hardware == expected_hardware


TESTDATA_8 = [
    ('', '', None, None, False, ['generator_cmd']),
    (
        'pyocd',
        'product',
        '--dummy-option-1,-do2',
        [],
        False,
        [
            '--dummy-option-1',
            '-do2',
            '--runner',
            'pyocd',
            '--dev-id',
            'board_id'
        ]
    ),
    (
        'openocd',
        'STM32 STLink',
        [],
        ['--dummy-option3'],
        False,
        [
            '--dummy-option3',
            '--runner',
            'openocd',
            '--cmd-pre-init',
            'hla_serial board_id'
        ]
    ),
    (
        'openocd',
        'STLINK-V3',
        [],
        [],
        True,
        ['--runner', 'openocd', '--cmd-pre-init', 'hla_serial board_id']
    ),
    (
        'openocd',
        'EDBG CMSIS-DAP',
        [],
        [],
        False,
        ['--runner', 'openocd', '--cmd-pre-init', 'cmsis_dap_serial board_id']
    ),
    (
        'jlink',
        'product',
        [],
        [],
        False,
        ['--runner', 'jlink', '--tool-opt=-SelectEmuBySN  board_id']
    ),
    (
        'stm32cubeprogrammer',
        'product',
        [],
        [],
        False,
        ['--runner', 'stm32cubeprogrammer', '--tool-opt=sn=board_id']
    ),
]


@pytest.mark.parametrize(
    'runner, product, west_flash_options,'
    ' runner_options, serial_not_pty, expected',
    TESTDATA_8,
    ids=[
        'generator_cmd',
        'pyocd',
        'openocd, stm32 stlink',
        'openocd, stlink-v3',
        'openocd, edbg cmsis-dap',
        'jlink',
        'stm32cubeprogrammer'
    ]
)
def test_devicehandler_handle(
    mocked_instance,
    runner,
    product,
    west_flash_options,
    runner_options,
    serial_not_pty,
    expected
):
    mock_process = mock.Mock(pid=0, returncode=0)
    mock_process.communicate = mock.Mock(
        return_value=(mock.Mock(), mock.Mock())
    )

    def mock_popen(command, stdout=None, stdin=None, stderr=None):
        if 'flash' in command:
            assert all([c in command for c in expected])

        return mock.Mock(
            __enter__=mock.Mock(return_value=mock_process),
            __exit__=mock.Mock(return_value=None),
            communicate=mock.Mock(return_value=(mock.Mock(), mock.Mock()))
        )

    def mock_thread(target=None, daemon=None, args=None):
        return mock.Mock()

    hardware_runner = runner
    hardware_product = product
    hardware_serial_pty = 'cmd1,| cmd2' if not serial_not_pty else None
    hardware_serial = mock.Mock()
    hardware_runner_params = runner_options
    hardware_flash_timeout = 1
    hardware = mock.Mock(
        product=hardware_product,
        runner=hardware_runner,
        serial=hardware_serial,
        serial_pty=hardware_serial_pty,
        runner_params=hardware_runner_params,
        flash_timeout=hardware_flash_timeout,
        probe_id='board_id'
    )
    harness = mock.Mock()
    handler_options_west_flash = west_flash_options

    handler = DeviceHandler(mocked_instance, 'build')
    handler.options = mock.Mock(
        west_flash=handler_options_west_flash,
        west_runner=None
    )
    handler.run_custom_script = mock.Mock(return_value=None)
    handler.make_device_available = mock.Mock(return_value=None)
    handler._final_handle_actions = mock.Mock(return_value=None)
    handler.generator_cmd = 'generator_cmd'

    with mock.patch.object(
        DeviceHandler,
        'get_hardware',
        mock.Mock(return_value=hardware)
    ), \
         mock.patch(
        'pty.openpty',
        mock.Mock(return_value=(mock.Mock(), mock.Mock()))
    ), \
         mock.patch('subprocess.Popen', mock.Mock(side_effect=mock_popen)), \
         mock.patch('os.ttyname', mock.Mock(return_value=mock.Mock())), \
         mock.patch('serial.Serial', mock.Mock(return_value=mock.Mock())), \
         mock.patch('builtins.open', mock.mock_open(read_data='')), \
         mock.patch('threading.Event', mock.Mock(return_value=mock.Mock())), \
         mock.patch('threading.Thread', mock.Mock(side_effect=mock_thread)):
        handler.handle(harness)

    assert True


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


TESTDATA_9 = [
    (
        ('1\n' * 60).encode('utf-8'),
        60,
        1,
        [None] * 60 + ['success'] * 6,
        1000,
        'failed',
        'Timeout'
    ),
    (
        ('1\n' * 60).encode('utf-8'),
        60,
        -1,
        [None] * 60 + ['success'] * 30,
        100,
        'failed',
        'Failed'
    ),
    (
        b'',
        60,
        1,
        ['success'] * 3,
        100,
        'failed',
        'unexpected eof'
    ),
    (
        b'\x81',
        60,
        1,
        ['success'] * 3,
        100,
        'failed',
        'unexpected byte'
    ),
    (
        '1\n2\n3\n4\n5\n'.encode('utf-8'),
        600,
        1,
        [None] * 3 + ['success'] * 7,
        100,
        'success',
        'Unknown'
    ),
]


@pytest.mark.parametrize(
    'content, timeout, pid, harness_states, cputime,'
    ' expected_instance_status, expected_instance_reason',
    TESTDATA_9,
    ids=[
        'timeout',
        'harness failed',
        'unexpected eof',
        'unexpected byte',
        'harness success'
    ]
)
def test_qemuhandler_thread(
    mocked_instance,
    faux_timer,
    content,
    timeout,
    pid,
    harness_states,
    cputime,
    expected_instance_status,
    expected_instance_reason
):
    def mock_cputime(pid):
        if pid > 0:
            return cputime
        else:
            raise ProcessLookupError()

    handler = QEMUHandler(mocked_instance, 'build')
    handler.ignore_unexpected_eof=False
    handler.results = {}
    handler.pid_fn = 'pid_fn'
    handler.fifo_fn = 'fifo_fn'
    type(mocked_instance.testsuite).timeout = mock.PropertyMock(
        return_value=timeout
    )

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

    harness = mock.Mock(capture_coverage=False)
    type(harness).state = mock.PropertyMock(side_effect=harness_states)

    p = mock.Mock()
    p.poll = mock.Mock(
        side_effect=itertools.cycle([True, True, True, True, False])
    )

    with mock.patch('time.time', mock.Mock(side_effect=faux_timer.time)), \
         mock.patch('builtins.open', new=mocked_open), \
         mock.patch('select.poll', mock.Mock(return_value=p)), \
         mock.patch('os.path.exists', mock.Mock(return_value=True)), \
         mock.patch('os.unlink', mock.Mock()), \
         mock.patch('os.mkfifo', mock.Mock()), \
         mock.patch('os.kill', mock.Mock()), \
         mock.patch.object(QEMUHandler, '_get_cpu_time', mock_cputime):

        QEMUHandler._thread(
            handler,
            handler.timeout,
            handler.build_dir,
            handler.log,
            handler.fifo_fn,
            handler.pid_fn,
            handler.results,
            harness,
            handler.ignore_unexpected_eof
        )

    assert handler.instance.status == expected_instance_status
    assert handler.instance.reason == expected_instance_reason


def test_qemuhandler_handle(mocked_instance, caplog, tmp_path):
    mock_process = mock.Mock(pid=0, returncode=0)
    mock_process.communicate = mock.Mock(
        return_value=(mock.Mock(), mock.Mock())
    )

    def mock_popen(command, stdout=None, stdin=None, stderr=None, cwd=None):
        return mock.Mock(
            __enter__=mock.Mock(return_value=mock_process),
            __exit__=mock.Mock(return_value=None),
            communicate=mock.Mock(return_value=(mock.Mock(), mock.Mock()))
        )

    def mock_thread(name=None, target=None, daemon=None, args=None):
        return mock.Mock()

    mocked_domain = mock.Mock()
    mocked_domain.get_default_domain = mock.Mock(return_value=mock.Mock())
    type(mocked_domain.get_default_domain()).build_dir = os.path.join(
        tmp_path,
        'domain_build_dir'
    )

    hardware_flash_timeout = 1
    hardware = mock.Mock(
        flash_timeout=hardware_flash_timeout,
        probe_id='board_id'
    )
    harness = mock.Mock()
    handler_options_west_flash = []

    handler = QEMUHandler(mocked_instance, 'build')
    handler.options = mock.Mock(
        west_flash=handler_options_west_flash,
        west_runner=None
    )
    handler.run_custom_script = mock.Mock(return_value=None)
    handler.make_device_available = mock.Mock(return_value=None)
    handler._final_handle_actions = mock.Mock(return_value=None)
    handler.generator_cmd = 'generator_cmd'

    with mock.patch.object(
        DeviceHandler,
        'get_hardware',
        mock.Mock(return_value=hardware)
    ), \
         mock.patch(
        'pty.openpty',
        mock.Mock(return_value=(mock.Mock(), mock.Mock()))
    ), \
         mock.patch('subprocess.Popen', mock.Mock(side_effect=mock_popen)), \
         mock.patch('os.ttyname', mock.Mock(return_value=mock.Mock())), \
         mock.patch('serial.Serial', mock.Mock(return_value=mock.Mock())), \
         mock.patch('builtins.open', mock.mock_open(read_data='')), \
         mock.patch('threading.Event', mock.Mock(return_value=mock.Mock())), \
         mock.patch('threading.Thread', mock.Mock(side_effect=mock_thread)), \
         mock.patch(
        'domains.Domains.from_file',
        mock.Mock(return_value=mocked_domain)
    ):
        handler.handle(harness)

    assert caplog.records[-1].message == 'return code from QEMU (None):' \
                                        f' {mock_process.returncode}'
    assert caplog.records[-1].levelname == 'DEBUG'
