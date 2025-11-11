# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import logging

from test_upgrade import run_upgrade_with_confirm
from twister_harness import DeviceAdapter, MCUmgrBle, Shell

logger = logging.getLogger(__name__)


def test_upgrade_with_confirm_ble(mcumgr_ble: MCUmgrBle, dut: DeviceAdapter, shell: Shell):
    """Verify that the application can be updated over BLE"""
    run_upgrade_with_confirm(dut, shell, mcumgr_ble)
