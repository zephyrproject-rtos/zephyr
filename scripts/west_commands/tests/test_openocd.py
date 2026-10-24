# Copyright (c) 2026 Andrii Anoshyn
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from unittest.mock import MagicMock, patch

import pytest
from conftest import RC_GDB, RC_KERNEL_ELF, RC_OPENOCD

from runners.core import FileType
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


#
# --gdb-init commands are appended, in the order given, after the ones the
# runner issues itself, so they cannot change the runner's default behavior.
# Twenty-two in-tree board.cmake files rely on this.
#

INIT_A = 'set print asm-demangle on'
INIT_B = 'mem 0x90000000 0x90020000 ro'
INIT_ARGS = ['--gdb-init', INIT_A, '--gdb-init', INIT_B]
INIT_EX = ['-ex', INIT_A, '-ex', INIT_B]
LOAD = ['-ex', 'load']
PRE_DEBUG = 'monitor reset halt'


@pytest.mark.parametrize(
    'command, argv, expected_tail',
    [
        # 'debug' loads the image first, so the user's commands come last.
        ('debug', INIT_ARGS, LOAD + INIT_EX),
        # Without the option the client gets no extra commands.
        ('debug', [], LOAD),
        # 'attach' loads nothing.
        ('attach', INIT_ARGS, INIT_EX),
        # --gdb-pre-debug is the other append point, and keeps its place.
        ('debug', ['--gdb-pre-debug', PRE_DEBUG, *INIT_ARGS], LOAD + ['-ex', PRE_DEBUG] + INIT_EX),
        # --no-load drops the load command.
        ('debug', ['--no-load', *INIT_ARGS], INIT_EX),
    ],
)
@patch('runners.openocd.OpenOcdBinaryRunner.read_version', return_value=(0, 12, 0))
@patch('runners.openocd.OpenOcdBinaryRunner.check_call_ignore_sigint')
@patch('runners.openocd.OpenOcdBinaryRunner.popen_ignore_int', return_value=MagicMock())
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_gdb_init(
    require,
    popen_ignore_int,
    check_call_ignore_sigint,
    read_version,
    command,
    argv,
    expected_tail,
    runner_config,
):
    runner = create_from_args(runner_config, argv)
    # Inject a fake build configuration so no real build dir is read.
    runner._build_conf = FakeBuildConf({'CONFIG_DEBUG_THREAD_INFO': True})
    runner.run(command)

    cmd = check_call_ignore_sigint.call_args[0][0]
    assert cmd[-len(expected_tail) :] == expected_tail


#
# 'west load' writes the ELF into RAM and resumes at its entry point instead
# of resetting, which would run whatever the reset vector points at. The
# same sequence serves 'west flash --file-type=elf'.
#

ENTRY = 0x80000000
RESUME = ['-c', f'resume 0x{ENTRY:016x}', '-c', 'shutdown']
RESET_INIT = ['-c', 'reset init']
LOAD_IMAGE = ['-c', f'load_image {RC_KERNEL_ELF}']
VERIFY_IMAGE = ['-c', f'verify_image {RC_KERNEL_ELF}']


def create_with_config(cfg, argv):
    parser = argparse.ArgumentParser(allow_abbrev=False)
    OpenOcdBinaryRunner.add_parser(parser)
    return OpenOcdBinaryRunner.create(cfg, parser.parse_args(argv))


def openocd_tail(cmd):
    '''The arguments after 'targets', i.e. what the command adds.'''
    return cmd[cmd.index('-c targets') + 1 :]


@pytest.mark.parametrize(
    'argv, expected_tail',
    [
        ([], RESET_INIT + LOAD_IMAGE + RESUME),
        # --cmd-reset-halt replaces the reset, e.g. with a plain halt.
        (['--cmd-reset-halt', 'halt'], ['-c', 'halt'] + LOAD_IMAGE + RESUME),
        (['--cmd-pre-load', 'x'], ['-c', 'x'] + RESET_INIT + LOAD_IMAGE + RESUME),
        (['--verify'], RESET_INIT + LOAD_IMAGE + VERIFY_IMAGE + RESUME),
        (['--verify-only'], VERIFY_IMAGE + RESUME),
        (
            ['--verify', '--cmd-post-verify', 'y'],
            RESET_INIT + LOAD_IMAGE + VERIFY_IMAGE + ['-c', 'y'] + RESUME,
        ),
    ],
)
@patch('runners.openocd.elf_entry_point', return_value=ENTRY)
@patch('runners.openocd.OpenOcdBinaryRunner.ensure_output')
@patch('runners.openocd.OpenOcdBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_load(
    require, check_call, ensure_output, elf_entry_point, argv, expected_tail, runner_config
):
    runner = create_with_config(runner_config._replace(file=None), argv)
    runner.run('load')

    assert openocd_tail(check_call.call_args[0][0]) == expected_tail
    elf_entry_point.assert_called_once_with(RC_KERNEL_ELF)


@patch('runners.openocd.elf_entry_point', return_value=ENTRY)
@patch('runners.openocd.OpenOcdBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_load_file_type_elf(require, check_call, elf_entry_point, runner_config):
    '''--file-type=elf makes run_common pass the ELF as cfg.file.'''
    cfg = runner_config._replace(file=RC_KERNEL_ELF, file_type=FileType.ELF)
    create_with_config(cfg, []).run('load')

    assert openocd_tail(check_call.call_args[0][0]) == RESET_INIT + LOAD_IMAGE + RESUME


@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_load_rejects_non_elf(require, runner_config):
    cfg = runner_config._replace(file='test-zephyr.hex', file_type=FileType.HEX)
    with pytest.raises(ValueError, match='only an ELF'):
        create_with_config(cfg, []).run('load')


@patch('runners.openocd.elf_entry_point', return_value=ENTRY)
@patch('runners.openocd.OpenOcdBinaryRunner.check_call')
@patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch)
def test_flash_elf_matches_load(require, check_call, elf_entry_point, runner_config):
    '''Flashing an ELF is the same RAM load sequence as 'load'.'''
    cfg = runner_config._replace(file=RC_KERNEL_ELF, file_type=FileType.ELF)
    create_with_config(cfg, []).run('flash')

    assert openocd_tail(check_call.call_args[0][0]) == RESET_INIT + LOAD_IMAGE + RESUME


def test_load_capability():
    assert 'load' in OpenOcdBinaryRunner.capabilities().commands
