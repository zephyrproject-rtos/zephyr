# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
import logging

from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


def test_ble_central_peripheral_ht(duts: list[DeviceAdapter]):
    """Flash two DUTs: one as BLE peripheral (Health Thermometer) and one as BLE central.

    Verify they connect and that the central can read temperature measurements from the peripheral.
    """
    assert len(duts) > 1, "This test requires more than one DUT to be configured."
    dut_peripheral = duts[0]
    dut_central = duts[1]

    logger.info("Waiting for the central and peripheral to connect and exchange data")
    dut_central.readlines_until(regex="Temperature", timeout=10)
    dut_peripheral.readlines_until(regex="temperature", timeout=10)
