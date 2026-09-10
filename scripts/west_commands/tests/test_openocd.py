# Copyright (c) 2026 Andrii Anoshyn
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from unittest.mock import MagicMock, patch

import pytest
from conftest import RC_GDB, RC_OPENOCD

from runners.openocd import OpenOcdBinaryRunner

TEST_SERIAL = 'E7038EAD'
SERIAL_CMD = '-c set _ZEPHYR_BOARD_SERIAL ' + TEST_SERIAL

#
# OpenOCD's '-rtos Zephyr' target awareness is only implemented for the
# cortex_m, cortex_r4, hla_target and arcv2 architectures. Appending the
# command for any other architecture makes OpenOCD abort, which 'west debug'
# surfaces as a silent timeout. The runner therefore gates the command on the
# build's architecture in addition to the OpenOCD version (see issue #108804).
#

RTOS_COMMAND = '$_TARGETNAME configure -rtos Zephyr'

# (CONFIG symbols that are 'y', architecture is RTOS-aware?)
ARCH_CASES = [
    ({'CONFIG_CPU_CORTEX_M': True}, True),
    ({'CONFIG_CPU_AARCH32_CORTEX_R': True}, True),
    ({'CONFIG_ISA_ARCV2': True}, True),
    # RISC-V / Xtensa and friends: none of the gating symbols are set.
    ({}, False),
]


class FakeBuildConf:
    '''Minimal stand-in for runners.core.BuildConfiguration.'''

    def __init__(self, options):
        self.options = options

    def getboolean(self, option):
        return self.options.get(option, False)


def require_patch(program):
    assert program in [RC_OPENOCD, RC_GDB]


def openocd_cmd(command, check_call, popen_ignore_int):
    if command == 'debugserver':
        return check_call.call_args[0][0]

    return popen_ignore_int.call_args[0][0]


@pytest.fixture
def openocd(runner_config):
    def _factory(enabled_configs):
        # The shared fixture leaves a non-None cfg.file (an artifact of its
        # positional construction); force it to None so the OpenOCD runner,
        # which builds a Path() from it, constructs cleanly.
        runner = OpenOcdBinaryRunner(runner_config._replace(file=None))
        # Inject a fake build configuration so no real build dir is read.
        # CONFIG_DEBUG_THREAD_INFO gates whether thread info is requested at
        # all; the architecture symbols gate whether the arch supports it.
        configs = {'CONFIG_DEBUG_THREAD_INFO': True, **enabled_configs}
        runner._build_conf = FakeBuildConf(configs)
        return runner

    return _factory


@pytest.mark.parametrize('command', ['debugserver', 'debug'])
@pytest.mark.parametrize('enabled_configs, rtos_supported', ARCH_CASES)
@patch('runners.openocd.OpenOcdBinaryRunner.read_version', return_value=(0, 12, 0))
@patch('runners.openocd.OpenOcdBinaryRunner.check_call_ignore_sigint')
@patch('runners.openocd.OpenOcdBinaryRunner.popen_ignore_int', return_value=MagicMock())
@patch('runners.openocd.OpenOcdBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debugserver_rtos_arch_gate(
    require,
    check_call,
    popen_ignore_int,
    check_call_ignore_sigint,
    read_version,
    command,
    enabled_configs,
    rtos_supported,
    openocd,
):
    '''The '-rtos Zephyr' command is only emitted for RTOS-aware archs.'''
    openocd(enabled_configs).run(command)

    cmd = openocd_cmd(command, check_call, popen_ignore_int)
    assert (RTOS_COMMAND in cmd) == rtos_supported


@pytest.mark.parametrize('command', ['debugserver', 'debug'])
@patch('runners.openocd.OpenOcdBinaryRunner.read_version', return_value=(0, 11, 0))
@patch('runners.openocd.OpenOcdBinaryRunner.check_call_ignore_sigint')
@patch('runners.openocd.OpenOcdBinaryRunner.popen_ignore_int', return_value=MagicMock())
@patch('runners.openocd.OpenOcdBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_debugserver_rtos_old_openocd(
    require, check_call, popen_ignore_int, check_call_ignore_sigint, read_version, command, openocd
):
    '''An OpenOCD too old for Zephyr awareness never gets the command, even
    on an otherwise supported architecture.'''
    openocd({'CONFIG_CPU_CORTEX_M': True}).run(command)

    cmd = openocd_cmd(command, check_call, popen_ignore_int)
    assert RTOS_COMMAND not in cmd


def create_from_args(runner_config, argv):
    '''Build an OpenOcdBinaryRunner from command-line arguments.'''
    parser = argparse.ArgumentParser(allow_abbrev=False)
    OpenOcdBinaryRunner.add_parser(parser)
    args = parser.parse_args(argv)
    # The shared fixture leaves a non-None cfg.file (an artifact of its
    # positional construction); force it to None so the runner, which builds
    # a Path() from it, constructs cleanly.
    return OpenOcdBinaryRunner.create(runner_config._replace(file=None), args)


@pytest.mark.parametrize(
    'argv',
    [
        ['-i', TEST_SERIAL],  # canonical selector
        ['--dev-id', TEST_SERIAL],  # long form
        ['--serial', TEST_SERIAL],  # deprecated alias
    ],
)
def test_dev_id_selects_serial(argv, runner_config):
    '''-i/--dev-id and the deprecated --serial alias all set the adapter
    serial, which is passed to OpenOCD as _ZEPHYR_BOARD_SERIAL.'''
    runner = create_from_args(runner_config, argv)
    assert runner.serial == [SERIAL_CMD]


def test_dev_id_overrides_serial(runner_config):
    '''Both flags land in the same destination, so argparse precedence
    applies and the last one given wins.'''
    runner = create_from_args(runner_config, ['--serial', 'OLD', '-i', TEST_SERIAL])
    assert runner.serial == [SERIAL_CMD]


def test_no_dev_id_no_serial(runner_config):
    '''Without a selector no _ZEPHYR_BOARD_SERIAL command is emitted.'''
    runner = create_from_args(runner_config, [])
    assert runner.serial == []
