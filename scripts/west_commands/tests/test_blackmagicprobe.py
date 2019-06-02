# Copyright (c) 2018 Foundries.io
# Copyright (c) 2019 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from unittest.mock import patch, call

import pytest

from runners.blackmagicprobe import BlackMagicProbeRunner
from conftest import RC_KERNEL_ELF, RC_GDB

TEST_GDB_SERIAL = 'test-gdb-serial'

# Expected subprocesses to be run for each command. Using the
# runner_config fixture (and always specifying gdb-serial) means we
# don't get 100% coverage, but it's a starting out point.
EXPECTED_COMMANDS = {
    'attach':
    ([RC_GDB,
      '-ex', "set confirm off",
      '-ex', "target extended-remote {}".format(TEST_GDB_SERIAL),
      '-ex', "monitor swdp_scan",
      '-ex', "attach 1",
      '-ex', "file {}".format(RC_KERNEL_ELF)],),
    'debug':
    ([RC_GDB,
      '-ex', "set confirm off",
      '-ex', "target extended-remote {}".format(TEST_GDB_SERIAL),
      '-ex', "monitor swdp_scan",
      '-ex', "attach 1",
      '-ex', "file {}".format(RC_KERNEL_ELF),
      '-ex', "load {}".format(RC_KERNEL_ELF)],),
    'flash':
    ([RC_GDB,
      '-ex', "set confirm off",
      '-ex', "target extended-remote {}".format(TEST_GDB_SERIAL),
      '-ex', "monitor swdp_scan",
      '-ex', "attach 1",
      '-ex', "load {}".format(RC_KERNEL_ELF),
      '-ex', "kill",
      '-ex', "quit",
      '-silent'],),
}

def require_patch(program):
    assert program == RC_GDB

@pytest.mark.parametrize('command', EXPECTED_COMMANDS)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_blackmagicprobe_init(cc, req, command, runner_config):
    '''Test commands using a runner created by constructor.'''
    runner = BlackMagicProbeRunner(runner_config, TEST_GDB_SERIAL)
    runner.run(command)
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS[command]]

@pytest.mark.parametrize('command', EXPECTED_COMMANDS)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_blackmagicprobe_create(cc, req, command, runner_config):
    '''Test commands using a runner created from command line parameters.'''
    args = ['--gdb-serial', TEST_GDB_SERIAL]
    parser = argparse.ArgumentParser()
    BlackMagicProbeRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = BlackMagicProbeRunner.create(runner_config, arg_namespace)
    runner.run(command)
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS[command]]
