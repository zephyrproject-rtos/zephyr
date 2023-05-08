# Copyright (c) 2020 Synopsys
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from os import path, fspath
from unittest.mock import patch
from unittest.mock import call

import pytest

from runners.mdb import MdbNsimBinaryRunner, MdbHwBinaryRunner
from conftest import RC_KERNEL_ELF, RC_BOARD_DIR, RC_BUILD_DIR


TEST_DRIVER_CMD = 'mdb'
TEST_NSIM_ARGS='test_nsim.args'
TEST_TARGET = 'test-target'
TEST_BOARD_NSIM_ARGS = '@' + path.join(RC_BOARD_DIR, 'support', TEST_NSIM_ARGS)

DOTCONFIG_HOSTLINK = f'''
CONFIG_ARC=y
CONFIG_UART_HOSTLINK=y
'''

DOTCONFIG_NO_HOSTLINK = f'''
CONFIG_ARC=y
'''

# mdb-nsim
TEST_NSIM_FLASH_CASES = [
    {
        'i': ['--cores=1', '--nsim_args=' + TEST_NSIM_ARGS],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
           '-toggle=include_local_symbols=1',
           '-nsim', TEST_BOARD_NSIM_ARGS,
           '-run', '-cl', RC_KERNEL_ELF]
    }]

TEST_NSIM_DEBUG_CASES = [
    {
        'i': ['--cores=1', '--nsim_args=' + TEST_NSIM_ARGS],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
           '-toggle=include_local_symbols=1',
           '-nsim', TEST_BOARD_NSIM_ARGS,
           '-OKN', RC_KERNEL_ELF
          ]
    }]

TEST_NSIM_MULTICORE_CASES = [['--cores=2', '--nsim_args=' + TEST_NSIM_ARGS]]
TEST_NSIM_CORE1 = [TEST_DRIVER_CMD, '-pset=1', '-psetname=core0',
              '-nooptions', '-nogoifmain', '-toggle=include_local_symbols=1',
              '-nsim', TEST_BOARD_NSIM_ARGS, RC_KERNEL_ELF]
TEST_NSIM_CORE2 = [TEST_DRIVER_CMD, '-pset=2', '-psetname=core1',
              '-prop=download=2', '-nooptions', '-nogoifmain',
              '-toggle=include_local_symbols=1',
              '-nsim', TEST_BOARD_NSIM_ARGS, RC_KERNEL_ELF]
TEST_NSIM_CORES_LAUNCH = [TEST_DRIVER_CMD, '-multifiles=core1,core0',
              '-run', '-cl']

# mdb-hw
TEST_HW_FLASH_CASES_NO_HOSTLINK = [
    {
        'i': ['--jtag=digilent', '--cores=1'],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
           '-toggle=include_local_symbols=1',
           '-digilent',
            '-run', '-cmd=-nowaitq run', '-cmd=quit', '-cl', RC_KERNEL_ELF]
    }, {
        'i': ['--jtag=digilent', '--cores=1', '--dig-device=test'],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
           '-toggle=include_local_symbols=1',
           '-digilent', '-prop=dig_device=test',
           '-run', '-cmd=-nowaitq run', '-cmd=quit', '-cl', RC_KERNEL_ELF]
    }]

TEST_HW_FLASH_CASES_HOSTLINK = [
    {
        'i': ['--jtag=digilent', '--cores=1'],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
           '-toggle=include_local_symbols=1',
           '-digilent', '-run', '-cl', RC_KERNEL_ELF]
    }, {
        'i': ['--jtag=digilent', '--cores=1', '--dig-device=test'],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
           '-toggle=include_local_symbols=1',
           '-digilent', '-prop=dig_device=test', '-run', '-cl', RC_KERNEL_ELF]
    }]

TEST_HW_FLASH_CASES_ERR = [
    {
        'i': ['--jtag=test_debug', '--cores=1'],
        'e': "unsupported jtag adapter test_debug"
    },{
        'i': ['--jtag=digilent', '--cores=16'],
        'e': "unsupported cores 16"
    }]

TEST_HW_DEBUG_CASES = [
    {
        'i': ['--jtag=digilent', '--cores=1'],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
               '-toggle=include_local_symbols=1',
               '-digilent',
               '-OKN', RC_KERNEL_ELF]
    }, {
        'i': ['--jtag=digilent', '--cores=1', '--dig-device=test'],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
               '-toggle=include_local_symbols=1',
               '-digilent', '-prop=dig_device=test',
               '-OKN', RC_KERNEL_ELF]
    }]

TEST_HW_DEBUG_CASES_ERR = [
    {
        'i': ['--jtag=test_debug', '--cores=1'],
        'e': "unsupported jtag adapter test_debug"
    }, {
        'i': ['--jtag=digilent', '--cores=16'],
        'e': "unsupported cores 16"
    }]

TEST_HW_MULTICORE_CASES = [['--jtag=digilent', '--cores=2']]
TEST_HW_CORE1 = [TEST_DRIVER_CMD, '-pset=1', '-psetname=core0',
              '-nooptions', '-nogoifmain', '-toggle=include_local_symbols=1',
              '-digilent', RC_KERNEL_ELF]
TEST_HW_CORE2 = [TEST_DRIVER_CMD, '-pset=2', '-psetname=core1',
              '-prop=download=2', '-nooptions', '-nogoifmain',
              '-toggle=include_local_symbols=1',
              '-digilent', RC_KERNEL_ELF]
TEST_HW_CORES_LAUNCH_NO_HOSTLINK = [TEST_DRIVER_CMD, '-multifiles=core1,core0', '-run',
              '-cmd=-nowaitq run', '-cmd=quit', '-cl']
TEST_HW_CORES_LAUNCH_HOSTLINK = [TEST_DRIVER_CMD, '-multifiles=core1,core0', '-run', '-cl']


def adjust_runner_config(runner_config, tmpdir, dotconfig):
    # Adjust a RunnerConfig object, 'runner_config', by
    # replacing its build directory with 'tmpdir' after writing
    # the contents of 'dotconfig' to tmpdir/zephyr/.config.

    zephyr = tmpdir / 'zephyr'
    zephyr.mkdir()
    with open(zephyr / '.config', 'w') as f:
        f.write(dotconfig)

    print("" + fspath(tmpdir))
    return runner_config._replace(build_dir=fspath(tmpdir))

#
# Fixtures
#

def mdb(runner_config, tmpdir, mdb_runner):
    '''MdbBinaryRunner from constructor kwargs or command line parameters'''
    # This factory takes either a dict of kwargs to pass to the
    # constructor, or a list of command-line arguments to parse and
    # use with the create() method.
    def _factory(args):
        # Ensure kernel binaries exist (as empty files, so commands
        # which use them must be patched out).
        tmpdir.ensure(RC_KERNEL_ELF)
        tmpdir.chdir()

        if isinstance(args, dict):
            return mdb_runner(runner_config, TEST_TARGET, **args)
        elif isinstance(args, list):
            parser = argparse.ArgumentParser(allow_abbrev=False)
            mdb_runner.add_parser(parser)
            arg_namespace = parser.parse_args(args)
            return mdb_runner.create(runner_config, arg_namespace)

    return _factory

@pytest.fixture
def mdb_nsim(runner_config, tmpdir):
    return mdb(runner_config, tmpdir, MdbNsimBinaryRunner)

@pytest.fixture
def mdb_hw_no_hl(runner_config, tmpdir):
    runner_config = adjust_runner_config(runner_config, tmpdir, DOTCONFIG_NO_HOSTLINK)
    return mdb(runner_config, tmpdir, MdbHwBinaryRunner)

@pytest.fixture
def mdb_hw_hl(runner_config, tmpdir):
    runner_config = adjust_runner_config(runner_config, tmpdir, DOTCONFIG_HOSTLINK)
    return mdb(runner_config, tmpdir, MdbHwBinaryRunner)

#
# Helpers
#

def require_patch(program):
    assert program == TEST_DRIVER_CMD

#
# Test cases for runners created by constructor.
#

# mdb-nsim test cases
@pytest.mark.parametrize('test_case', TEST_NSIM_FLASH_CASES)
@patch('runners.mdb.MdbNsimBinaryRunner.call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_flash_nsim(require, cc, test_case, mdb_nsim):
    mdb_nsim(test_case['i']).run('flash')
    assert require.called
    cc.assert_called_once_with(test_case['o'], cwd=RC_BUILD_DIR)

@pytest.mark.parametrize('test_case', TEST_NSIM_DEBUG_CASES)
@patch('runners.mdb.MdbNsimBinaryRunner.call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debug_nsim(require, pii, test_case, mdb_nsim):
    mdb_nsim(test_case['i']).run('debug')
    assert require.called
    pii.assert_called_once_with(test_case['o'], cwd=RC_BUILD_DIR)

@pytest.mark.parametrize('test_case', TEST_NSIM_MULTICORE_CASES)
@patch('runners.mdb.MdbNsimBinaryRunner.check_call')
@patch('runners.mdb.MdbNsimBinaryRunner.call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_multicores_nsim(require, pii, cc, test_case, mdb_nsim):
    mdb_nsim(test_case).run('flash')
    assert require.called
    cc_calls = [call(TEST_NSIM_CORE1, cwd=RC_BUILD_DIR), call(TEST_NSIM_CORE2, cwd=RC_BUILD_DIR)]
    cc.assert_has_calls(cc_calls)
    pii.assert_called_once_with(TEST_NSIM_CORES_LAUNCH, cwd=RC_BUILD_DIR)


# mdb-hw test cases
@pytest.mark.parametrize('test_case', TEST_HW_FLASH_CASES_NO_HOSTLINK)
@patch('runners.mdb.MdbHwBinaryRunner.call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_flash_hw_no_hl(require, cc, test_case, mdb_hw_no_hl, tmpdir):
    mdb_hw_no_hl(test_case['i']).run('flash')
    assert require.called
    cc.assert_called_once_with(test_case['o'], cwd=tmpdir)

@pytest.mark.parametrize('test_case', TEST_HW_FLASH_CASES_HOSTLINK)
@patch('runners.mdb.MdbHwBinaryRunner.call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_flash_hw_hl(require, cc, test_case, mdb_hw_hl, tmpdir):
    mdb_hw_hl(test_case['i']).run('flash')
    assert require.called
    cc.assert_called_once_with(test_case['o'], cwd=tmpdir)

@pytest.mark.parametrize('test_case', TEST_HW_FLASH_CASES_ERR)
@patch('runners.mdb.MdbHwBinaryRunner.call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_flash_hw_err(require, cc, test_case, mdb_hw_no_hl):
    with pytest.raises(ValueError) as rinfo:
        mdb_hw_no_hl(test_case['i']).run('flash')

    assert str(rinfo.value) == test_case['e']

@pytest.mark.parametrize('test_case', TEST_HW_DEBUG_CASES)
@patch('runners.mdb.MdbHwBinaryRunner.call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debug_hw(require, pii, test_case, mdb_hw_no_hl, tmpdir):
    mdb_hw_no_hl(test_case['i']).run('debug')
    assert require.called
    pii.assert_called_once_with(test_case['o'], cwd=tmpdir)

@pytest.mark.parametrize('test_case', TEST_HW_DEBUG_CASES)
@patch('runners.mdb.MdbHwBinaryRunner.call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debug_hw_hl(require, pii, test_case, mdb_hw_hl, tmpdir):
    mdb_hw_hl(test_case['i']).run('debug')
    assert require.called
    pii.assert_called_once_with(test_case['o'], cwd=tmpdir)

@pytest.mark.parametrize('test_case', TEST_HW_DEBUG_CASES_ERR)
@patch('runners.mdb.MdbHwBinaryRunner.call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debug_hw_err(require, pii, test_case, mdb_hw_no_hl):
    with pytest.raises(ValueError) as rinfo:
        mdb_hw_no_hl(test_case['i']).run('debug')

    assert str(rinfo.value) == test_case['e']

@pytest.mark.parametrize('test_case', TEST_HW_MULTICORE_CASES)
@patch('runners.mdb.MdbHwBinaryRunner.check_call')
@patch('runners.mdb.MdbHwBinaryRunner.call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_multicores_hw_no_hl(require, pii, cc, test_case, mdb_hw_no_hl, tmpdir):
    mdb_hw_no_hl(test_case).run('flash')
    assert require.called
    cc_calls = [call(TEST_HW_CORE1, cwd=tmpdir), call(TEST_HW_CORE2, cwd=tmpdir)]
    cc.assert_has_calls(cc_calls)
    pii.assert_called_once_with(TEST_HW_CORES_LAUNCH_NO_HOSTLINK, cwd=tmpdir)

@pytest.mark.parametrize('test_case', TEST_HW_MULTICORE_CASES)
@patch('runners.mdb.MdbHwBinaryRunner.check_call')
@patch('runners.mdb.MdbHwBinaryRunner.call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_multicores_hw_hl(require, pii, cc, test_case, mdb_hw_hl, tmpdir):
    mdb_hw_hl(test_case).run('flash')
    assert require.called
    cc_calls = [call(TEST_HW_CORE1, cwd=tmpdir), call(TEST_HW_CORE2, cwd=tmpdir)]
    cc.assert_has_calls(cc_calls)
    pii.assert_called_once_with(TEST_HW_CORES_LAUNCH_HOSTLINK, cwd=tmpdir)
