# Copyright (c) 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import logging

logger = logging.getLogger(__name__)


def test_os_boot(dut, harness_devices):
    dut.readlines_until('Booting Zephyr OS build', timeout=5)
    harness_devices[0].readlines_until('Booting Zephyr OS build', timeout=5)


def test_bluetooth_boot(dut, harness_devices):
    dut.readlines_until('Scanning successfully started', timeout=5)
    harness_devices[0].readlines_until('Advertising successfully started', timeout=5)


def test_bluetooth_connection(dut, harness_devices):
    dut.readlines_until('Connected', timeout=5)


def test_match_temp_value_over_ble(dut, harness_devices):
    dut.readlines_until(r'Temperature \d{1,2}C', timeout=5)
