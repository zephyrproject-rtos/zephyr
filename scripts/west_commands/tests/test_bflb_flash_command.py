# Copyright (c) 2026 The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from unittest.mock import MagicMock, patch

import pytest
from conftest import RC_BUILD_DIR, RC_KERNEL_BIN

from runners.bflb_flash_command import (
    DEFAULT_CHIP,
    DEFAULT_EXECUTABLE,
    DEFAULT_FLASH_ADDR,
    DEFAULT_SPEED,
    BlFlashCommandBinaryRunner,
)

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
        (['-b', '230400'], '230400'),
    ],
)
@patch('runners.bflb_flash_command.BuildConfiguration')
@patch('runners.core.ZephyrBinaryRunner.ensure_output')
@patch('runners.core.ZephyrBinaryRunner.require')
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_baud_rate(cc, req, eo, bc, runner_config, extra, expected):
    bc.return_value.edt.chosen_node.return_value.regs = [MagicMock(size=0x400000)]

    runner = BlFlashCommandBinaryRunner.create(runner_config, parse_args(*extra))
    runner.run('flash')

    bc.assert_called_once_with(RC_BUILD_DIR)
    cc.assert_called_once_with(
        [
            DEFAULT_EXECUTABLE,
            '--port',
            TEST_PORT,
            '--baudrate',
            expected,
            '--chipname',
            DEFAULT_CHIP,
            '--flash',
            '--write',
            '--start',
            hex(DEFAULT_FLASH_ADDR),
            '--file',
            RC_KERNEL_BIN,
        ]
    )
