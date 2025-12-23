#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
from twister_harness import DeviceAdapter

timeout = 5
regex = "Hello world from .*"


def test_uart_in_app(dut: DeviceAdapter):
    """Verify logs from uart in application"""
    dut.readlines_until(regex=regex, print_output=True, timeout=timeout)


def test_uart_in_second_core(dut: DeviceAdapter):
    """Verify logs from uart in second core"""
    dut.readlines_until(connection_index=1, regex=regex, print_output=True, timeout=timeout)
