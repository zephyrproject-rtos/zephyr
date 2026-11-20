# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

import logging
import re

import pytest
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)


def pytest_addoption(parser) -> None:
    """Add local parser options to pytest."""
    parser.addoption('--hci-transport', default=None, help='Configuration HCI transport for bumble')


@pytest.fixture(name='initialize', scope='session')
def fixture_initialize(request, shell: Shell, dut: DeviceAdapter):
    """Session initialization"""
    # Get HCI transport for bumble
    hci = request.config.getoption('--hci-transport')

    if hci is None:
        for fixture in dut.device_config.fixtures:
            if fixture.startswith('usb_hci:'):
                hci = fixture.split(sep=':', maxsplit=1)[1]
                break

    assert hci is not None

    shell.exec_command("bt init")
    lines = dut.readlines_until(regex="Bluetooth initialized")
    regex = r'Identity: *[PR]:(?P<bd_addr>([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2})'
    bd_addr = None
    for line in lines:
        logger.info(f"Shell log {line}")
        m = re.search(regex, line)
        if m:
            bd_addr = m.group('bd_addr')

    if bd_addr is None:
        logger.error('Fail to get IUT BD address')
        raise AssertionError

    shell.exec_command("br pscan on")
    shell.exec_command("br iscan on")
    logger.info('initialized')
    return hci, bd_addr


@pytest.fixture(name='registered', scope='session')
def fixture_registered(initialize, shell: Shell, dut: DeviceAdapter):
    """Register the callbacks and the SBC endpoint, which every test of the suite needs."""
    shell.exec_command("br clear all")
    for command, response in (
        ("a2dp_sink register_cb", "success"),
        ("a2dp_sink register_ep sink sbc", "SBC sink endpoint is registered"),
    ):
        lines = shell.exec_command(command)
        if not any(response in line for line in lines):
            dut.readlines_until(regex=response, timeout=10)
    return initialize


@pytest.fixture
def a2dp_sink_dut(registered):
    logger.info('Start running testcase')
    yield registered
    logger.info('Done')
