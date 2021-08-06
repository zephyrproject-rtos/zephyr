# Copyright (c) 2018 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from unittest.mock import patch

import pytest

from runners.pyocd import PyOcdBinaryRunner
from conftest import RC_BUILD_DIR, RC_GDB, RC_KERNEL_HEX, RC_KERNEL_ELF


#
# Test values to provide as constructor arguments and command line
# parameters, to verify they're respected.
#

TEST_PYOCD = 'test-pyocd'
TEST_ADDR = 0xadd
TEST_DEV_ID = 'test-dev-id'
TEST_FREQUENCY = 'test-frequency'
TEST_DAPARG = 'test-daparg'
TEST_TARGET = 'test-target'
TEST_FLASH_OPTS = ['--test-flash', 'args']
TEST_GDB_PORT = 1
TEST_TELNET_PORT = 2
TEST_TOOL_OPT = 'test-opt'

TEST_ALL_KWARGS = {
    'pyocd': TEST_PYOCD,
    'flash_addr': TEST_ADDR,
    'flash_opts': TEST_FLASH_OPTS,
    'gdb_port': TEST_GDB_PORT,
    'telnet_port': TEST_TELNET_PORT,
    'tui': False,
    'dev_id': TEST_DEV_ID,
    'frequency': TEST_FREQUENCY,
    'daparg': TEST_DAPARG,
    'tool_opt': TEST_TOOL_OPT,
}

TEST_DEF_KWARGS = {}

TEST_ALL_PARAMS = (['--target', TEST_TARGET,
                    '--daparg', TEST_DAPARG,
                    '--pyocd', TEST_PYOCD] +
                   ['--flash-opt={}'.format(o) for o in
                    TEST_FLASH_OPTS] +
                   ['--gdb-port', str(TEST_GDB_PORT),
                    '--telnet-port', str(TEST_TELNET_PORT),
                    '--dev-id', TEST_DEV_ID,
                    '--frequency', str(TEST_FREQUENCY),
                    '--tool-opt', TEST_TOOL_OPT])

TEST_DEF_PARAMS = ['--target', TEST_TARGET]

#
# Expected results.
#
# These record expected argument lists for system calls made by the
# pyocd runner using its check_call() and run_server_and_client()
# methods.
#
# They are shared between tests that create runners directly and
# tests that construct runners from parsed command-line arguments, to
# ensure that results are consistent.
#

FLASH_ALL_EXPECTED_CALL = ([TEST_PYOCD,
                            'flash',
                            '-e', 'sector',
                            '-a', hex(TEST_ADDR), '-da', TEST_DAPARG,
                            '-t', TEST_TARGET, '-u', TEST_DEV_ID,
                            '-f', TEST_FREQUENCY,
                            TEST_TOOL_OPT] +
                           TEST_FLASH_OPTS +
                           [RC_KERNEL_HEX])
FLASH_DEF_EXPECTED_CALL = ['pyocd', 'flash', '-e', 'sector',
                           '-t', TEST_TARGET, RC_KERNEL_HEX]


DEBUG_ALL_EXPECTED_SERVER = [TEST_PYOCD,
                             'gdbserver',
                             '-da', TEST_DAPARG,
                             '-p', str(TEST_GDB_PORT),
                             '-T', str(TEST_TELNET_PORT),
                             '-t', TEST_TARGET,
                             '-u', TEST_DEV_ID,
                             '-f', TEST_FREQUENCY,
                             TEST_TOOL_OPT]
DEBUG_ALL_EXPECTED_CLIENT = [RC_GDB, RC_KERNEL_ELF,
                             '-ex', 'target remote :{}'.format(TEST_GDB_PORT),
                             '-ex', 'monitor halt',
                             '-ex', 'monitor reset',
                             '-ex', 'load']
DEBUG_DEF_EXPECTED_SERVER = ['pyocd',
                             'gdbserver',
                             '-p', '3333',
                             '-T', '4444',
                             '-t', TEST_TARGET]
DEBUG_DEF_EXPECTED_CLIENT = [RC_GDB, RC_KERNEL_ELF,
                             '-ex', 'target remote :3333',
                             '-ex', 'monitor halt',
                             '-ex', 'monitor reset',
                             '-ex', 'load']


DEBUGSERVER_ALL_EXPECTED_CALL = [TEST_PYOCD,
                                 'gdbserver',
                                 '-da', TEST_DAPARG,
                                 '-p', str(TEST_GDB_PORT),
                                 '-T', str(TEST_TELNET_PORT),
                                 '-t', TEST_TARGET,
                                 '-u', TEST_DEV_ID,
                                 '-f', TEST_FREQUENCY,
                                 TEST_TOOL_OPT]
DEBUGSERVER_DEF_EXPECTED_CALL = ['pyocd',
                                 'gdbserver',
                                 '-p', '3333',
                                 '-T', '4444',
                                 '-t', TEST_TARGET]


#
# Fixtures
#

@pytest.fixture
def pyocd(runner_config, tmpdir):
    '''PyOcdBinaryRunner from constructor kwargs or command line parameters'''
    # This factory takes either a dict of kwargs to pass to the
    # constructor, or a list of command-line arguments to parse and
    # use with the create() method.
    def _factory(args):
        # Ensure kernel binaries exist (as empty files, so commands
        # which use them must be patched out).
        tmpdir.ensure(RC_KERNEL_HEX)
        tmpdir.ensure(RC_KERNEL_ELF)
        tmpdir.chdir()

        if isinstance(args, dict):
            return PyOcdBinaryRunner(runner_config, TEST_TARGET, **args)
        elif isinstance(args, list):
            parser = argparse.ArgumentParser()
            PyOcdBinaryRunner.add_parser(parser)
            arg_namespace = parser.parse_args(args)
            return PyOcdBinaryRunner.create(runner_config, arg_namespace)
    return _factory


#
# Helpers
#

def require_patch(program):
    assert program in ['pyocd', TEST_PYOCD, RC_GDB]


#
# Test cases for runners created by constructor.
#

@pytest.mark.parametrize('pyocd_args,expected', [
    (TEST_ALL_KWARGS, FLASH_ALL_EXPECTED_CALL),
    (TEST_DEF_KWARGS, FLASH_DEF_EXPECTED_CALL)
])
@patch('runners.pyocd.PyOcdBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_flash(require, cc, pyocd_args, expected, pyocd):
    pyocd(pyocd_args).run('flash')
    assert require.called
    cc.assert_called_once_with(expected)


@pytest.mark.parametrize('pyocd_args,expectedv', [
    (TEST_ALL_KWARGS, (DEBUG_ALL_EXPECTED_SERVER, DEBUG_ALL_EXPECTED_CLIENT)),
    (TEST_DEF_KWARGS, (DEBUG_DEF_EXPECTED_SERVER, DEBUG_DEF_EXPECTED_CLIENT))
])
@patch('runners.pyocd.PyOcdBinaryRunner.run_server_and_client')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debug(require, rsc, pyocd_args, expectedv, pyocd):
    pyocd(pyocd_args).run('debug')
    assert require.called
    rsc.assert_called_once_with(*expectedv)


@pytest.mark.parametrize('pyocd_args,expected', [
    (TEST_ALL_KWARGS, DEBUGSERVER_ALL_EXPECTED_CALL),
    (TEST_DEF_KWARGS, DEBUGSERVER_DEF_EXPECTED_CALL)
])
@patch('runners.pyocd.PyOcdBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debugserver(require, cc, pyocd_args, expected, pyocd):
    pyocd(pyocd_args).run('debugserver')
    assert require.called
    cc.assert_called_once_with(expected)


#
# Test cases for runners created via command line arguments.
#
# (Unlike the constructor tests, these require additional patching to mock and
# verify runners.core.BuildConfiguration usage.)
#

@pytest.mark.parametrize('pyocd_args,flash_addr,expected', [
    (TEST_ALL_PARAMS, TEST_ADDR, FLASH_ALL_EXPECTED_CALL),
    (TEST_DEF_PARAMS, 0x0, FLASH_DEF_EXPECTED_CALL)
])
@patch('runners.pyocd.BuildConfiguration')
@patch('runners.pyocd.PyOcdBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_flash_args(require, cc, bc, pyocd_args, flash_addr, expected, pyocd):
    with patch.object(PyOcdBinaryRunner, 'get_flash_address',
                      return_value=flash_addr):
        pyocd(pyocd_args).run('flash')
        assert require.called
        bc.assert_called_once_with(RC_BUILD_DIR)
        cc.assert_called_once_with(expected)


@pytest.mark.parametrize('pyocd_args, expectedv', [
    (TEST_ALL_PARAMS, (DEBUG_ALL_EXPECTED_SERVER, DEBUG_ALL_EXPECTED_CLIENT)),
    (TEST_DEF_PARAMS, (DEBUG_DEF_EXPECTED_SERVER, DEBUG_DEF_EXPECTED_CLIENT)),
])
@patch('runners.pyocd.BuildConfiguration')
@patch('runners.pyocd.PyOcdBinaryRunner.run_server_and_client')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debug_args(require, rsc, bc, pyocd_args, expectedv, pyocd):
    pyocd(pyocd_args).run('debug')
    assert require.called
    bc.assert_called_once_with(RC_BUILD_DIR)
    rsc.assert_called_once_with(*expectedv)


@pytest.mark.parametrize('pyocd_args, expected', [
    (TEST_ALL_PARAMS, DEBUGSERVER_ALL_EXPECTED_CALL),
    (TEST_DEF_PARAMS, DEBUGSERVER_DEF_EXPECTED_CALL),
])
@patch('runners.pyocd.BuildConfiguration')
@patch('runners.pyocd.PyOcdBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debugserver_args(require, cc, bc, pyocd_args, expected, pyocd):
    pyocd(pyocd_args).run('debugserver')
    assert require.called
    bc.assert_called_once_with(RC_BUILD_DIR)
    cc.assert_called_once_with(expected)
