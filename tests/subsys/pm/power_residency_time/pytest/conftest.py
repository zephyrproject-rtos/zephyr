#!/usr/bin/env python3
# Copyright (c) 2024 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0
import pytest
from twister_harness import DeviceAdapter

from scripts.pm.pwsh_stm32l562 import PowerShield

def pytest_addoption(parser):
    parser.addoption("--powershield",
            help="Path to PowerShield device")

@pytest.fixture(scope="module")
def current_measurement_output(dut: DeviceAdapter, request):
    powershield_device = request.config.getoption("--powershield")
    PwSh = PowerShield()
    PwSh.init_power_monitor(powershield_device)
    PwSh.measure_current(10)
    data = PwSh.get_data()
    PwSh.pwsh.close()
    return data
