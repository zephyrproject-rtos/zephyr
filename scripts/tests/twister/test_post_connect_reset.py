#!/usr/bin/env python3
# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for post_connect_reset.py functions
"""

import subprocess
from unittest import mock

import pytest
from twisterlib import post_connect_reset
from twisterlib.post_connect_reset import PostConnectResetError


@pytest.mark.parametrize(
    'dut_config, platform_config, expected',
    [
        ({'method': 'west'}, {'method': 'serial', 'sequence': []}, {'method': 'west'}),
        (None, {'method': 'west'}, {'method': 'west'}),
        (None, None, None),
    ],
    ids=['hardware map wins', 'platform default', 'none'],
)
def test_resolve(dut_config, platform_config, expected):
    assert post_connect_reset.resolve(dut_config, platform_config) == expected


def test_apply_serial_lines_sets_only_listed_lines():
    ser = mock.Mock(dtr='unchanged', rts='unchanged')

    post_connect_reset.apply_serial_lines(ser, {'rts': True})

    assert ser.dtr == 'unchanged'
    assert ser.rts is True


def test_run_serial_sequence():
    ser = mock.Mock()
    lines = []
    type(ser).dtr = mock.PropertyMock(side_effect=lambda v: lines.append(('dtr', v)))
    type(ser).rts = mock.PropertyMock(side_effect=lambda v: lines.append(('rts', v)))
    config = {
        'method': 'serial',
        'sequence': [
            {'dtr': False, 'rts': True},
            {'delay_ms': 100},
            {'rts': False},
        ],
    }

    with mock.patch('time.sleep') as sleep_mock:
        post_connect_reset.run(config, ser=ser)

    assert lines == [('dtr', False), ('rts', True), ('rts', False)]
    sleep_mock.assert_called_once_with(0.1)


def test_run_serial_without_port():
    with pytest.raises(PostConnectResetError, match='open serial port'):
        post_connect_reset.run({'method': 'serial', 'sequence': []})


def test_run_west():
    with mock.patch('subprocess.run') as run_mock:
        post_connect_reset.run({'method': 'west'}, west_reset_command=['west', 'reset'])

    run_mock.assert_called_once_with(
        ['west', 'reset'],
        check=True,
        timeout=post_connect_reset.DEFAULT_TIMEOUT_S,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )


def test_run_west_without_command():
    with pytest.raises(PostConnectResetError, match='west reset command'):
        post_connect_reset.run({'method': 'west'})


def test_run_uses_configured_timeout():
    with mock.patch('subprocess.run') as run_mock:
        post_connect_reset.run({'method': 'west', 'timeout': 5}, west_reset_command=['west'])

    assert run_mock.call_args.kwargs['timeout'] == 5


@pytest.mark.parametrize(
    'exception, message',
    [
        (subprocess.CalledProcessError(2, ['west'], output=b'boom'), 'exited with 2: boom'),
        (subprocess.TimeoutExpired(['west'], 30), 'timed out after 30 s'),
        (OSError('no such file'), 'Cannot run west'),
    ],
    ids=['nonzero exit', 'timeout', 'not executable'],
)
def test_run_command_failure(exception, message):
    with (
        mock.patch('subprocess.run', side_effect=exception),
        pytest.raises(PostConnectResetError, match=message),
    ):
        post_connect_reset.run({'method': 'west'}, west_reset_command=['west', 'reset'])


def test_run_unknown_method():
    with pytest.raises(PostConnectResetError, match='Unknown method'):
        post_connect_reset.run({'method': 'script'})


SERIAL = {'method': 'serial', 'sequence': []}


@pytest.mark.parametrize(
    'kwargs, expected',
    [
        ({'dut_config': {'method': 'none'}, 'platform_config': SERIAL}, None),
        ({'dut_config': None, 'platform_config': {'method': 'none'}}, None),
        ({'dut_config': SERIAL, 'platform_config': None, 'serial_pty': True}, None),
        (
            {'dut_config': {'method': 'west'}, 'platform_config': None, 'serial_pty': True},
            {'method': 'west'},
        ),
    ],
    ids=[
        'hardware map none',
        'board none',
        'serial skipped with serial_pty',
        'west kept with serial_pty',
    ],
)
def test_resolve_rules(kwargs, expected):
    assert post_connect_reset.resolve(**kwargs) == expected
