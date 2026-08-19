# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
import logging
import time
import uuid

import pytest
from twister_harness import Shell

logger = logging.getLogger(__name__)


def test_ble_connect(shells: list[Shell]):
    """Flash two DUTs with a Bluetooth shell image and verify they can connect to each other.

    Set a unique advertised device name on the peripheral, start advertising,
    connect from the central by name, and verify both sides reach connected state.
    Finally disconnect and verify the central observes the disconnection.
    """
    assert len(shells) > 1, "This test requires more than one DUT to be configured."
    shell_central = shells[0]
    dut_central = shell_central._device
    shell_peripheral = shells[1]
    dut_peripheral = shell_peripheral._device

    lines = shell_central.exec_command("bt init")
    pytest.LineMatcher(lines).fnmatch_lines(["Bluetooth initialized"])

    lines = shell_peripheral.exec_command("bt init")
    pytest.LineMatcher(lines).fnmatch_lines(["Bluetooth initialized"])

    adv_device_name = f"ble_test_{uuid.uuid4().hex[:8]}"
    logger.info(f"Set unique advertised device name on the peripheral: {adv_device_name}")
    shell_peripheral.exec_command(f"bt name {adv_device_name}")
    lines = shell_peripheral.exec_command("bt advertise on")
    pytest.LineMatcher(lines).fnmatch_lines(["Advertising started"])

    logger.info("Connect from the central by name")
    lines = shell_central.exec_command(f"bt connect-name {adv_device_name}")
    pytest.LineMatcher(lines).fnmatch_lines(["Bluetooth active scan enabled"])
    dut_peripheral.readlines_until(regex="Connected.*", timeout=10)

    logger.info(
        "Successfully connected,  wait a bit to ensure connection is fully established"
        " and stable then disconnect"
    )
    time.sleep(3)
    shell_peripheral.exec_command("bt disconnect")
    dut_central.readlines_until(regex="Disconnected.*", timeout=10)
