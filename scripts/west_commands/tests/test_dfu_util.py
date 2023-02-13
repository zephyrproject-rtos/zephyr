# Copyright (c) 2018 Foundries.io
# Copyright (c) 2019 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
from unittest.mock import patch, call

import pytest

from runners.dfu import DfuUtilBinaryRunner, DfuSeConfig
from conftest import RC_KERNEL_BIN

DFU_UTIL = 'dfu-util'
TEST_EXE = 'test-dfu-util'
TEST_PID = '0000:9999'
TEST_PID_RES = '-d,{}'.format(TEST_PID)
TEST_ALT_INT = '1'
TEST_ALT_STR = 'alt-name'
TEST_BIN_NAME = 'test-img.bin'
TEST_DFUSE_ADDR = 2
TEST_DFUSE_OPTS = 'test-dfuse-opt'
TEST_DCFG_OPT = DfuSeConfig(address=TEST_DFUSE_ADDR, options='test-dfuse-opt')
TEST_DCFG_OPT_RES = '{}:{}'.format(hex(TEST_DFUSE_ADDR), TEST_DFUSE_OPTS)
TEST_DCFG_NOPT = DfuSeConfig(address=TEST_DFUSE_ADDR, options='')
TEST_DCFG_NOPT_RES = '{}:'.format(hex(TEST_DFUSE_ADDR))
# A map from a test case to the expected dfu-util call.
# Test cases are (alt, exe, img, dfuse) tuples.
EXPECTED_COMMAND = {
    (DFU_UTIL, TEST_ALT_INT, None, RC_KERNEL_BIN):
    [DFU_UTIL, TEST_PID_RES, '-a', TEST_ALT_INT, '-D', RC_KERNEL_BIN],

    (DFU_UTIL, TEST_ALT_STR, None, RC_KERNEL_BIN):
    [DFU_UTIL, TEST_PID_RES, '-a', TEST_ALT_STR, '-D', RC_KERNEL_BIN],

    (TEST_EXE, TEST_ALT_INT, None, RC_KERNEL_BIN):
    [TEST_EXE, TEST_PID_RES, '-a', TEST_ALT_INT, '-D', RC_KERNEL_BIN],

    (DFU_UTIL, TEST_ALT_INT, None, TEST_BIN_NAME):
    [DFU_UTIL, TEST_PID_RES, '-a', TEST_ALT_INT, '-D', TEST_BIN_NAME],

    (DFU_UTIL, TEST_ALT_INT, TEST_DCFG_OPT, RC_KERNEL_BIN):
    [DFU_UTIL, TEST_PID_RES, '-s', TEST_DCFG_OPT_RES, '-a', TEST_ALT_INT,
     '-D', RC_KERNEL_BIN],

    (DFU_UTIL, TEST_ALT_INT, TEST_DCFG_NOPT, RC_KERNEL_BIN):
    [DFU_UTIL, TEST_PID_RES, '-s', TEST_DCFG_NOPT_RES, '-a', TEST_ALT_INT,
     '-D', RC_KERNEL_BIN],
}

def find_device_patch():
    return True

def require_patch(program):
    assert program in [DFU_UTIL, TEST_EXE]

os_path_isfile = os.path.isfile

def os_path_isfile_patch(filename):
    if filename == RC_KERNEL_BIN:
        return True
    return os_path_isfile(filename)

def id_fn(tc):
    return 'exe={},alt={},dfuse_config={},img={}'.format(*tc)

@pytest.mark.parametrize('tc', [
    # (exe, alt, dfuse_config, img)
    (DFU_UTIL, TEST_ALT_INT, None, RC_KERNEL_BIN),
    (DFU_UTIL, TEST_ALT_STR, None, RC_KERNEL_BIN),
    (TEST_EXE, TEST_ALT_INT, None, RC_KERNEL_BIN),
    (DFU_UTIL, TEST_ALT_INT, None, TEST_BIN_NAME),
    (DFU_UTIL, TEST_ALT_INT, TEST_DCFG_OPT, RC_KERNEL_BIN),
    (DFU_UTIL, TEST_ALT_INT, TEST_DCFG_NOPT, RC_KERNEL_BIN),
], ids=id_fn)
@patch('runners.dfu.DfuUtilBinaryRunner.find_device',
       side_effect=find_device_patch)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_dfu_util_init(cc, req, find_device, tc, runner_config):
    '''Test commands using a runner created by constructor.'''
    exe, alt, dfuse_config, img = tc
    runner = DfuUtilBinaryRunner(runner_config, TEST_PID, alt, img, exe=exe,
                                 dfuse_config=dfuse_config)
    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')
    assert find_device.called
    assert req.called_with(exe)
    assert cc.call_args_list == [call(EXPECTED_COMMAND[tc])]

def get_flash_address_patch(args, bcfg):
    return TEST_DFUSE_ADDR

@pytest.mark.parametrize('tc', [
    # arg spec: (exe, alt, dfuse, modifiers, img)
    (None, TEST_ALT_INT, False, None, None),
    (None, TEST_ALT_STR, False, None, None),
    (TEST_EXE, TEST_ALT_INT, False, None, None),
    (None, TEST_ALT_INT, False, None, TEST_BIN_NAME),
    (None, TEST_ALT_INT, True, TEST_DFUSE_OPTS, None),
    (None, TEST_ALT_INT, True, None, None),

], ids=id_fn)
@patch('runners.dfu.DfuUtilBinaryRunner.find_device',
       side_effect=find_device_patch)
@patch('runners.core.ZephyrBinaryRunner.get_flash_address',
       side_effect=get_flash_address_patch)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.core.ZephyrBinaryRunner.check_call')
def test_dfu_util_create(cc, req, gfa, find_device, tc, runner_config, tmpdir):
    '''Test commands using a runner created from command line parameters.'''
    exe, alt, dfuse, modifiers, img = tc
    args = ['--pid', TEST_PID, '--alt', alt]
    if img:
        args.extend(['--img', img])
    if dfuse:
        args.append('--dfuse')
        if modifiers:
            args.extend(['--dfuse-modifiers', modifiers])
        else:
            args.extend(['--dfuse-modifiers', ''])
    if exe:
        args.extend(['--dfu-util', exe])

    (tmpdir / 'zephyr').mkdir()
    with open(os.fspath(tmpdir / 'zephyr' / '.config'), 'w') as f:
        f.write('\n')
    runner_config = runner_config._replace(build_dir=os.fspath(tmpdir))

    parser = argparse.ArgumentParser(allow_abbrev=False)
    DfuUtilBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = DfuUtilBinaryRunner.create(runner_config, arg_namespace)
    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')

    if dfuse:
        cfg = DfuSeConfig(address=TEST_DFUSE_ADDR, options=modifiers or '')
    else:
        cfg = None
    map_tc = (exe or DFU_UTIL, alt, cfg, img or RC_KERNEL_BIN)
    assert find_device.called
    assert req.called_with(exe)
    assert cc.call_args_list == [call(EXPECTED_COMMAND[map_tc])]
