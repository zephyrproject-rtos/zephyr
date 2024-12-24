# Copyright (c) 2024 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

import pytest
from twister_harness import DeviceAdapter

from scripts.pm.power_monitor_stm32l562e_dk.PowerShield import PowerShield


def pytest_addoption(parser):
    parser.addoption("--powershield", help="Path to PowerShield device")


@pytest.fixture(scope='module')
def power_monitor_measure(dut: DeviceAdapter, request):
    powershield_device = request.config.getoption("--powershield")
    PM_Device = PowerShield()
    PM_Device.init(power_device_path=powershield_device)
    PM_Device.measure(time=6)
    return PM_Device.get_data()
