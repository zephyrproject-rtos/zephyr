# Copyright (c) 2026 The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

import pytest

from runners.amebaflash import AmebaFlashBinaryRunner

HAL_DEFAULT_SPEED = '1500000'
BOARD_ARGS = ['--device', 'amebadplus', '--image-dir', '/test/images', '--port', '/dev/ttyUSB0']


def fake_setup_parser(parser):
    '''Mirror the options of the HAL flash script relevant to these tests.'''
    parser.add_argument('--device', required=True)
    parser.add_argument('--image-dir', required=True)
    parser.add_argument('--port', action='append', required=True)
    parser.add_argument('--baudrate', default=HAL_DEFAULT_SPEED, help='Serial baudrate')


@pytest.fixture
def flash_mod():
    mod = SimpleNamespace(setup_parser=fake_setup_parser, main=MagicMock())
    with patch('runners.amebaflash._get_flash_module', return_value=mod):
        yield mod


def run(runner_config, flash_mod, argv):
    parser = argparse.ArgumentParser(allow_abbrev=False)
    AmebaFlashBinaryRunner.add_parser(parser)
    args = parser.parse_args(BOARD_ARGS + argv)
    AmebaFlashBinaryRunner.create(runner_config, args).run('flash')
    flash_mod.main.assert_called_once()
    return flash_mod.main.call_args.args[0].baudrate


@pytest.mark.parametrize(
    'argv, expected',
    [
        ([], HAL_DEFAULT_SPEED),
        (['--baud-rate', '115200'], '115200'),
        (['--baudrate', '230400'], '230400'),
        # Board --baud-rate from runners.yaml comes first, user options last
        (['--baud-rate=115200', '--baudrate', '460800'], '460800'),
        (['--baud-rate=115200', '--baud-rate', '921600'], '921600'),
    ],
)
def test_baud_rate(runner_config, flash_mod, argv, expected):
    assert run(runner_config, flash_mod, argv) == expected


def test_baudrate_deprecated(runner_config, flash_mod, caplog):
    run(runner_config, flash_mod, ['--baudrate', '230400'])
    assert 'Argument --baudrate is deprecated' in caplog.text
