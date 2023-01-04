# Copyright (c) 2021 Gerson Fernando Budke <nandojve@gmail.com>
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import platform
from unittest.mock import patch, call

import pytest

from runners.gd32isp import Gd32ispBinaryRunner
from conftest import RC_KERNEL_BIN

if platform.system() != 'Linux':
    pytest.skip("skipping Linux-only gd32isp tests", allow_module_level=True)

TEST_GD32ISP_CLI   = 'GD32_ISP_Console'
TEST_GD32ISP_CLI_T = 'GD32_ISP_CLI'
TEST_GD32ISP_DEV   = 'test-gd32test'
TEST_GD32ISP_PORT  = 'test-gd32isp-serial'
TEST_GD32ISP_SPEED = '2000000'
TEST_GD32ISP_ADDR  = '0x08765430'

EXPECTED_COMMANDS_DEFAULT = [
    [TEST_GD32ISP_CLI, '-c', '--pn', '/dev/ttyUSB0', '--br', '57600',
     '--sb', '1', '-i', TEST_GD32ISP_DEV, '-e', '--all', '-d',
     '--a', '0x08000000', '--fn', RC_KERNEL_BIN],
]

EXPECTED_COMMANDS = [
    [TEST_GD32ISP_CLI_T, '-c', '--pn', TEST_GD32ISP_PORT,
     '--br', TEST_GD32ISP_SPEED,
     '--sb', '1', '-i', TEST_GD32ISP_DEV, '-e', '--all', '-d',
     '--a', TEST_GD32ISP_ADDR, '--fn', RC_KERNEL_BIN],
]

def require_patch(program):
    assert program in [TEST_GD32ISP_CLI, TEST_GD32ISP_CLI_T]

os_path_isfile = os.path.isfile

def os_path_isfile_patch(filename):
    if filename == RC_KERNEL_BIN:
        return True
    return os_path_isfile(filename)


@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_gd32isp_init(cc, req, runner_config):
    runner = Gd32ispBinaryRunner(runner_config, TEST_GD32ISP_DEV)
    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS_DEFAULT]


@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_gd32isp_create(cc, req, runner_config):
    args = ['--device', TEST_GD32ISP_DEV,
            '--port', TEST_GD32ISP_PORT,
            '--speed', TEST_GD32ISP_SPEED,
            '--addr', TEST_GD32ISP_ADDR,
            '--isp', TEST_GD32ISP_CLI_T]
    parser = argparse.ArgumentParser(allow_abbrev=False)
    Gd32ispBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = Gd32ispBinaryRunner.create(runner_config, arg_namespace)
    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS]
