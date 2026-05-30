# Copyright (c) 2026 Hsiu-Chi Tsai
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging
from unittest.mock import patch

import pytest
from conftest import RC_KERNEL_BIN

from runners.esp32 import Esp32BinaryRunner

TEST_DEVICE = '/dev/ttyACM0'
TEST_IDF_PATH = '/test/esp-idf'


def require_esptool(program):
    assert program == 'esptool'


def parse_args(*extra):
    parser = argparse.ArgumentParser(allow_abbrev=False)
    Esp32BinaryRunner.add_parser(parser)
    return parser.parse_args(['--esp-idf-path', TEST_IDF_PATH, '--esp-device', TEST_DEVICE, *extra])


def after_value(cmd):
    '''Value passed to esptool's --after, or None if absent.'''
    return cmd[cmd.index('--after') + 1] if '--after' in cmd else None


@pytest.mark.parametrize(
    'reset_type, expected',
    [
        (None, 'hard-reset'),
        ('hard-reset', 'hard-reset'),
        ('watchdog-reset', 'watchdog-reset'),
    ],
)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_esptool)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_flash_after(cc, req, runner_config, reset_type, expected):
    '''--reset-type selects esptool's post-flash --after value.'''
    extra = ['--reset-type', reset_type] if reset_type else []
    runner = Esp32BinaryRunner.create(runner_config, parse_args(*extra))
    runner.run('flash')

    cmd = cc.call_args_list[-1].args[0]
    assert after_value(cmd) == expected
    assert RC_KERNEL_BIN in cmd


@pytest.mark.parametrize('reset_type', ['hard-reset', 'watchdog-reset'])
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_esptool)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_no_reset_ignores_reset_type(cc, req, runner_config, caplog, reset_type):
    '''--no-reset drops --after and warns for any explicit --reset-type.'''
    args = parse_args('--no-reset', '--reset-type', reset_type)
    runner = Esp32BinaryRunner.create(runner_config, args)
    with caplog.at_level(logging.WARNING, logger='runners.esp32'):
        runner.run('flash')

    cmd = cc.call_args_list[-1].args[0]
    assert '--after' not in cmd
    assert any('reset is disabled' in r.message for r in caplog.records)
