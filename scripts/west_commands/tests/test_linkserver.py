# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

'''Tests for the argv the LinkServer runner hands to the GDB client.'''

import argparse
from unittest.mock import patch

import pytest
from conftest import RC_GDB, RC_KERNEL_ELF

from runners.linkserver import DEFAULT_LINKSERVER_EXE, LinkServerBinaryRunner

#
# Test values. The LinkServer version only matters because some argv contents
# are gated on it; this one is recent enough that nothing is gated out.
#

TEST_DEVICE = 'test-device'
TEST_LINKSERVER_VERSION = 'v1.5.30'

#
# Expected results.
#
# The fragments the runner appends to the GDB client argv, in the order it
# appends them.
#

# The runner passes an empty string in place of '-batch' when batch mode is off.
CLIENT_HEAD = [RC_GDB, RC_KERNEL_ELF, '']
CLIENT_HEAD_BATCH = [RC_GDB, RC_KERNEL_ELF, '-batch']

CONNECT = ['-ex', 'target remote :3333']
# LinkServer reports a flash node pointing at RAM as inaccessible, so the runner
# turns that check off before loading.
LOAD = ['-ex', 'set mem inaccessible-by-default off', '-ex', 'monitor reset', '-ex', 'load']
TEARDOWN = ['-ex', 'monitor ondisconnect cont', '-ex', 'monitor kill_server', '-ex', 'quit']

# Two --gdb-init commands, and the argv they must produce.
INIT_ARGS = ['--gdb-init', 'monitor semihosting enable', '--gdb-init', 'b main']
INIT_EX = ['-ex', 'monitor semihosting enable', '-ex', 'b main']


#
# Helpers
#


def require_patch(program, path=None):
    assert program == DEFAULT_LINKSERVER_EXE
    return program


def create_from_args(runner_config, argv):
    parser = argparse.ArgumentParser(allow_abbrev=False)
    LinkServerBinaryRunner.add_parser(parser)
    args = parser.parse_args(['--device', TEST_DEVICE, *argv])
    return LinkServerBinaryRunner.create(runner_config, args)


#
# Fixtures
#


@pytest.fixture
def client_cmd(runner_config, tmpdir):
    '''Run a command and return the argv handed to the GDB client.'''

    def _run(command, argv=()):
        # The runner refuses to debug unless the ELF file exists, so create an
        # empty one; the commands using it are patched out below.
        tmpdir.ensure(RC_KERNEL_ELF)
        tmpdir.chdir()

        runner = create_from_args(runner_config, argv)

        with (
            patch('runners.core.ZephyrBinaryRunner.require', side_effect=require_patch),
            # Answer the 'LinkServer -v' probe without running anything.
            patch.object(
                LinkServerBinaryRunner,
                'check_output',
                return_value=f'LinkServer {TEST_LINKSERVER_VERSION}'.encode(),
            ),
            patch.object(LinkServerBinaryRunner, 'run_server_and_client') as with_server,
        ):
            runner.run(command)

        return with_server.call_args[0][1]

    return _run


#
# Test cases
#
# --gdb-init commands are appended after the ones the runner issues itself and
# before the session ends, so they cannot change the runner's defaults and still
# take effect while the target is halted.
#


@pytest.mark.parametrize(
    'command, argv, expected',
    [
        # Interactive 'debug' hands the prompt to the user, so it must not tear
        # the session down.
        ('debug', [], CLIENT_HEAD + CONNECT + LOAD),
        ('debug', INIT_ARGS, CLIENT_HEAD + CONNECT + LOAD + INIT_EX),
        # Batch mode ends the session itself instead of waiting for input.
        ('debug', ['--batch'], CLIENT_HEAD_BATCH + CONNECT + LOAD + TEARDOWN),
        ('debug', ['--batch', *INIT_ARGS], CLIENT_HEAD_BATCH + CONNECT + LOAD + INIT_EX + TEARDOWN),
        # 'attach' leaves the target running and loads nothing.
        ('attach', [], CLIENT_HEAD + CONNECT),
        ('attach', INIT_ARGS, CLIENT_HEAD + CONNECT + INIT_EX),
    ],
)
def test_client_cmd(client_cmd, command, argv, expected):
    assert client_cmd(command, argv) == expected
