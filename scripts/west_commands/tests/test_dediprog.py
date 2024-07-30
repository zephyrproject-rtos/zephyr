# Copyright (c) 2018 Foundries.io
# Copyright (c) 2019 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import platform
from unittest.mock import patch, call

import pytest

from runners.dediprog import DediProgBinaryRunner
from conftest import RC_KERNEL_BIN

DPCMD_EXE = 'dpcmd.exe' if platform.system() == 'Windows' else 'dpcmd'

EXPECTED_COMMAND = {
    (RC_KERNEL_BIN, None):
    [DPCMD_EXE,
     '--auto', RC_KERNEL_BIN,
     '-x', 'ff',
     '--silent', '--verify'],


    (RC_KERNEL_BIN, '0'):
    [DPCMD_EXE,
     '--auto', RC_KERNEL_BIN, '--vcc', '0',
     '-x', 'ff',
     '--silent', '--verify'],

    (RC_KERNEL_BIN, '1'):
    [DPCMD_EXE,
     '--auto', RC_KERNEL_BIN, '--vcc', '1',
     '-x', 'ff',
     '--silent', '--verify'],
}

def require_patch(program):
    assert program in [DPCMD_EXE]

def id_fn(tc):
    return 'spi_image={},vcc={}'.format(*tc)

@pytest.mark.parametrize('tc', [
    (RC_KERNEL_BIN, None),
    (RC_KERNEL_BIN, '0'),
    (RC_KERNEL_BIN, '1'),
], ids=id_fn)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_dediprog_init(cc, req, tc, runner_config):
    '''Test commands using a runner created by constructor.'''
    spi_image, vcc = tc
    runner = DediProgBinaryRunner(runner_config, spi_image=spi_image,
                                  vcc=vcc, retries=0)
    runner.run('flash')
    assert cc.call_args_list == [call(EXPECTED_COMMAND[tc])]

@pytest.mark.parametrize('tc', [
    (RC_KERNEL_BIN, None),
    (RC_KERNEL_BIN, '0'),
    (RC_KERNEL_BIN, '1'),
], ids=id_fn)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_dediprog_create(cc, req, tc, runner_config):
    '''Test commands using a runner created from command line parameters.'''
    spi_image, vcc = tc
    args = ['--spi-image', spi_image, '--retries', '0']
    if vcc:
        args.extend(['--vcc', vcc])
    parser = argparse.ArgumentParser(allow_abbrev=False)
    DediProgBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = DediProgBinaryRunner.create(runner_config, arg_namespace)
    runner.run('flash')
    assert cc.call_args_list == [call(EXPECTED_COMMAND[tc])]
