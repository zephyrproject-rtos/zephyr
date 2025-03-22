#!/usr/bin/env python3
#
# Copyright (c) 2025 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
import threading
import time

import pytest
import serial
import yaml
from twister_harness import DeviceAdapter

from scripts.pm.power_monitor_stm32l562e_dk.PowerShield import PowerShield


def pytest_addoption(parser):
    parser.addoption("--controller-device", help="Path to controller device.")
    parser.addoption("--powershield", help="Path to PowerShield device")
    parser.addoption("--expected-values", help="Path to yaml file with expected values")


def load_power_state_data(path):
    with open(path) as file:
        return yaml.safe_load(file)


@pytest.fixture
def expected_values(request):
    expected_rms_file = request.config.getoption("--expected-values")
    return load_power_state_data(expected_rms_file)['power_states']


def send_shell_commands(controllerDevice):
    controller = serial.Serial(
        controllerDevice,
        baudrate=115200,
        bytesize=serial.EIGHTBITS,
        stopbits=serial.STOPBITS_ONE,
        parity=serial.PARITY_NONE,
        timeout=1,
    )
    time.sleep(13)
    print("Setting gpioc 6 as output")
    controller.write(b"gpio conf gpioc 6 o\n")
    print("Setting gpioc 6 to 1")
    controller.write(b"gpio set gpioc 6 1\n")
    time.sleep(1)
    controller.write(b"gpio set gpioc 6 0\n")
    print("Setting gpioc 6 to 0")


@pytest.fixture(scope="module")
def power_monitor_measure(dut: DeviceAdapter, request):
    powershield_device = request.config.getoption("--powershield")
    controller_device = request.config.getoption("--controller-device")
    shell_thread = threading.Thread(target=send_shell_commands, args=(controller_device,))
    shell_thread.start()
    PM_Device = PowerShield()
    PM_Device.init(power_device_path=powershield_device)
    PM_Device.measure(time=6)
    data = PM_Device.get_data()
    return data
