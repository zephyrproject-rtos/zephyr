# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""Fixtures for the native_sim Wi-Fi interop test.

The tests drive the Zephyr ``wifi`` shell against a real Linux
``mac80211_hwsim`` radio with hostapd access points. Two things have to happen
before the tests can run, and both are handled here so that ``test_wifi.py``
stays independent of how the radio is brought up:

* the simulated radio + APs must exist on the host (``wifi_radio`` fixture), and
* the native_sim binary must be granted ``cap_net_raw``/``cap_net_admin`` before
  it is launched, because it opens an AF_PACKET socket and an nl80211 netlink
  socket on the host STA interface (``dut`` fixture override).

See README.md for the host prerequisites.
"""

import logging
import os
import subprocess
import time
from pathlib import Path

import pytest
from twister_harness import DeviceAdapter
from twister_harness.fixtures import determine_scope

logger = logging.getLogger(__name__)


def pytest_addoption(parser):
    parser.addoption(
        '--wifi-setup',
        action='store',
        default='none',
        choices=('none', 'net-tools'),
        help='How to bring up the mac80211_hwsim radio + APs. "none" (default) '
        'assumes the radio is already set up; "net-tools" runs the net-tools '
        'setup script for the duration of the session.',
    )
    parser.addoption(
        '--net-tools-dir',
        action='store',
        default=None,
        help='Path to the net-tools checkout (default: $ZEPHYR_BASE/../tools/net-tools).',
    )
    parser.addoption(
        '--wifi-iface',
        action='store',
        default='zwifi',
        help='Host STA interface the native_sim driver binds to. Must match the '
        '"host-interface" property of the zephyr,native-sim-wifi DT node '
        '(default "zwifi").',
    )
    parser.addoption(
        '--no-setcap',
        action='store_true',
        default=False,
        help='Do not setcap the native_sim binary (assume it already has the '
        'required capabilities, e.g. when running the whole test as root).',
    )


def _maybe_sudo(cmd: list[str]) -> list[str]:
    """Prefix a command with sudo unless we are already root."""
    if os.geteuid() == 0:
        return cmd
    return ['sudo', *cmd]


def _net_tools_dir(request) -> Path:
    opt = request.config.getoption('--net-tools-dir')
    if opt:
        return Path(opt)
    # Prefer $ZEPHYR_BASE, but fall back to deriving the workspace root from this
    # file's location (.../zephyr/tests/net/wifi/interop/pytest/conftest.py) so
    # the test works even when ZEPHYR_BASE is not exported.
    zephyr_base = os.environ.get('ZEPHYR_BASE')
    zephyr_base = Path(zephyr_base) if zephyr_base else Path(__file__).resolve().parents[5]
    return zephyr_base.parent / 'tools' / 'net-tools'


@pytest.fixture(scope='session')
def wifi_radio(request):
    """Ensure the mac80211_hwsim radio + hostapd APs are up for the session."""
    setup = request.config.getoption('--wifi-setup')
    iface = request.config.getoption('--wifi-iface')

    if setup == 'none':
        logger.info('Assuming the Wi-Fi radio "%s" is already set up', iface)
        yield
        return

    net_tools = _net_tools_dir(request)
    script = net_tools / 'net-setup.sh'
    if not script.exists():
        pytest.skip(f'net-tools setup script not found: {script}')

    start = _maybe_sudo([str(script), '-c', 'zwifi.conf', '-i', iface, 'start'])
    logger.info('Starting Wi-Fi radio: %s', ' '.join(start))
    if subprocess.run(start, cwd=net_tools).returncode != 0:
        pytest.skip('Failed to start mac80211_hwsim radio (is it already running?)')

    # Give hostapd a moment to start beaconing before the first scan.
    time.sleep(2)

    yield

    stop = _maybe_sudo([str(script), '-c', 'zwifi.conf', '-i', iface, 'stop'])
    logger.info('Stopping Wi-Fi radio: %s', ' '.join(stop))
    subprocess.run(stop, cwd=net_tools)


@pytest.fixture(scope=determine_scope)
def dut(unlaunched_dut: DeviceAdapter, wifi_radio, request):
    """Grant the native_sim binary net capabilities, then launch it.

    Overrides the twister_harness ``dut`` fixture, which would otherwise launch
    the binary immediately - before we get a chance to setcap it.
    """
    if not request.config.getoption('--no-setcap'):
        binary = Path(unlaunched_dut.device_config.app_build_dir) / 'zephyr' / 'zephyr.exe'
        cmd = _maybe_sudo(['setcap', 'cap_net_raw,cap_net_admin+ep', str(binary)])
        logger.info('Granting capabilities: %s', ' '.join(cmd))
        subprocess.run(cmd, check=True)

    unlaunched_dut.launch()
    yield unlaunched_dut
