# Copyright (c) 2020 Synopsys
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from os import path
from unittest.mock import patch
from unittest.mock import call

import pytest

from runners.mdb import MdbNsimBinaryRunner, MdbHwBinaryRunner
from conftest import RC_KERNEL_ELF, RC_BOARD_DIR, RC_BUILD_DIR


TEST_DRIVER_CMD = 'mdb'
TEST_NSIM_ARGS='test_nsim.args'
TEST_TARGET = 'test-target'
TEST_BOARD_NSIM_ARGS = '@' + path.join(RC_BOARD_DIR, 'support', TEST_NSIM_ARGS)

# mdb-nsim
TEST_NSIM_FLASH_CASES = [
    {
        'i': ['--cores=1', '--nsim=' + TEST_NSIM_ARGS],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
           '-toggle=include_local_symbols=1',
           '-nsim', TEST_BOARD_NSIM_ARGS,
           '-run', '-cl', RC_KERNEL_ELF]
    }]

TEST_NSIM_DEBUG_CASES = [
    {
        'i': ['--cores=1', '--nsim=' + TEST_NSIM_ARGS],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
           '-toggle=include_local_symbols=1',
           '-nsim', TEST_BOARD_NSIM_ARGS,
           '-OKN', RC_KERNEL_ELF
          ]
    }]

TEST_NSIM_MULTICORE_CASES = [['--cores=2', '--nsim=' + TEST_NSIM_ARGS]]
TEST_NSIM_CORE1 = [TEST_DRIVER_CMD, '-pset=1', '-psetname=core0', '',
              '-nooptions', '-nogoifmain', '-toggle=include_local_symbols=1',
              '-nsim', TEST_BOARD_NSIM_ARGS, RC_KERNEL_ELF]
TEST_NSIM_CORE2 = [TEST_DRIVER_CMD, '-pset=2', '-psetname=core1',
              '-prop=download=2', '-nooptions', '-nogoifmain',
              '-toggle=include_local_symbols=1',
              '-nsim', TEST_BOARD_NSIM_ARGS, RC_KERNEL_ELF]
TEST_NSIM_CORES_LAUNCH = [TEST_DRIVER_CMD, '-multifiles=core1,core0',
              '-run', '-cl']

# mdb-hw
TEST_HW_FLASH_CASES = [
    {
        'i': ['--jtag=digilent', '--cores=1'],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
           '-toggle=include_local_symbols=1',
           '-digilent', '',
            '-run', '-cmd=-nowaitq run', '-cmd=quit', '-cl', RC_KERNEL_ELF]
    }, {
        'i': ['--jtag=digilent', '--cores=1', '--dig-device=test'],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
           '-toggle=include_local_symbols=1',
           '-digilent', '-prop=dig_device=test',
           '-run', '-cmd=-nowaitq run', '-cmd=quit', '-cl', RC_KERNEL_ELF]
    }, {
        'i': ['--jtag=test_debug', '--cores=1'],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
           '-toggle=include_local_symbols=1',
           '',
           '-run', '-cmd=-nowaitq run', '-cmd=quit', '-cl', RC_KERNEL_ELF]
    }]

TEST_HW_DEBUG_CASES = [
    {
        'i': ['--jtag=digilent', '--cores=1'],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
               '-toggle=include_local_symbols=1',
               '-digilent', '',
               '-OKN', RC_KERNEL_ELF]
    }, {
        'i': ['--jtag=digilent', '--cores=1', '--dig-device=test'],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
               '-toggle=include_local_symbols=1',
               '-digilent', '-prop=dig_device=test',
               '-OKN', RC_KERNEL_ELF]
    }, {
        'i': ['--jtag=test_debug', '--cores=1'],
        'o': [TEST_DRIVER_CMD, '-nooptions', '-nogoifmain',
               '-toggle=include_local_symbols=1',
               '',
               '-OKN', RC_KERNEL_ELF]
    }]

TEST_HW_MULTICORE_CASES = [['--jtag=digilent', '--cores=2']]
TEST_HW_CORE1 = [TEST_DRIVER_CMD, '-pset=1', '-psetname=core0', '',
              '-nooptions', '-nogoifmain', '-toggle=include_local_symbols=1',
              '-digilent', '', RC_KERNEL_ELF]
TEST_HW_CORE2 = [TEST_DRIVER_CMD, '-pset=2', '-psetname=core1',
              '-prop=download=2', '-nooptions', '-nogoifmain',
              '-toggle=include_local_symbols=1',
              '-digilent', '', RC_KERNEL_ELF]
TEST_HW_CORES_LAUNCH = [TEST_DRIVER_CMD, '-multifiles=core1,core0', '-run',
              '-cmd=-nowaitq run', '-cmd=quit', '-cl']

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
            parser = argparse.ArgumentParser()
            mdb_runner.add_parser(parser)
            arg_namespace = parser.parse_args(args)
            return mdb_runner.create(runner_config, arg_namespace)

    return _factory

@pytest.fixture
def mdb_nsim(runner_config, tmpdir):
    return mdb(runner_config, tmpdir, MdbNsimBinaryRunner)

@pytest.fixture
def mdb_hw(runner_config, tmpdir):
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
@patch('runners.mdb.get_cld_pid', return_value=(False, -1))
@patch('time.sleep', return_value=None)
@patch('runners.mdb.MdbNsimBinaryRunner.popen_ignore_int')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_flash_nsim(require, cc, t, gcp, test_case, mdb_nsim):
    mdb_nsim(test_case['i']).run('flash')
    assert require.called
    cc.assert_called_once_with(test_case['o'], cwd=RC_BUILD_DIR)

@pytest.mark.parametrize('test_case', TEST_NSIM_DEBUG_CASES)
@patch('runners.mdb.get_cld_pid', return_value=(False, -1))
@patch('time.sleep', return_value=None)
@patch('runners.mdb.MdbNsimBinaryRunner.popen_ignore_int')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debug_nsim(require, pii, t, gcp, test_case, mdb_nsim):
    mdb_nsim(test_case['i']).run('debug')
    assert require.called
    pii.assert_called_once_with(test_case['o'], cwd=RC_BUILD_DIR)

@pytest.mark.parametrize('test_case', TEST_NSIM_MULTICORE_CASES)
@patch('runners.mdb.get_cld_pid', return_value=(False, -1))
@patch('time.sleep', return_value=None)
@patch('runners.mdb.MdbNsimBinaryRunner.check_call')
@patch('runners.mdb.MdbNsimBinaryRunner.popen_ignore_int')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_multicores_nsim(require, pii, cc, t, gcp, test_case, mdb_nsim):
    mdb_nsim(test_case).run('flash')
    assert require.called
    cc_calls = [call(TEST_NSIM_CORE1, cwd=RC_BUILD_DIR), call(TEST_NSIM_CORE2, cwd=RC_BUILD_DIR)]
    cc.assert_has_calls(cc_calls)
    pii.assert_called_once_with(TEST_NSIM_CORES_LAUNCH, cwd=RC_BUILD_DIR)


# mdb-hw test cases
@pytest.mark.parametrize('test_case', TEST_HW_FLASH_CASES)
@patch('runners.mdb.get_cld_pid', return_value=(False, -1))
@patch('time.sleep', return_value=None)
@patch('runners.mdb.MdbHwBinaryRunner.popen_ignore_int')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_flash_hw(require, cc, t, gcp, test_case, mdb_hw):
    mdb_hw(test_case['i']).run('flash')
    assert require.called
    cc.assert_called_once_with(test_case['o'], cwd=RC_BUILD_DIR)

@pytest.mark.parametrize('test_case', TEST_HW_DEBUG_CASES)
@patch('runners.mdb.get_cld_pid', return_value=(False, -1))
@patch('time.sleep', return_value=None)
@patch('runners.mdb.MdbHwBinaryRunner.popen_ignore_int')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debug_hw(require, pii, t, gcp, test_case, mdb_hw):
    mdb_hw(test_case['i']).run('debug')
    assert require.called
    pii.assert_called_once_with(test_case['o'], cwd=RC_BUILD_DIR)

@pytest.mark.parametrize('test_case', TEST_HW_MULTICORE_CASES)
@patch('runners.mdb.get_cld_pid', return_value=(False, -1))
@patch('time.sleep', return_value=None)
@patch('runners.mdb.MdbHwBinaryRunner.check_call')
@patch('runners.mdb.MdbHwBinaryRunner.popen_ignore_int')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_multicores_hw(require, pii, cc, t, gcp, test_case, mdb_hw):
    mdb_hw(test_case).run('flash')
    assert require.called
    cc_calls = [call(TEST_HW_CORE1, cwd=RC_BUILD_DIR), call(TEST_HW_CORE2, cwd=RC_BUILD_DIR)]
    cc.assert_has_calls(cc_calls)
    pii.assert_called_once_with(TEST_HW_CORES_LAUNCH, cwd=RC_BUILD_DIR)
