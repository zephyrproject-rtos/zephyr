# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
import re

import pytest
from test_l2cap_common import L2CAP_SERVER_PSM, logger
from twister_harness import DeviceAdapter, Shell


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

    shell.exec_command("bt init")
    lines = dut.readlines_until("Bluetooth initialized")
    regex = r'Identity: *(?P<bd_addr>([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2}) *\((.*?)\))'
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

    shell.exec_command(f"l2cap_br register {format(L2CAP_SERVER_PSM, 'x')}")
    logger.info("l2cap server register")
    return hci, bd_addr


@pytest.fixture
def l2cap_br_dut(initialize):
    logger.info('Start running testcase')
    yield initialize
    logger.info('Done')
