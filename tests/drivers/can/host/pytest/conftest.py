# Copyright (c) 2024 Vestas Wind Systems A/S
#
# SPDX-License-Identifier: Apache-2.0

"""
Configuration of Zephyr CAN <=> host CAN test suite.
"""

import logging
import re

import pytest
from can import Bus, BusABC
from can_shell import CanShellBus
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)


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
        # No fixtures are needed when the can sidecar provides the bus.
        for fixture in dut.device_config.fixtures or []:
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


@pytest.fixture
def can_dut(dut: DeviceAdapter, shell: Shell, chosen: str) -> BusABC:
    """Return DUT CAN bus."""
    bus = CanShellBus(dut, shell, chosen)
    yield bus
    bus.shutdown()
    dut.clear_buffer()


@pytest.fixture
def can_host(context: str, sidecar_params: dict[str, str]) -> BusABC:
    """Return host CAN bus."""
    iface = sidecar_params.get('iface')
    if iface:
        # The twister "can" sidecar created this SocketCAN interface for this
        # run, so use it directly rather than a python-can configuration context
        # the user would otherwise have to set up by hand.
        logger.info('using SocketCAN interface "%s" from the can sidecar', iface)
        bus = Bus(interface='socketcan', channel=iface, fd=True)
    else:
        bus = Bus(config_context=context)
    yield bus
    bus.shutdown()
