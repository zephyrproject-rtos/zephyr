# Copyright (c) 2018 Foundries.io
# Copyright (c) 2019 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from unittest.mock import patch, call

from runners.bossac import BossacBinaryRunner
from conftest import RC_KERNEL_BIN

TEST_BOSSAC_PORT = 'test-bossac-serial'
TEST_OFFSET = 1234
EXPECTED_COMMANDS = [
    ['stty', '-F', TEST_BOSSAC_PORT, 'raw', 'ispeed', '1200', 'ospeed', '1200',
     'cs8', '-cstopb', 'ignpar', 'eol', '255', 'eof', '255'],
    ['bossac', '-p', TEST_BOSSAC_PORT, '-R', '-e', '-w', '-v',
     '-o', str(TEST_OFFSET), '-b', RC_KERNEL_BIN],
]

def require_patch(program):
    assert program in ['bossac', 'stty']

@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_init(cc, req, runner_config):
    '''Test commands using a runner created by constructor.'''
    runner = BossacBinaryRunner(runner_config, port=TEST_BOSSAC_PORT,
                                offset=TEST_OFFSET)
    runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS]

@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_create(cc, req, runner_config):
    '''Test commands using a runner created from command line parameters.'''
    args = ['--bossac-port', str(TEST_BOSSAC_PORT),
            '--offset', str(TEST_OFFSET)]
    parser = argparse.ArgumentParser()
    BossacBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = BossacBinaryRunner.create(runner_config, arg_namespace)
    runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS]
