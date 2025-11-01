# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import logging
import random
import re
import time

import pytest
from twister_harness import Shell
from twister_harness.fixtures import determine_scope

logger = logging.getLogger(__name__)


def get_addr_with_name_from_lines(lines: list[str], name: str):
    dut_adv_line = ''
    pattern = r"([0-9A-Fa-f]{2}(?::[0-9A-Fa-f]{2}){5}) \((.*?)\)"
    for line in lines:
        if name in line:
            dut_adv_line = line.strip()
            break
    if dut_adv_line:
        match = re.search(pattern, dut_adv_line)
        if match:
            address = match.group(1)  # get address
            address_type = match.group(2)  # get address type
            return address, address_type
    return None, None


@pytest.fixture(scope=determine_scope)
def harness_build_dirs(request: pytest.FixtureRequest) -> list[str]:
    """
    Return a list of build directories for each harness device.
    """
    build_dir = request.config.getoption('--build-dir')
    return [build_dir]


def test_ble_connection(shell: Shell, harness_shells: list[Shell]) -> None:
    """Test BLE connection between central and peripheral devices.

    Args:
        shell (Shell): The central device shell interface.
        harness_shells (list[Shell]): List of harness device shell interfaces.
    """
    adv_device_name = f"ztest_{random.randint(0, 999999):06}"
    central_device = shell
    peripheral_device = harness_shells[0]

    # Initialize both devices
    peripheral_device.exec_command('bt init')
    central_device.exec_command('bt init')
    central_device._device.readlines_until(regex=r'Bluetooth initialized', timeout=3)

    # Set advertising name and start advertising
    peripheral_device.exec_command(f'bt name {adv_device_name}')
    peripheral_device.exec_command('bt advertise on')

    # Central device scans for the peripheral
    central_device.exec_command('bt scan on')
    lines = central_device._device.readlines_until(regex=rf'{adv_device_name}.*\r?\n?$', timeout=5)
    address, address_type = get_addr_with_name_from_lines(lines, adv_device_name)
    logger.info(f'ADV device address: {address}, address_type: {address_type}')
    central_device.exec_command('bt scan off')

    # Central connects to peripheral
    central_device.exec_command(f'bt connect {address} {address_type}')
    central_device._device.readlines_until(
        regex=rf'Connected: {address} \({address_type}\)', timeout=5
    )
    time.sleep(3)

    # Disconnect and stop advertising
    central_device.exec_command('bt disconnect')
    peripheral_device.exec_command('bt advertise off')
