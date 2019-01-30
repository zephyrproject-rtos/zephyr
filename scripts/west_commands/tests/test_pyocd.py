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
TEST_BOARD_ID = 'test-board-id'
TEST_FREQUENCY = 'test-frequency'
TEST_DAPARG = 'test-daparg'
TEST_TARGET = 'test-target'
TEST_FLASH_OPTS = ['--test-flash', 'args']
TEST_PORT = 1

TEST_ALL_KWARGS = {
    'pyocd': TEST_PYOCD,
    'flash_addr': TEST_ADDR,
    'flash_opts': TEST_FLASH_OPTS,
    'gdb_port': TEST_PORT,
    'tui': False,
    'board_id': TEST_BOARD_ID,
    'frequency': TEST_FREQUENCY,
    'daparg': TEST_DAPARG,
}

TEST_DEF_KWARGS = {}

TEST_ALL_PARAMS = (['--target', TEST_TARGET,
                    '--daparg', TEST_DAPARG,
                    '--pyocd', TEST_PYOCD] +
                   ['--flash-opt={}'.format(o) for o in
                    TEST_FLASH_OPTS] +
                   ['--gdb-port', str(TEST_PORT),
                    '--board-id', TEST_BOARD_ID,
                    '--frequency', str(TEST_FREQUENCY)])

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
                            '-a', hex(TEST_ADDR), '-da', TEST_DAPARG,
                            '-t', TEST_TARGET, '-b', TEST_BOARD_ID,
                            '-f', TEST_FREQUENCY] +
                           TEST_FLASH_OPTS +
                           [RC_KERNEL_HEX])
FLASH_DEF_EXPECTED_CALL = ['pyocd', 'flash', '-t', TEST_TARGET, RC_KERNEL_HEX]


DEBUG_ALL_EXPECTED_SERVER = [TEST_PYOCD,
                             'gdbserver',
                             '-da', TEST_DAPARG,
                             '-p', str(TEST_PORT),
                             '-t', TEST_TARGET,
                             '-b', TEST_BOARD_ID,
                             '-f', TEST_FREQUENCY]
DEBUG_ALL_EXPECTED_CLIENT = [RC_GDB, RC_KERNEL_ELF,
                             '-ex', 'target remote :{}'.format(TEST_PORT),
                             '-ex', 'monitor halt',
                             '-ex', 'monitor reset',
                             '-ex', 'load']
DEBUG_DEF_EXPECTED_SERVER = ['pyocd',
                             'gdbserver',
                             '-p', '3333',
                             '-t', TEST_TARGET]
DEBUG_DEF_EXPECTED_CLIENT = [RC_GDB, RC_KERNEL_ELF,
                             '-ex', 'target remote :3333',
                             '-ex', 'monitor halt',
                             '-ex', 'monitor reset',
                             '-ex', 'load']


DEBUGSERVER_ALL_EXPECTED_CALL = [TEST_PYOCD,
                                 'gdbserver',
                                 '-da', TEST_DAPARG,
                                 '-p', str(TEST_PORT),
                                 '-t', TEST_TARGET,
                                 '-b', TEST_BOARD_ID,
                                 '-f', TEST_FREQUENCY]
DEBUGSERVER_DEF_EXPECTED_CALL = ['pyocd',
                                 'gdbserver',
                                 '-p', '3333',
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
# Test cases for runners created by constructor.
#

@pytest.mark.parametrize('pyocd_args,expected', [
    (TEST_ALL_KWARGS, FLASH_ALL_EXPECTED_CALL),
    (TEST_DEF_KWARGS, FLASH_DEF_EXPECTED_CALL)
])
@patch('runners.pyocd.PyOcdBinaryRunner.check_call')
def test_flash(cc, pyocd_args, expected, pyocd):
    pyocd(pyocd_args).run('flash')
    cc.assert_called_once_with(expected)


@pytest.mark.parametrize('pyocd_args,expectedv', [
    (TEST_ALL_KWARGS, (DEBUG_ALL_EXPECTED_SERVER, DEBUG_ALL_EXPECTED_CLIENT)),
    (TEST_DEF_KWARGS, (DEBUG_DEF_EXPECTED_SERVER, DEBUG_DEF_EXPECTED_CLIENT))
])
@patch('runners.pyocd.PyOcdBinaryRunner.run_server_and_client')
def test_debug(rsc, pyocd_args, expectedv, pyocd):
    pyocd(pyocd_args).run('debug')
    rsc.assert_called_once_with(*expectedv)


@pytest.mark.parametrize('pyocd_args,expected', [
    (TEST_ALL_KWARGS, DEBUGSERVER_ALL_EXPECTED_CALL),
    (TEST_DEF_KWARGS, DEBUGSERVER_DEF_EXPECTED_CALL)
])
@patch('runners.pyocd.PyOcdBinaryRunner.check_call')
def test_debugserver(cc, pyocd_args, expected, pyocd):
    pyocd(pyocd_args).run('debugserver')
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
def test_flash_args(cc, bc, pyocd_args, flash_addr, expected, pyocd):
    with patch.object(PyOcdBinaryRunner, 'get_flash_address',
                      return_value=flash_addr):
        pyocd(pyocd_args).run('flash')
        bc.assert_called_once_with(RC_BUILD_DIR)
        cc.assert_called_once_with(expected)


@pytest.mark.parametrize('pyocd_args, expectedv', [
    (TEST_ALL_PARAMS, (DEBUG_ALL_EXPECTED_SERVER, DEBUG_ALL_EXPECTED_CLIENT)),
    (TEST_DEF_PARAMS, (DEBUG_DEF_EXPECTED_SERVER, DEBUG_DEF_EXPECTED_CLIENT)),
])
@patch('runners.pyocd.BuildConfiguration')
@patch('runners.pyocd.PyOcdBinaryRunner.run_server_and_client')
def test_debug_args(rsc, bc, pyocd_args, expectedv, pyocd):
    pyocd(pyocd_args).run('debug')
    bc.assert_called_once_with(RC_BUILD_DIR)
    rsc.assert_called_once_with(*expectedv)


@pytest.mark.parametrize('pyocd_args, expected', [
    (TEST_ALL_PARAMS, DEBUGSERVER_ALL_EXPECTED_CALL),
    (TEST_DEF_PARAMS, DEBUGSERVER_DEF_EXPECTED_CALL),
])
@patch('runners.pyocd.BuildConfiguration')
@patch('runners.pyocd.PyOcdBinaryRunner.check_call')
def test_debugserver_args(cc, bc, pyocd_args, expected, pyocd):
    pyocd(pyocd_args).run('debugserver')
    bc.assert_called_once_with(RC_BUILD_DIR)
    cc.assert_called_once_with(expected)
