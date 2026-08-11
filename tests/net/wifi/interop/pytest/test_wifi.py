# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""Interop tests driving the Zephyr ``wifi`` shell on native_sim.

The tests exercise scan / connect / status / disconnect against the simulated
``mac80211_hwsim`` access points provided by net-tools:

* ``zephyr-open``  - open network on channel 1
* ``zephyr-wpa2``  - WPA2-PSK network on channel 6, passphrase ``password``

The ``shell`` and ``dut`` fixtures (the latter overridden in conftest.py) bring
up the radio, setcap the binary and launch it before the tests run.
"""

import logging

import pytest
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

OPEN_SSID = 'zephyr-open'
WPA2_SSID = 'zephyr-wpa2'
WPA2_PSK = 'password'
WPA2_CHANNEL = 6

# Security types accepted by "wifi connect -k <n>".
KEY_MGMT_NONE = 0
KEY_MGMT_WPA2_PSK = 1


@pytest.fixture(autouse=True)
def _disconnect_after(shell: Shell):
    """Best-effort disconnect after each test to isolate the connection state."""
    yield
    shell.exec_command('wifi disconnect')


def _connect(
    shell: Shell,
    dut: DeviceAdapter,
    ssid: str,
    key_mgmt: int,
    psk: str | None = None,
    timeout: float = 60.0,
) -> None:
    cmd = f'wifi connect -s {ssid} -k {key_mgmt}'
    if psk is not None:
        cmd += f' -p {psk}'
    shell.exec_command(cmd)
    # "Connected" is printed asynchronously once the connection completes.
    dut.readlines_until(regex='Connected', timeout=timeout)


def _disconnect(shell: Shell) -> None:
    # "Disconnection request done" is printed synchronously, so it is part of
    # the command output captured by exec_command (do not readlines_until for
    # it afterwards - it would already be consumed).
    output = '\n'.join(
        shell.exec_command('wifi disconnect', get_full_output=True, full_output_timeout=5.0)
    )
    assert 'Disconnection request done' in output


def test_scan(shell: Shell, dut: DeviceAdapter):
    """`wifi scan` finds the simulated open and WPA2 access points."""
    shell.exec_command('wifi scan')
    output = '\n'.join(dut.readlines_until(regex='Scan request done', timeout=30.0))
    assert OPEN_SSID in output, f'{OPEN_SSID} not found in scan results'
    assert WPA2_SSID in output, f'{WPA2_SSID} not found in scan results'


def test_statistics(shell: Shell):
    """`wifi statistics` prints the expected counters."""
    output = '\n'.join(shell.exec_command('wifi statistics'))
    assert 'Bytes received' in output
    assert 'Packets sent' in output


def test_status_disconnected(shell: Shell):
    """`wifi status` reports a non-connected state when idle."""
    output = '\n'.join(shell.get_filtered_output(shell.exec_command('wifi status')))
    assert 'State:' in output
    assert 'COMPLETED' not in output


def test_connect_open(shell: Shell, dut: DeviceAdapter):
    """Connect to the open network, verify status, then disconnect."""
    _connect(shell, dut, OPEN_SSID, KEY_MGMT_NONE)

    output = '\n'.join(shell.get_filtered_output(shell.exec_command('wifi status')))
    assert 'State: COMPLETED' in output
    assert f'SSID: {OPEN_SSID}' in output

    _disconnect(shell)


def test_connect_wpa2(shell: Shell, dut: DeviceAdapter):
    """Connect to the WPA2-PSK network (full 4-way handshake), verify, disconnect."""
    _connect(shell, dut, WPA2_SSID, KEY_MGMT_WPA2_PSK, psk=WPA2_PSK)

    output = '\n'.join(shell.get_filtered_output(shell.exec_command('wifi status')))
    assert 'State: COMPLETED' in output
    assert f'SSID: {WPA2_SSID}' in output
    assert f'Channel: {WPA2_CHANNEL}' in output

    _disconnect(shell)
