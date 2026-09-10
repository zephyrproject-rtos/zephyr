# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from twister_harness.device.binary_adapter import NativeSimulatorAdapter
from twister_harness.helpers.shell import Shell


def test_if_shell_helper_properly_send_command(shell_simulator_adapter: NativeSimulatorAdapter) -> None:
    """Run shell_simulator.py program, send "zen" command via shell helper and verify output."""
    shell = Shell(shell_simulator_adapter, timeout=5.0)
    assert shell.wait_for_prompt()
    lines = shell.exec_command('zen')
    assert 'The Zen of Python, by Tim Peters' in lines
