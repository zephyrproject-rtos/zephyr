# Copyright (c) 2025 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

import pytest
import yaml
from twister_harness import DeviceAdapter

from scripts.pm.power_monitor_stm32l562e_dk.PowerShield import PowerShield


def pytest_addoption(parser):
    parser.addoption("--powershield", help="Path to PowerShield device")
    parser.addoption("--expected-values", help="Path to yaml file with expected values")


def load_power_state_data(path):
    with open(path) as file:
        return yaml.safe_load(file)


@pytest.fixture
def expected_values(request):
    expected_values_file = request.config.getoption("--expected-values")
    return load_power_state_data(expected_values_file)['power_states']


@pytest.fixture(scope='module')
def power_monitor_measure(dut: DeviceAdapter, request):
    powershield_device = request.config.getoption("--powershield")
    PM_Device = PowerShield()
    PM_Device.init(power_device_path=powershield_device)
    PM_Device.measure(time=2)
    return PM_Device.get_data()
