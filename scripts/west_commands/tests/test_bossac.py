# Copyright (c) 2018 Foundries.io
# Copyright (c) 2019 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import platform
from unittest.mock import patch, call

import pytest

from runners.bossac import BossacBinaryRunner
from conftest import RC_KERNEL_BIN

if platform.system() != 'Linux':
    pytest.skip("skipping Linux-only bossac tests", allow_module_level=True)

TEST_BOSSAC_PORT = 'test-bossac-serial'
TEST_OFFSET = 1234
TEST_FLASH_ADDRESS = 5678

EXPECTED_COMMANDS = [
    ['stty', '-F', TEST_BOSSAC_PORT, 'raw', 'ispeed', '1200', 'ospeed', '1200',
     'cs8', '-cstopb', 'ignpar', 'eol', '255', 'eof', '255'],
    ['bossac', '-p', TEST_BOSSAC_PORT, '-R', '-e', '-w', '-v',
     '-b', RC_KERNEL_BIN],
]

EXPECTED_COMMANDS_WITH_OFFSET = [
    ['stty', '-F', TEST_BOSSAC_PORT, 'raw', 'ispeed', '1200', 'ospeed', '1200',
     'cs8', '-cstopb', 'ignpar', 'eol', '255', 'eof', '255'],
    ['bossac', '-p', TEST_BOSSAC_PORT, '-R', '-e', '-w', '-v',
     '-b', RC_KERNEL_BIN, '-o', str(TEST_OFFSET)],
]

EXPECTED_COMMANDS_WITH_FLASH_ADDRESS = [
    [
        'stty', '-F', TEST_BOSSAC_PORT, 'raw', 'ispeed', '1200', 'ospeed',
        '1200', 'cs8', '-cstopb', 'ignpar', 'eol', '255', 'eof', '255'
    ],
    [
        'bossac',
        '-p',
        TEST_BOSSAC_PORT,
        '-R',
        '-e',
        '-w',
        '-v',
        '-b',
        RC_KERNEL_BIN,
        '-o',
        str(TEST_FLASH_ADDRESS),
    ],
]


def require_patch(program):
    assert program in ['bossac', 'stty']

@patch('runners.bossac.BossacBinaryRunner.supports', return_value=True)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_init(cc, req, supports, runner_config):
    '''Test commands using a runner created by constructor.'''
    runner = BossacBinaryRunner(runner_config, port=TEST_BOSSAC_PORT,
                                offset=TEST_OFFSET)
    runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS_WITH_OFFSET]

@patch('runners.bossac.BossacBinaryRunner.supports', return_value=True)
@patch('runners.core.BuildConfiguration._init')
@patch('runners.core.ZephyrBinaryRunner.get_flash_address',
       return_value=None)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_create(cc, req, gfa, bcfg, supports, runner_config):
    '''Test commands using a runner created from command line parameters.'''
    args = ['--bossac-port', str(TEST_BOSSAC_PORT)]
    parser = argparse.ArgumentParser()
    BossacBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = BossacBinaryRunner.create(runner_config, arg_namespace)
    runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS]


@patch('runners.bossac.BossacBinaryRunner.supports', return_value=True)
@patch('runners.core.BuildConfiguration._init')
@patch('runners.core.ZephyrBinaryRunner.get_flash_address',
       return_value=TEST_FLASH_ADDRESS)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_create_with_offset(cc, req, gfa, bcfg, supports,
                                   runner_config):
    '''Test commands using a runner created from command line parameters.'''
    args = ['--bossac-port', str(TEST_BOSSAC_PORT),
            '--offset', str(TEST_OFFSET)]
    parser = argparse.ArgumentParser()
    BossacBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = BossacBinaryRunner.create(runner_config, arg_namespace)
    runner.run('flash')
    assert cc.call_args_list == [call(x) for x in EXPECTED_COMMANDS_WITH_OFFSET]


@patch('runners.bossac.BossacBinaryRunner.supports', return_value=True)
@patch('runners.core.BuildConfiguration._init')
@patch('runners.core.ZephyrBinaryRunner.get_flash_address',
       return_value=TEST_FLASH_ADDRESS)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_bossac_create_with_flash_address(cc, req, gfa, bcfg, supports,
                                          runner_config):
    args = [
        '--bossac-port',
        str(TEST_BOSSAC_PORT),
    ]
    parser = argparse.ArgumentParser()
    BossacBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = BossacBinaryRunner.create(runner_config, arg_namespace)
    runner.run('flash')
    assert cc.call_args_list == [
        call(x) for x in EXPECTED_COMMANDS_WITH_FLASH_ADDRESS
    ]
