# Copyright (c) 2018 Foundries.io
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
from pathlib import Path
import shutil
import typing
from unittest.mock import patch, call

import pytest

from runners.nrfjprog import NrfJprogBinaryRunner
from conftest import RC_KERNEL_HEX


#
# Test values
#

TEST_DEF_SNR = 'test-default-serial-number'  # for mocking user input
TEST_OVR_SNR = 'test-override-serial-number'

# nRF53 flashing is special in that we have different results
# depending on the input hex file. For that reason, we test it with
# real hex files.
TEST_DIR = Path(__file__).parent / 'nrfjprog'
NRF5340_APP_ONLY_HEX = os.fspath(TEST_DIR / 'nrf5340_app_only.hex')
NRF5340_NET_ONLY_HEX = os.fspath(TEST_DIR / 'nrf5340_net_only.hex')
NRF5340_APP_AND_NET_HEX = os.fspath(TEST_DIR / 'nrf5340_app_and_net.hex')

#
# A dictionary mapping test cases to expected results.
#
# The keys are TC objects.
#
# The values are usually nrfjprog commands we expect to be executed for
# each test case. Verification is done by mocking the check_call()
# ZephyrBinaryRunner method which is used to run the commands.
#
# Values can also be callables which take a tmpdir and return the
# expected nrfjprog commands. This is needed for nRF53 testing.
#

class TC(typing.NamedTuple):    # 'TestCase'
    # NRF51, NRF52, etc.
    family: str

    # 'APP', 'NET', 'APP+NET', or None.
    coprocessor: typing.Optional[str]

    # Run 'nrfjprog --recover' first if True
    recover: bool

    # Use --reset instead of --pinreset if True
    softreset: bool

    # --snr TEST_OVR_SNR if True, --snr TEST_DEF_SNR if False
    snr: bool

    # --chiperase if True,
    # --sectorerase if False (or --sectoranduicrerase on nRF52)
    erase: bool


EXPECTED_RESULTS = {

    # -------------------------------------------------------------------------
    # NRF51
    #
    #  family   CP    recov  soft   snr    erase
    TC('NRF51', None, False, False, False, False):
    (['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF51',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF51', '--snr', TEST_DEF_SNR]),

    TC('NRF51', None, False, False, False, True):
    (['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF51',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF51', '--snr', TEST_DEF_SNR]),

    TC('NRF51', None, False, False, True, False):
    (['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF51',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF51', '--snr', TEST_OVR_SNR]),

    TC('NRF51', None, False, True, False, False):
    (['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF51',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF51', '--snr', TEST_DEF_SNR]),

    TC('NRF51', None, True, False, False, False):
    (['nrfjprog', '--recover', '-f', 'NRF51', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF51',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF51', '--snr', TEST_DEF_SNR]),

    TC('NRF51', None, True, True, True, True):
    (['nrfjprog', '--recover', '-f', 'NRF51', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF51',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF51', '--snr', TEST_OVR_SNR]),

    # -------------------------------------------------------------------------
    # NRF52
    #
    #  family   CP    recov  soft   snr    erase
    TC('NRF52', None, False, False, False, False):
    (['nrfjprog', '--program', RC_KERNEL_HEX, '--sectoranduicrerase',
      '--verify', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinresetenable', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF52', '--snr', TEST_DEF_SNR]),

    TC('NRF52', None, False, False, False, True):
    (['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF52',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinresetenable', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF52', '--snr', TEST_DEF_SNR]),

    TC('NRF52', None, False, False, True, False):
    (['nrfjprog', '--program', RC_KERNEL_HEX, '--sectoranduicrerase',
      '--verify', '-f', 'NRF52', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--pinresetenable', '-f', 'NRF52', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF52', '--snr', TEST_OVR_SNR]),

    TC('NRF52', None, False, True, False, False):
    (['nrfjprog', '--program', RC_KERNEL_HEX, '--sectoranduicrerase',
      '--verify', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF52', '--snr', TEST_DEF_SNR]),

    TC('NRF52', None, True, False, False, False):
    (['nrfjprog', '--recover', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--sectoranduicrerase',
      '--verify', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinresetenable', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF52', '--snr', TEST_DEF_SNR]),

    TC('NRF52', None, True, True, True, True):
    (['nrfjprog', '--recover', '-f', 'NRF52', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF52',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF52', '--snr', TEST_OVR_SNR]),

    # -------------------------------------------------------------------------
    # NRF53 APP only
    #
    #  family   CP     recov  soft   snr    erase

    TC('NRF53', 'APP', False, False, False, False):
    (['nrfjprog', '--program', NRF5340_APP_ONLY_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR, '--coprocessor', 'CP_APPLICATION'],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),

    TC('NRF53', 'APP', False, False, False, True):
    (['nrfjprog', '--program', NRF5340_APP_ONLY_HEX, '--chiperase',
      '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR, '--coprocessor', 'CP_APPLICATION'],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),

    TC('NRF53', 'APP', False, False, True, False):
    (['nrfjprog', '--program', NRF5340_APP_ONLY_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--snr', TEST_OVR_SNR, '--coprocessor', 'CP_APPLICATION'],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_OVR_SNR]),

    TC('NRF53', 'APP', False, True, False, False):
    (['nrfjprog', '--program', NRF5340_APP_ONLY_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR, '--coprocessor', 'CP_APPLICATION'],
     ['nrfjprog', '--reset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),

    TC('NRF53', 'APP', True, False, False, False):
    (['nrfjprog', '--recover', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--recover', '-f', 'NRF53', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--program', NRF5340_APP_ONLY_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR, '--coprocessor', 'CP_APPLICATION'],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),

    TC('NRF53', 'APP', True, True, True, True):
    (['nrfjprog', '--recover', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--recover', '-f', 'NRF53', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--program', NRF5340_APP_ONLY_HEX, '--chiperase',
      '--verify', '-f', 'NRF53', '--snr', TEST_OVR_SNR, '--coprocessor', 'CP_APPLICATION'],
     ['nrfjprog', '--reset', '-f', 'NRF53', '--snr', TEST_OVR_SNR]),

    # -------------------------------------------------------------------------
    # NRF53 NET only
    #
    #  family   CP     recov  soft   snr    erase

    TC('NRF53', 'NET', False, False, False, False):
    (['nrfjprog', '--program', NRF5340_NET_ONLY_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR, '--coprocessor', 'CP_NETWORK'],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),

    TC('NRF53', 'NET', False, False, False, True):
    (['nrfjprog', '--program', NRF5340_NET_ONLY_HEX, '--chiperase',
      '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR, '--coprocessor', 'CP_NETWORK'],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),

    TC('NRF53', 'NET', False, False, True, False):
    (['nrfjprog', '--program', NRF5340_NET_ONLY_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--snr', TEST_OVR_SNR, '--coprocessor', 'CP_NETWORK'],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_OVR_SNR]),

    TC('NRF53', 'NET', False, True, False, False):
    (['nrfjprog', '--program', NRF5340_NET_ONLY_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR, '--coprocessor', 'CP_NETWORK'],
     ['nrfjprog', '--reset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),

    TC('NRF53', 'NET', True, False, False, False):
    (['nrfjprog', '--recover', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--recover', '-f', 'NRF53', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--program', NRF5340_NET_ONLY_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR, '--coprocessor', 'CP_NETWORK'],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),

    TC('NRF53', 'NET', True, True, True, True):
    (['nrfjprog', '--recover', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--recover', '-f', 'NRF53', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--program', NRF5340_NET_ONLY_HEX, '--chiperase',
      '--verify', '-f', 'NRF53', '--snr', TEST_OVR_SNR, '--coprocessor', 'CP_NETWORK'],
     ['nrfjprog', '--reset', '-f', 'NRF53', '--snr', TEST_OVR_SNR]),

    # -------------------------------------------------------------------------
    # NRF53 APP+NET
    #
    #  family   CP     recov  soft   snr    erase

    TC('NRF53', 'APP+NET', False, False, False, False):
    (lambda tmpdir, infile: \
        (['nrfjprog',
          '--program',
          os.fspath(tmpdir / 'GENERATED_CP_NETWORK_' + Path(infile).name),
          '--sectorerase', '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR,
          '--coprocessor', 'CP_NETWORK'],
         ['nrfjprog',
          '--program',
          os.fspath(tmpdir / 'GENERATED_CP_APPLICATION_' + Path(infile).name),
          '--sectorerase', '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR,
          '--coprocessor', 'CP_APPLICATION'],
         ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR])),

    TC('NRF53', 'APP+NET', False, False, False, True):
    (lambda tmpdir, infile: \
        (['nrfjprog',
          '--program',
          os.fspath(tmpdir / 'GENERATED_CP_NETWORK_' + Path(infile).name),
          '--chiperase', '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR,
          '--coprocessor', 'CP_NETWORK'],
         ['nrfjprog',
          '--program',
          os.fspath(tmpdir / 'GENERATED_CP_APPLICATION_' + Path(infile).name),
          '--chiperase', '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR,
          '--coprocessor', 'CP_APPLICATION'],
         ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR])),

    TC('NRF53', 'APP+NET', False, False, True, False):
    (lambda tmpdir, infile: \
        (['nrfjprog',
          '--program',
          os.fspath(tmpdir / 'GENERATED_CP_NETWORK_' + Path(infile).name),
          '--sectorerase', '--verify', '-f', 'NRF53', '--snr', TEST_OVR_SNR,
          '--coprocessor', 'CP_NETWORK'],
         ['nrfjprog',
          '--program',
          os.fspath(tmpdir / 'GENERATED_CP_APPLICATION_' + Path(infile).name),
          '--sectorerase', '--verify', '-f', 'NRF53', '--snr', TEST_OVR_SNR,
          '--coprocessor', 'CP_APPLICATION'],
         ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_OVR_SNR])),

    TC('NRF53', 'APP+NET', False, True, False, False):
    (lambda tmpdir, infile: \
        (['nrfjprog',
          '--program',
          os.fspath(tmpdir / 'GENERATED_CP_NETWORK_' + Path(infile).name),
          '--sectorerase', '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR,
          '--coprocessor', 'CP_NETWORK'],
         ['nrfjprog',
          '--program',
          os.fspath(tmpdir / 'GENERATED_CP_APPLICATION_' + Path(infile).name),
          '--sectorerase', '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR,
          '--coprocessor', 'CP_APPLICATION'],
         ['nrfjprog', '--reset', '-f', 'NRF53', '--snr', TEST_DEF_SNR])),

    TC('NRF53', 'APP+NET', True, False, False, False):
    (lambda tmpdir, infile: \
        (['nrfjprog', '--recover', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK',
          '--snr', TEST_DEF_SNR],
         ['nrfjprog', '--recover', '-f', 'NRF53', '--snr', TEST_DEF_SNR],
         ['nrfjprog',
          '--program',
          os.fspath(tmpdir / 'GENERATED_CP_NETWORK_' + Path(infile).name),
          '--sectorerase', '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR,
          '--coprocessor', 'CP_NETWORK'],
         ['nrfjprog',
          '--program',
          os.fspath(tmpdir / 'GENERATED_CP_APPLICATION_' + Path(infile).name),
          '--sectorerase', '--verify', '-f', 'NRF53', '--snr', TEST_DEF_SNR,
          '--coprocessor', 'CP_APPLICATION'],
         ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR])),

    TC('NRF53', 'APP+NET', True, True, True, True):
    (lambda tmpdir, infile: \
        (['nrfjprog', '--recover', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK',
          '--snr', TEST_OVR_SNR],
         ['nrfjprog', '--recover', '-f', 'NRF53', '--snr', TEST_OVR_SNR],
         ['nrfjprog',
          '--program',
          os.fspath(tmpdir / 'GENERATED_CP_NETWORK_' + Path(infile).name),
          '--chiperase', '--verify', '-f', 'NRF53', '--snr', TEST_OVR_SNR,
          '--coprocessor', 'CP_NETWORK'],
         ['nrfjprog',
          '--program',
          os.fspath(tmpdir / 'GENERATED_CP_APPLICATION_' + Path(infile).name),
          '--chiperase', '--verify', '-f', 'NRF53', '--snr', TEST_OVR_SNR,
          '--coprocessor', 'CP_APPLICATION'],
         ['nrfjprog', '--reset', '-f', 'NRF53', '--snr', TEST_OVR_SNR])),

    # -------------------------------------------------------------------------
    # NRF91
    #
    #  family   CP    recov  soft   snr    erase
    TC('NRF91', None, False, False, False, False):
    (['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF91',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF91', '--snr', TEST_DEF_SNR]),

    TC('NRF91', None, False, False, False, True):
    (['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF91',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF91', '--snr', TEST_DEF_SNR]),

    TC('NRF91', None, False, False, True, False):
    (['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF91',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF91', '--snr', TEST_OVR_SNR]),

    TC('NRF91', None, False, True, False, False):
    (['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF91',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF91', '--snr', TEST_DEF_SNR]),

    TC('NRF91', None, True, False, False, False):
    (['nrfjprog', '--recover', '-f', 'NRF91', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF91',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF91', '--snr', TEST_DEF_SNR]),

    TC('NRF91', None, True, True, True, True):
    (['nrfjprog', '--recover', '-f', 'NRF91', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF91',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF91', '--snr', TEST_OVR_SNR]),
}

#
# Monkey-patches
#

def get_board_snr_patch(glob):
    return TEST_DEF_SNR

def require_patch(program):
    assert program == 'nrfjprog'

os_path_isfile = os.path.isfile

def os_path_isfile_patch(filename):
    if filename == RC_KERNEL_HEX:
        return True
    return os_path_isfile(filename)

#
# Test functions.
#
# These are white box tests that rely on the above monkey-patches.
#

def id_fn(test_case):
    if test_case.coprocessor is None:
        cp = ''
    else:
        cp = f', {test_case.coprocessor}'
    s = 'soft reset' if test_case.softreset else 'pin reset'
    sn = 'default snr' if test_case.snr else 'override snr'
    e = 'chip erase' if test_case.erase else 'sector[anduicr] erase'
    r = 'recover' if test_case.recover else 'no recover'

    return f'{test_case.family}{cp}, {s}, {sn}, {e}, {r}'

def fix_up_runner_config(test_case, runner_config, tmpdir):
    # Helper that adjusts the common runner_config fixture for our
    # nRF-specific tests.

    to_replace = {}

    # Provide a skeletal zephyr/.config file to use as the runner's
    # BuildConfiguration.
    zephyr = tmpdir / 'zephyr'
    zephyr.mkdir()
    dotconfig = os.fspath(zephyr / '.config')
    with open(dotconfig, 'w') as f:
        f.write(f'''
CONFIG_SOC_SERIES_{test_case.family}X=y
''')
    to_replace['build_dir'] = tmpdir

    if test_case.family != 'NRF53':
        return runner_config._replace(**to_replace)

    if test_case.coprocessor == 'APP':
        to_replace['hex_file'] = NRF5340_APP_ONLY_HEX
    elif test_case.coprocessor == 'NET':
        to_replace['hex_file'] = NRF5340_NET_ONLY_HEX
    elif test_case.coprocessor == 'APP+NET':
        # Since the runner is going to generate files next to its input
        # file, we need to stash a copy in a tmpdir it can use.
        outfile = tmpdir / Path(NRF5340_APP_AND_NET_HEX).name
        shutil.copyfile(NRF5340_APP_AND_NET_HEX, outfile)
        to_replace['hex_file'] = os.fspath(outfile)
    else:
        assert False, f'bad test case {test_case}'

    return runner_config._replace(**to_replace)

@pytest.mark.parametrize('test_case', EXPECTED_RESULTS.keys(), ids=id_fn)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.nrfjprog.NrfJprogBinaryRunner.get_board_snr',
       side_effect=get_board_snr_patch)
@patch('runners.nrfjprog.NrfJprogBinaryRunner.check_call')
def test_nrfjprog_init(check_call, get_snr, require, test_case,
                       runner_config, tmpdir):
    runner_config = fix_up_runner_config(test_case, runner_config, tmpdir)
    expected = EXPECTED_RESULTS[test_case]
    snr = TEST_OVR_SNR if test_case.snr else None
    runner = NrfJprogBinaryRunner(runner_config,
                                  test_case.family,
                                  test_case.softreset,
                                  snr,
                                  erase=test_case.erase,
                                  recover=test_case.recover)

    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')
    assert require.called

    if callable(expected):
        assert (check_call.call_args_list ==
                [call(x) for x in expected(tmpdir, runner_config.hex_file)])
    else:
        assert check_call.call_args_list == [call(x) for x in expected]

    if snr is None:
        get_snr.assert_called_once_with('*')
    else:
        get_snr.assert_not_called()

@pytest.mark.parametrize('test_case', EXPECTED_RESULTS.keys(), ids=id_fn)
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
@patch('runners.nrfjprog.NrfJprogBinaryRunner.get_board_snr',
       side_effect=get_board_snr_patch)
@patch('runners.nrfjprog.NrfJprogBinaryRunner.check_call')
def test_nrfjprog_create(check_call, get_snr, require, test_case,
                         runner_config, tmpdir):
    runner_config = fix_up_runner_config(test_case, runner_config, tmpdir)
    expected = EXPECTED_RESULTS[test_case]

    args = []
    if test_case.softreset:
        args.append('--softreset')
    if test_case.snr:
        args.extend(['--dev-id', TEST_OVR_SNR])
    if test_case.erase:
        args.append('--erase')
    if test_case.recover:
        args.append('--recover')

    parser = argparse.ArgumentParser(allow_abbrev=False)
    NrfJprogBinaryRunner.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = NrfJprogBinaryRunner.create(runner_config, arg_namespace)
    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')

    assert require.called
    if callable(expected):
        assert (check_call.call_args_list ==
                [call(x) for x in expected(tmpdir, runner_config.hex_file)])
    else:
        assert check_call.call_args_list == [call(x) for x in expected]

    if not test_case.snr:
        get_snr.assert_called_once_with('*')
    else:
        get_snr.assert_not_called()
