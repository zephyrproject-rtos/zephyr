# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

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
    """Session initializtion"""
    # Get HCI transport for bumble
    hci = request.config.getoption('--hci-transport')

    if hci is None:
        for fixture in dut.device_config.fixtures:
            if fixture.startswith('usb_hci:'):
                hci = fixture.split(sep=':', maxsplit=1)[1]
                break

    assert hci is not None

    lines = shell.exec_command("bt init")
    lines = dut.readlines_until("Settings Loaded")
    regex = r'Identity: *(?P<bd_addr>(.*?):(.*?):(.*?):(.*?):(.*?):(.*?) *\((.*?)\))'
    bd_addr = None
    for line in lines:
        logger.info(f"Shell log {line}")
        m = re.search(regex, line)
        if m:
            bd_addr = m.group('bd_addr')

    if bd_addr is None:
        logger.error('Fail to get IUT BD address')
        raise AssertionError

    lines = shell.exec_command("br pscan on")
    lines = shell.exec_command("br iscan on")
    logger.info('initialized')
    return hci, bd_addr


@pytest.fixture
def smp_initiator_dut(initialize):
    logger.info('Start running testcase')
    yield initialize
    logger.info('Done')
