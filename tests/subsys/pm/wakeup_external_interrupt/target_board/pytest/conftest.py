#!/usr/bin/env python3
#
# Copyright (c) 2024 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
import pytest
import threading
import time
import serial
from twister_harness import DeviceAdapter
from scripts.pm.pwsh_stm32l562 import PowerShield

def pytest_addoption(parser):
    parser.addoption("--controller",
            help="Path to controller device.")
    parser.addoption("--power-monitor",
            help="Path to PowerShield device")

def send_shell_commands(controllerDevice):
    controller = serial.Serial(controllerDevice,
        baudrate=115200,
        bytesize=serial.EIGHTBITS,
        stopbits=serial.STOPBITS_ONE,
        parity=serial.PARITY_NONE,
        timeout=1)
    time.sleep(9)
    print("setting gpioc 6 as output")
    controller.write(b"gpio conf gpioc 6 o\n")
    print("setting gpioc 6 to 1")
    controller.write(b"gpio set gpioc 6 1\n")
    time.sleep(1)
    controller.write(b"gpio set gpioc 6 0\n")
    print("setting gpioc 6 to 0")

@pytest.fixture(scope="module")
def current_measurement_output(dut: DeviceAdapter, request):
    powershield_device = request.config.getoption("--powershield")
    controller_device = request.config.getoption("--controller-device")
    shell_thread = threading.Thread(target=send_shell_commands, args=(controller_device,))
    shell_thread.start()
    PwSh = PowerShield()
    PwSh.init_power_monitor(powershield_device)
    PwSh.measure_current(5)
    data = PwSh.get_data()
    PwSh.pwsh.close()
    return data
