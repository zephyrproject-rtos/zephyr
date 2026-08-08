# Copyright (c) 2024 Vestas Wind Systems A/S
#
# SPDX-License-Identifier: Apache-2.0

"""
Configuration of Zephyr CAN <=> host CAN test suite.
"""

import json
import logging
import re
import subprocess

import pytest
from can import Bus, BusABC
from can.exceptions import CanError
from can.util import load_config
from can_shell import CanShellBus
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)


def _socketcan_channel(context: str) -> str | None:
    """Return the SocketCAN channel for a python-can context, if applicable."""
    try:
        config = load_config(context=context)
    except CanError as err:
        pytest.fail(f'failed to load python-can context "{context}": {err}')

    if config.get('interface') != 'socketcan':
        logger.info(
            'skipping SocketCAN link state check for python-can interface "%s"',
            config.get('interface'),
        )
        return None

    channel = config.get('channel')
    if not channel:
        pytest.fail(
            f'python-can context "{context}" uses SocketCAN but does not configure a channel'
        )

    if not isinstance(channel, str):
        pytest.fail(
            f'python-can context "{context}" uses unsupported SocketCAN channel "{channel}"'
        )

    return channel


def _check_socketcan_link(channel: str) -> None:
    """Fail if a SocketCAN network interface is missing or down."""
    try:
        result = subprocess.run(
            ['ip', '-details', '-json', 'link', 'show', 'dev', channel],
            check=False,
            capture_output=True,
            text=True,
        )
    except FileNotFoundError:
        pytest.fail('ip command not found; install iproute2 to check SocketCAN link state')

    if result.returncode != 0:
        details = result.stderr.strip() or result.stdout.strip()
        if 'does not exist' in details:
            pytest.fail(f'SocketCAN interface "{channel}" not found: {details}')

        pytest.fail(
            f'failed to query SocketCAN interface "{channel}"' + (f': {details}' if details else '')
        )

    try:
        links = json.loads(result.stdout)
    except json.JSONDecodeError as err:
        pytest.fail(f'failed to parse "ip" output for SocketCAN interface "{channel}": {err}')

    if not links:
        pytest.fail(f'SocketCAN interface "{channel}" not found')

    link = links[0]
    link_type = link.get('link_type')
    link_kind = link.get('linkinfo', {}).get('info_kind')
    if link_type != 'can' and link_kind not in ('can', 'vcan'):
        pytest.fail(
            f'network interface "{channel}" is not a SocketCAN interface '
            f'(link_type={link_type}, info_kind={link_kind})'
        )

    flags = link.get('flags', [])
    if 'UP' not in flags:
        pytest.fail(
            f'SocketCAN interface "{channel}" is down; bring it up with '
            f'"sudo ip link set up {channel}"'
        )

    logger.info('SocketCAN interface "%s" is up', channel)


def pytest_addoption(parser) -> None:
    """Add local parser options to pytest."""
    parser.addoption(
        '--can-context',
        default=None,
        help='Configuration context to use for python-can (default: None)',
    )


@pytest.fixture(name='context', scope='session')
def fixture_context(request, dut: DeviceAdapter) -> str:
    """Return the name of the python-can configuration context to use."""
    ctx = request.config.getoption('--can-context')

    if ctx is None:
        for fixture in dut.device_config.fixtures:
            if fixture.startswith('can:'):
                ctx = fixture.split(sep=':', maxsplit=1)[1]
                break

    logger.info('using python-can configuration context "%s"', ctx)
    return ctx


@pytest.fixture(name='chosen', scope='module')
def fixture_chosen(shell: Shell) -> str:
    """Return the name of the zephyr,canbus devicetree chosen device."""
    chosen_regex = re.compile(r'zephyr,canbus:\s+(\S+)')
    lines = shell.get_filtered_output(shell.exec_command('can_host chosen'))

    for line in lines:
        m = chosen_regex.match(line)
        if m:
            chosen = m.groups()[0]
            logger.info('testing on zephyr,canbus chosen device "%s"', chosen)
            return chosen

    pytest.fail('zephyr,canbus chosen device not found or not ready')
    return None


@pytest.fixture(name='host_can_ready', scope='session')
def fixture_host_can_ready(context: str) -> None:
    """Verify that the configured host CAN interface is ready."""
    channel = _socketcan_channel(context)

    if channel is not None:
        _check_socketcan_link(channel)


@pytest.fixture
def can_dut(dut: DeviceAdapter, shell: Shell, chosen: str, host_can_ready: None) -> BusABC:
    """Return DUT CAN bus."""
    bus = CanShellBus(dut, shell, chosen)
    yield bus
    bus.shutdown()
    dut.clear_buffer()


@pytest.fixture
def can_host(context: str, host_can_ready: None) -> BusABC:
    """Return host CAN bus."""
    bus = Bus(config_context=context)
    yield bus
    bus.shutdown()
