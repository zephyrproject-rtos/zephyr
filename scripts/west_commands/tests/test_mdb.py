# Copyright (c) 2020 Synopsys
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from os import path
from unittest.mock import patch
from unittest.mock import call

import pytest

from runners.mdb import MdbBinaryRunner
from conftest import RC_KERNEL_ELF, RC_BOARD_DIR


TEST_MDB = 'mdb'
TEST_NSIM_ARGS='test_nsim.args'
TEST_TARGET = 'test-target'
TEST_BOARD_NSIM_ARGS = '@' + path.join(RC_BOARD_DIR, 'support', TEST_NSIM_ARGS)

TEST_FLASH_CASES = [
        {'i': ['--jtag=digilent', '--cores=1'],
         'o': [TEST_MDB, '-nooptions', '-nogoifmain',
               '-toggle=include_local_symbols=1',
               '-digilent', '',
                '-run', '-cmd=-nowaitq run', '-cmd=quit', '-cl', RC_KERNEL_ELF
              ]
        },
        {'i': ['--jtag=digilent', '--cores=1', '--dig-device=test'],
         'o': [TEST_MDB, '-nooptions', '-nogoifmain',
               '-toggle=include_local_symbols=1',
               '-digilent', '-prop=dig_device=test',
               '-run', '-cmd=-nowaitq run', '-cmd=quit', '-cl', RC_KERNEL_ELF
              ]
        },
        {'i': ['--jtag=test_debug', '--cores=1'],
         'o': [TEST_MDB, '-nooptions', '-nogoifmain',
               '-toggle=include_local_symbols=1',
               '',
               '-run', '-cmd=-nowaitq run', '-cmd=quit', '-cl', RC_KERNEL_ELF
              ]
        },
        {'i': ['--cores=1', '--nsim=' + TEST_NSIM_ARGS],
         'o': [TEST_MDB, '-nooptions', '-nogoifmain',
               '-toggle=include_local_symbols=1',
               '-nsim', TEST_BOARD_NSIM_ARGS,
               '-run', '-cl', RC_KERNEL_ELF
              ]
        },

        ]


TEST_DEBUG_CASES = [
        {'i': ['--jtag=digilent', '--cores=1'],
         'o': [TEST_MDB, '-nooptions', '-nogoifmain',
               '-toggle=include_local_symbols=1',
               '-digilent', '',
               '-OKN', RC_KERNEL_ELF
              ]
        },
        {'i': ['--jtag=digilent', '--cores=1', '--dig-device=test'],
         'o': [TEST_MDB, '-nooptions', '-nogoifmain',
               '-toggle=include_local_symbols=1',
               '-digilent', '-prop=dig_device=test',
               '-OKN', RC_KERNEL_ELF
              ]
        },
        {'i': ['--jtag=test_debug', '--cores=1'],
         'o': [TEST_MDB, '-nooptions', '-nogoifmain',
               '-toggle=include_local_symbols=1',
               '',
               '-OKN', RC_KERNEL_ELF
              ]
        },
        {'i': ['--cores=1','--nsim=' + TEST_NSIM_ARGS],
         'o': [TEST_MDB, '-nooptions', '-nogoifmain',
               '-toggle=include_local_symbols=1',
               '-nsim', TEST_BOARD_NSIM_ARGS,
               '-OKN', RC_KERNEL_ELF
              ]
        },

        ]

TEST_MULTICORE_CASES = [['--jtag=digilent', '--cores=2']]
TEST_CORE1 = [TEST_MDB, '-pset=1', '-psetname=core0', '', '-nooptions',
              '-nogoifmain', '-toggle=include_local_symbols=1',
              '-digilent', '', RC_KERNEL_ELF]
TEST_CORE2 = [TEST_MDB, '-pset=2', '-psetname=core1', '-prop=download=2',
              '-nooptions', '-nogoifmain',
              '-toggle=include_local_symbols=1',
              '-digilent', '', RC_KERNEL_ELF]
TEST_CORES = [TEST_MDB, '-multifiles=core0,core1', '-run',
              '-cmd=-nowaitq run', '-cmd=quit', '-cl']
#
# Fixtures
#

@pytest.fixture
def mdb(runner_config, tmpdir):
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
            return MdbBinaryRunner(runner_config, TEST_TARGET, **args)
        elif isinstance(args, list):
            parser = argparse.ArgumentParser()
            MdbBinaryRunner.add_parser(parser)
            arg_namespace = parser.parse_args(args)
            return MdbBinaryRunner.create(runner_config, arg_namespace)
    return _factory

#
# Helpers
#

def require_patch(program):
    assert program in ['mdb', TEST_MDB]

#
# Test cases for runners created by constructor.
#

@pytest.mark.parametrize('test_case', TEST_FLASH_CASES)
@patch('runners.mdb.MdbBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_flash(require, cc, test_case, mdb):
    mdb(test_case['i']).run('flash')
    assert require.called
    cc.assert_called_once_with(test_case['o'])


@pytest.mark.parametrize('test_case', TEST_DEBUG_CASES)
@patch('runners.mdb.MdbBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debug(require, cc, test_case, mdb):
    mdb(test_case['i']).run('debug')
    assert require.called
    cc.assert_called_once_with(test_case['o'])


@pytest.mark.parametrize('test_case', TEST_MULTICORE_CASES)
@patch('runners.mdb.MdbBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_multicores(require, cc, test_case, mdb):
    mdb(test_case).run('flash')
    assert require.called
    cc_calls = [call(TEST_CORE1), call(TEST_CORE2), call(TEST_CORES)]
    cc.assert_has_calls(cc_calls)
