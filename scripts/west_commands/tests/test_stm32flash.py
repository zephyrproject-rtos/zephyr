# Copyright (c) 2019 Thomas Kupper
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import platform
from unittest.mock import patch, call

import pytest

from runners.stm32flash import Stm32flashBinaryRunner
from conftest import RC_KERNEL_BIN

TEST_CMD = 'stm32flash'

TEST_DEVICE = '/dev/ttyUSB0'
if platform.system() == 'Darwin':
    TEST_DEVICE = '/dev/tty.SLAB_USBtoUART'
TEST_BAUD = '115200'
TEST_FORCE_BINARY = False
TEST_ADDR = '0x08000000'
TEST_BIN_SIZE = '4095'
TEST_EXEC_ADDR = '0'
TEST_SERIAL_MODE = '8e1'
TEST_RESET = False
TEST_VERIFY = False

# Expected subprocesses to be run for each action. Using the
# runner_config fixture (and always specifying all necessary
# parameters) means we don't get 100% coverage, but it's a
# starting out point.
EXPECTED_COMMANDS = {
    'info':
    ([TEST_CMD,
      '-b', TEST_BAUD,
      '-m', TEST_SERIAL_MODE,
      TEST_DEVICE],),
    'erase':
    ([TEST_CMD,
      '-b', TEST_BAUD,
      '-m', TEST_SERIAL_MODE,
      '-S', TEST_ADDR + ":" + str((int(TEST_BIN_SIZE)  >> 12) + 1 << 12),
      '-o', TEST_DEVICE],),
    'start':
    ([TEST_CMD,
      '-b', TEST_BAUD,
      '-m', TEST_SERIAL_MODE,
      '-g', TEST_EXEC_ADDR, TEST_DEVICE],),

    'write':
    ([TEST_CMD,
      '-b', TEST_BAUD,
      '-m', TEST_SERIAL_MODE,
      '-S', TEST_ADDR + ":" + TEST_BIN_SIZE,
      '-w', RC_KERNEL_BIN,
      TEST_DEVICE],),
}

def require_patch(program):
    assert program == TEST_CMD

def os_path_getsize_patch(filename):
    if filename == RC_KERNEL_BIN:
        return TEST_BIN_SIZE
    return os.path.isfile(filename)

def os_path_isfile_patch(filename):
    if filename == RC_KERNEL_BIN:
        return True
    return os.path.isfile(filename)

@pytest.mark.parametrize('action', EXPECTED_COMMANDS)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_stm32flash_init(cc, req, action, runner_config):
    '''Test actions using a runner created by constructor.'''
    test_exec_addr = TEST_EXEC_ADDR
    if action == 'write':
        test_exec_addr = None

    runner = Stm32flashBinaryRunner(runner_config, device=TEST_DEVICE,
                 action=action, baud=TEST_BAUD, force_binary=TEST_FORCE_BINARY,
                 start_addr=TEST_ADDR, exec_addr=test_exec_addr,
                 serial_mode=TEST_SERIAL_MODE, reset=TEST_RESET, verify=TEST_VERIFY)

    with patch('os.path.getsize', side_effect=os_path_getsize_patch):
        with patch('os.path.isfile', side_effect=os_path_isfile_patch):
            runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS[action]]

@pytest.mark.parametrize('action', EXPECTED_COMMANDS)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_stm32flash_create(cc, req, action, runner_config):
    '''Test actions using a runner created from action line parameters.'''
    if action == 'start':
        args = ['--action', action, '--baud-rate', TEST_BAUD, '--start-addr', TEST_ADDR,
                '--execution-addr', TEST_EXEC_ADDR]
    else:
        args = ['--action', action, '--baud-rate', TEST_BAUD, '--start-addr', TEST_ADDR]

    parser = argparse.ArgumentParser()
    Stm32flashBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = Stm32flashBinaryRunner.create(runner_config, arg_namespace)
    with patch('os.path.getsize', side_effect=os_path_getsize_patch):
        with patch('os.path.isfile', side_effect=os_path_isfile_patch):
            runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS[action]]
