# Copyright (c) 2026 The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from unittest.mock import patch

import pytest
from conftest import RC_KERNEL_BIN

from runners.bflb_mcu_tool import DEFAULT_EXECUTABLE, DEFAULT_SPEED, BlFlashCommandBinaryRunner

TEST_PORT = '/dev/ttyACM0'


def parse_args(*extra):
    parser = argparse.ArgumentParser(allow_abbrev=False)
    BlFlashCommandBinaryRunner.add_parser(parser)
    return parser.parse_args(['--dev-id', TEST_PORT, *extra])


@pytest.mark.parametrize(
    'extra, expected',
    [
        ([], DEFAULT_SPEED),
        (['--baud-rate', '115200'], '115200'),
        (['--baudrate', '460800'], '460800'),
        (['-b', '230400'], '230400'),
    ],
)
@patch('runners.core.ZephyrBinaryRunner.ensure_output')
@patch('runners.core.ZephyrBinaryRunner.require')
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_baud_rate(cc, req, eo, runner_config, extra, expected):
    runner = BlFlashCommandBinaryRunner.create(runner_config, parse_args(*extra))
    runner.run('flash')

    cc.assert_called_once_with(
        [
            DEFAULT_EXECUTABLE,
            '--port',
            TEST_PORT,
            '--baudrate',
            expected,
            '--chipname',
            'bl602',
            '--firmware',
            RC_KERNEL_BIN,
        ]
    )
