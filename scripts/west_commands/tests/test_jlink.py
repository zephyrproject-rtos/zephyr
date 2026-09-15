# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

'''Tests for the argv the J-Link runner hands to the GDB client.'''

import argparse
from dataclasses import replace
from types import SimpleNamespace
from unittest.mock import patch

import pytest
from conftest import RC_GDB, RC_KERNEL_ELF

from runners.jlink import JLinkBinaryRunner

#
# Test values. The J-Link version only matters because some argv contents are
# gated on it; this one is recent enough that nothing is gated out.
#

TEST_DEVICE = 'test-device'
TEST_JLINK_VERSION = (7, 88, 0)

#
# Expected results.
#
# The fragments the runner appends to the GDB client argv, in the order it
# appends them.
#

# The runner passes an empty string in place of '-batch' when batch mode is off.
CLIENT_HEAD = [RC_GDB, RC_KERNEL_ELF, '']
CLIENT_HEAD_BATCH = [RC_GDB, RC_KERNEL_ELF, '-batch']

CONNECT = ['-ex', 'target remote :2331']
CONNECT_HOST = ['-ex', 'target remote h:2331']
LOAD = ['-ex', 'monitor halt', '-ex', 'monitor reset', '-ex', 'load']
MONITOR_RESET = ['-ex', 'monitor reset']
GO_AND_QUIT = ['-ex', 'monitor go', '-ex', 'disconnect', '-ex', 'quit']
HALT_AND_CONFIRM = ['-ex', 'monitor halt', '-ex', 'monitor reset', '-ex', 'set confirm off']
RESET_SEQUENCE = HALT_AND_CONFIRM + GO_AND_QUIT

# Two --gdb-init commands, and the argv they must produce.
INIT_ARGS = ['--gdb-init', 'monitor semihosting enable', '--gdb-init', 'b main']
INIT_EX = ['-ex', 'monitor semihosting enable', '-ex', 'b main']


#
# Helpers
#


def require_patch(program, path=None):
    # The commander name is platform-dependent and the runner resolves it to an
    # absolute path, but neither reaches the GDB client argv checked here.
    return program


def create_from_args(runner_config, argv):
    parser = argparse.ArgumentParser(allow_abbrev=False)
    JLinkBinaryRunner.add_parser(parser)
    args = parser.parse_args(['--device', TEST_DEVICE, *argv])
    # The shared fixture leaves a non-None cfg.file; force it to None so the
    # runner does not mistake it for a user-provided image.
    return JLinkBinaryRunner.create(runner_config._replace(file=None), args)


#
# Fixtures
#


@pytest.fixture
def client_cmd(runner_config):
    '''Run a command and return the argv handed to the GDB client.'''

    def _run(command, argv=()):
        runner = create_from_args(runner_config, argv)
        # Bypass the version probe, which loads the J-Link shared library, and
        # the build directory read. Both are platform-specific, and neither
        # affects the client argv.
        runner._jlink_version = TEST_JLINK_VERSION
        runner._build_conf = SimpleNamespace(getboolean=lambda option: False)

        with (
            patch('runners.jlink.MISSING_REQUIREMENTS', False),
            patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch),
            patch.object(JLinkBinaryRunner, 'run_server_and_client') as with_server,
            patch.object(JLinkBinaryRunner, 'check_call_ignore_sigint') as client_only,
        ):
            runner.run(command)

        # --gdb-host connects to an existing server, so no server is started.
        return client_only.call_args[0][0] if runner.gdb_host else with_server.call_args[0][1]

    return _run


#
# Test cases
#
# --gdb-init commands are appended after the ones the runner issues itself and
# before the target is resumed, so they cannot change the runner's defaults and
# still take effect while the target is halted.
#


@pytest.mark.parametrize(
    'command, argv, expected',
    [
        # Interactive 'debug' hands the prompt to the user, so it must not tear
        # the session down.
        ('debug', [], CLIENT_HEAD + CONNECT + LOAD),
        ('debug', INIT_ARGS, CLIENT_HEAD + CONNECT + LOAD + INIT_EX),
        # Batch mode resumes the target and exits instead of waiting for input.
        ('debug', ['--batch'], CLIENT_HEAD_BATCH + CONNECT + LOAD + GO_AND_QUIT),
        (
            'debug',
            ['--batch', *INIT_ARGS],
            CLIENT_HEAD_BATCH + CONNECT + LOAD + INIT_EX + GO_AND_QUIT,
        ),
        # --reset restarts the target once the image is loaded. It leaves the
        # CPU halted, so the user's commands come after it, not before.
        ('debug', ['--reset'], CLIENT_HEAD + CONNECT + LOAD + MONITOR_RESET),
        ('debug', ['--reset', *INIT_ARGS], CLIENT_HEAD + CONNECT + LOAD + MONITOR_RESET + INIT_EX),
        # 'attach' leaves the target running and loads nothing.
        ('attach', [], CLIENT_HEAD + CONNECT),
        ('attach', INIT_ARGS, CLIENT_HEAD + CONNECT + INIT_EX),
        # --reset applies to 'attach' too, and the user's commands still come
        # after it.
        ('attach', ['--reset'], CLIENT_HEAD + CONNECT + MONITOR_RESET),
        ('attach', ['--reset', *INIT_ARGS], CLIENT_HEAD + CONNECT + MONITOR_RESET + INIT_EX),
        # 'reset' is a fixed sequence with no user session, so it ignores the
        # option even when a board passes it through shared runner args.
        ('reset', [], CLIENT_HEAD + CONNECT + RESET_SEQUENCE),
        ('reset', INIT_ARGS, CLIENT_HEAD + CONNECT + RESET_SEQUENCE),
        # --gdb-host only changes where the client connects.
        ('debug', ['--gdb-host', 'h'], CLIENT_HEAD + CONNECT_HOST + LOAD),
    ],
)
def test_client_cmd(client_cmd, command, argv, expected):
    assert client_cmd(command, argv) == expected


def test_gdb_init_passed_verbatim(client_cmd):
    '''The commands are not parsed, so GDB expression syntax survives.'''
    command = 'p {int}0x20000000'
    assert client_cmd('attach', ['--gdb-init', command])[-2:] == ['-ex', command]


def test_gdb_init_requires_the_capability(runner_config):
    '''Runners without the capability reject the option instead of ignoring it.

    Exercises the shared gate in runners/core.py through a runner that declares
    the capability, with it switched back off.
    '''

    class NoGdbInit(JLinkBinaryRunner):
        @classmethod
        def capabilities(cls):
            return replace(super().capabilities(), gdb_init=False)

    parser = argparse.ArgumentParser(allow_abbrev=False)
    NoGdbInit.add_parser(parser)
    args = parser.parse_args(['--device', TEST_DEVICE, '--gdb-init', 'p 1'])

    with pytest.raises(ValueError, match="doesn't support --gdb-init"):
        NoGdbInit.create(runner_config._replace(file=None), args)
