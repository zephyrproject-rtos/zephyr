# Copyright (c) 2018 Foundries.io
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import functools
import io
import os
import shlex
import typing
from unittest.mock import patch, call

import pytest

from runners.nrfjprog import NrfJprogBinaryRunner
from runners.nrfutil import NrfUtilBinaryRunner
from conftest import RC_KERNEL_HEX


#
# Test values
#

TEST_DEF_SNR = 'test-default-serial-number'  # for mocking user input
TEST_OVR_SNR = 'test-override-serial-number'

TEST_TOOL_OPT = '--ip 192.168.1.10'
TEST_TOOL_OPT_L = shlex.split(TEST_TOOL_OPT)

CLASS_MAP = {'nrfjprog': NrfJprogBinaryRunner, 'nrfutil': NrfUtilBinaryRunner}

#
# A dictionary mapping test cases to expected results.
#
# The keys are TC objects.
#
# The values are usually tool commands we expect to be executed for
# each test case. Verification is done by mocking the check_call()
# ZephyrBinaryRunner method which is used to run the commands.
#
# Values can also be callables which take a tmpdir and return the
# expected commands. This is needed for nRF53 testing.
#

class TC(typing.NamedTuple):    # 'TestCase'
    # NRF51, NRF52, etc.
    family: str

    # 'APP', 'NET', 'APP+NET', or None.
    coprocessor: typing.Optional[str]

    # Run a recover command first if True
    recover: bool

    # Use --reset instead of --pinreset if True
    softreset: bool

    # --snr TEST_OVR_SNR if True, --snr TEST_DEF_SNR if False
    snr: bool

    # --chiperase if True,
    # --sectorerase if False (or --sectoranduicrerase on nRF52)
    erase: bool

    # --tool-opt TEST_TOOL_OPT if True
    tool_opt: bool

EXPECTED_MAP = {'nrfjprog': 0, 'nrfutil': 1}
EXPECTED_RESULTS = {

    # -------------------------------------------------------------------------
    # NRF51
    #
    #  family          CP    recov  soft   snr    erase  tool_opt
    TC('nrf51', None, False, False, False, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF51',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF51', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf51', None, False, False, False, True, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF51',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF51', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf51', None, False, False, True, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF51',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF51', '--snr', TEST_OVR_SNR]),
     (TEST_OVR_SNR, None)),

    TC('nrf51', None, False, True, False, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF51',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF51', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf51', None, True, False, False, False, False):
    ((['nrfjprog', '--recover', '-f', 'NRF51', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF51',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF51', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf51', None, True, True, True, True, False):
    ((['nrfjprog', '--recover', '-f', 'NRF51', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF51',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF51', '--snr', TEST_OVR_SNR]),
     (TEST_OVR_SNR, None)),

    TC('nrf51', None, True, True, True, True, True):
    ((['nrfjprog', '--recover', '-f', 'NRF51', '--snr', TEST_OVR_SNR] + TEST_TOOL_OPT_L,
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF51',
      '--snr', TEST_OVR_SNR] + TEST_TOOL_OPT_L,
     ['nrfjprog', '--reset', '-f', 'NRF51', '--snr', TEST_OVR_SNR] + TEST_TOOL_OPT_L),
     (TEST_OVR_SNR, None)),

    # -------------------------------------------------------------------------
    # NRF52
    #
    #  family          CP    recov  soft   snr    erase  tool_opt
    TC('nrf52', None, False, False, False, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectoranduicrerase',
      '--verify', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinresetenable', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF52', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf52', None, False, False, False, True, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF52',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinresetenable', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF52', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf52', None, False, False, True, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectoranduicrerase',
      '--verify', '-f', 'NRF52', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--pinresetenable', '-f', 'NRF52', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF52', '--snr', TEST_OVR_SNR]),
     (TEST_OVR_SNR, None)),

    TC('nrf52', None, False, True, False, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectoranduicrerase',
      '--verify', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF52', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf52', None, True, False, False, False, False):
    ((['nrfjprog', '--recover', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--sectoranduicrerase',
      '--verify', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinresetenable', '-f', 'NRF52', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF52', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf52', None, True, True, True, True, False):
    ((['nrfjprog', '--recover', '-f', 'NRF52', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF52',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF52', '--snr', TEST_OVR_SNR]),
     (TEST_OVR_SNR, None)),

    TC('nrf52', None, True, True, True, True, True):
    ((['nrfjprog', '--recover', '-f', 'NRF52', '--snr', TEST_OVR_SNR] + TEST_TOOL_OPT_L,
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF52',
      '--snr', TEST_OVR_SNR] + TEST_TOOL_OPT_L,
     ['nrfjprog', '--reset', '-f', 'NRF52', '--snr', TEST_OVR_SNR] + TEST_TOOL_OPT_L),
     (TEST_OVR_SNR, None)),

    # -------------------------------------------------------------------------
    # NRF53 APP
    #
    #  family          CP     recov  soft   snr    erase  tool_opt
    TC('nrf53', 'APP', False, False, False, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--coprocessor', 'CP_APPLICATION', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf53', 'APP', False, False, False, True, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase',
      '--verify', '-f', 'NRF53', '--coprocessor', 'CP_APPLICATION', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf53', 'APP', False, False, True, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--coprocessor', 'CP_APPLICATION', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_OVR_SNR]),
     (TEST_OVR_SNR, None)),

    TC('nrf53', 'APP', False, True, False, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--coprocessor', 'CP_APPLICATION', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf53', 'APP', True, False, False, False, False):
    ((['nrfjprog', '--recover', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--recover', '-f', 'NRF53', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--coprocessor', 'CP_APPLICATION', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf53', 'APP', True, True, True, True, False):
    ((['nrfjprog', '--recover', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--recover', '-f', 'NRF53', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase',
      '--verify', '-f', 'NRF53', '--coprocessor', 'CP_APPLICATION', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF53', '--snr', TEST_OVR_SNR]),
     (TEST_OVR_SNR, None)),

    # -------------------------------------------------------------------------
    # NRF53 NET
    #
    #  family          CP     recov  soft   snr    erase  tool_opt
    TC('nrf53', 'NET', False, False, False, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf53', 'NET', False, False, False, True, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase',
      '--verify', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf53', 'NET', False, False, True, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_OVR_SNR]),
     (TEST_OVR_SNR, None)),

    TC('nrf53', 'NET', False, True, False, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf53', 'NET', True, False, False, False, False):
    ((['nrfjprog', '--recover', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--recover', '-f', 'NRF53', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase',
      '--verify', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF53', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf53', 'NET', True, True, True, True, False):
    ((['nrfjprog', '--recover', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--recover', '-f', 'NRF53', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase',
      '--verify', '-f', 'NRF53', '--coprocessor', 'CP_NETWORK', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF53', '--snr', TEST_OVR_SNR]),
     (TEST_OVR_SNR, None)),

    # -------------------------------------------------------------------------
    # NRF91
    #
    #  family          CP    recov  soft   snr    erase  tool_opt
    TC('nrf91', None, False, False, False, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF91',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF91', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf91', None, False, False, False, True, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF91',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF91', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf91', None, False, False, True, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF91',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF91', '--snr', TEST_OVR_SNR]),
     (TEST_OVR_SNR, None)),

    TC('nrf91', None, False, True, False, False, False):
    ((['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF91',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF91', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf91', None, True, False, False, False, False):
    ((['nrfjprog', '--recover', '-f', 'NRF91', '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--sectorerase', '--verify', '-f', 'NRF91',
      '--snr', TEST_DEF_SNR],
     ['nrfjprog', '--pinreset', '-f', 'NRF91', '--snr', TEST_DEF_SNR]),
     (TEST_DEF_SNR, None)),

    TC('nrf91', None, True, True, True, True, False):
    ((['nrfjprog', '--recover', '-f', 'NRF91', '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF91',
      '--snr', TEST_OVR_SNR],
     ['nrfjprog', '--reset', '-f', 'NRF91', '--snr', TEST_OVR_SNR]),
     (TEST_OVR_SNR, None)),

    TC('nrf91', None, True, True, True, True, True):
    ((['nrfjprog', '--recover', '-f', 'NRF91', '--snr', TEST_OVR_SNR] + TEST_TOOL_OPT_L,
     ['nrfjprog', '--program', RC_KERNEL_HEX, '--chiperase', '--verify', '-f', 'NRF91',
      '--snr', TEST_OVR_SNR] + TEST_TOOL_OPT_L,
     ['nrfjprog', '--reset', '-f', 'NRF91', '--snr', TEST_OVR_SNR] + TEST_TOOL_OPT_L),
     (TEST_OVR_SNR, None)),
}

#
# Monkey-patches
#

def get_board_snr_patch(glob):
    return TEST_DEF_SNR

def require_patch(cur_tool, program):
    assert cur_tool == program

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
        cp = f'-{test_case.coprocessor}'
    s = 'soft_reset' if test_case.softreset else 'pin_reset'
    sn = 'default_snr' if test_case.snr else 'override_snr'
    e = 'chip_erase' if test_case.erase else 'sector[anduicr]_erase'
    r = 'recover' if test_case.recover else 'no_recover'
    t = 'tool_opt' if test_case.tool_opt else 'no_tool_opt'

    return f'{test_case.family[:5]}{cp}-{s}-{sn}-{e}-{r}-{t}'

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
CONFIG_SOC_SERIES_{test_case.family.upper()}X=y
''')
        if test_case.family == 'nrf53':
            f.write(f'''
CONFIG_SOC_NRF5340_CPU{test_case.coprocessor}=y
''')

    to_replace['build_dir'] = tmpdir

    return runner_config._replace(**to_replace)

def check_expected(tool, test_case, check_fn, get_snr, tmpdir, runner_config):

    expected = EXPECTED_RESULTS[test_case][EXPECTED_MAP[tool]]
    if tool == 'nrfutil':
        # Skip the preparation calls for now
        assert len(check_fn.call_args_list) >= 1
        xi = len(check_fn.call_args_list) - 1
        assert len(check_fn.call_args_list[xi].args) == 1
        # Extract filename
        nrfutil_args = check_fn.call_args_list[xi].args[0]
        tmpfile = nrfutil_args[nrfutil_args.index('--batch-path') + 1]
        cmds = (['nrfutil', '--json', 'device', 'x-execute-batch', '--batch-path',
                 tmpfile, '--serial-number', expected[0]],)
        call_args = [call(nrfutil_args)]
    else:
        cmds = expected
        call_args = check_fn.call_args_list

    if callable(cmds):
        assert (call_args ==
                [call(x) for x in cmds(tmpdir, runner_config.hex_file)])
    else:
        assert call_args == [call(x) for x in cmds]

    if not test_case.snr:
        get_snr.assert_called_once_with('*')
    else:
        get_snr.assert_not_called()

@pytest.mark.parametrize('tool', ["nrfjprog","nrfutil"])
@pytest.mark.parametrize('test_case', EXPECTED_RESULTS.keys(), ids=id_fn)
@patch('runners.core.ZephyrBinaryRunner.require')
@patch('runners.nrfjprog.NrfBinaryRunner.get_board_snr',
       side_effect=get_board_snr_patch)
@patch('runners.nrfutil.subprocess.Popen')
@patch('runners.nrfjprog.NrfBinaryRunner.check_call')
def test_init(check_call, popen, get_snr, require, tool, test_case,
              runner_config, tmpdir):
    popen.return_value.__enter__.return_value.stdout = io.BytesIO(b'')

    require.side_effect = functools.partial(require_patch, tool)
    runner_config = fix_up_runner_config(test_case, runner_config, tmpdir)
    snr = TEST_OVR_SNR if test_case.snr else None
    tool_opt = TEST_TOOL_OPT_L if test_case.tool_opt else []
    cls = CLASS_MAP[tool]
    runner = cls(runner_config,
                 test_case.family,
                 test_case.softreset,
                 snr,
                 erase=test_case.erase,
                 tool_opt=tool_opt,
                 recover=test_case.recover)

    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')
    assert require.called

    CHECK_FN_MAP = {'nrfjprog': check_call, 'nrfutil': popen}
    check_expected(tool, test_case, CHECK_FN_MAP[tool], get_snr, tmpdir,
                   runner_config)

@pytest.mark.parametrize('tool', ["nrfjprog","nrfutil"])
@pytest.mark.parametrize('test_case', EXPECTED_RESULTS.keys(), ids=id_fn)
@patch('runners.core.ZephyrBinaryRunner.require')
@patch('runners.nrfjprog.NrfBinaryRunner.get_board_snr',
       side_effect=get_board_snr_patch)
@patch('runners.nrfutil.subprocess.Popen')
@patch('runners.nrfjprog.NrfBinaryRunner.check_call')
def test_create(check_call, popen, get_snr, require, tool, test_case,
                runner_config, tmpdir):
    popen.return_value.__enter__.return_value.stdout = io.BytesIO(b'')

    require.side_effect = functools.partial(require_patch, tool)
    runner_config = fix_up_runner_config(test_case, runner_config, tmpdir)

    args = []
    if test_case.softreset:
        args.append('--softreset')
    if test_case.snr:
        args.extend(['--dev-id', TEST_OVR_SNR])
    if test_case.erase:
        args.append('--erase')
    if test_case.recover:
        args.append('--recover')
    if test_case.tool_opt:
        args.extend(['--tool-opt', TEST_TOOL_OPT])

    parser = argparse.ArgumentParser(allow_abbrev=False)
    cls = CLASS_MAP[tool]
    cls.add_parser(parser)
    arg_namespace = parser.parse_args(args)
    runner = cls.create(runner_config, arg_namespace)
    with patch('os.path.isfile', side_effect=os_path_isfile_patch):
        runner.run('flash')

    assert require.called

    CHECK_FN_MAP = {'nrfjprog': check_call, 'nrfutil': popen}
    check_expected(tool, test_case, CHECK_FN_MAP[tool], get_snr, tmpdir,
                   runner_config)
