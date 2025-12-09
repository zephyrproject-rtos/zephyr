#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

import logging
import re
import time
from collections.abc import Generator

import pytest
import serial
from twister_harness import DeviceAdapter
from twister_harness.fixtures import determine_scope

logger = logging.getLogger("test_both_uart")
logger.setLevel(logging.DEBUG)


@pytest.fixture(scope=determine_scope)
def second_serial(
    request: pytest.FixtureRequest, device_object: DeviceAdapter
) -> Generator[serial.Serial | None, None, None]:
    """Return second serial port if provided."""
    device_serials = request.config.getoption('--device-serial')

    if not device_serials or len(device_serials) <= 1:
        pytest.skip("Second serial not available")

    logger.debug(f"Opening serial connection for {device_serials[1]}")
    second_serial = serial.Serial(
        port=device_serials[1],
        baudrate=device_object.device_config.baud,
        timeout=1,
        rtscts=True,
    )
    yield second_serial
    second_serial.close()


def test_uart_in_app(dut: DeviceAdapter):
    """Verify logs from uart in application"""
    timeout = 5
    regex = "Hello world from .*"
    dut.readlines_until(regex=regex, print_output=True, timeout=timeout)


def test_uart_in_second_core(second_serial: serial.Serial | None, dut: DeviceAdapter):
    """Verify logs from uart in second core"""
    timeout = 5
    regex = "Hello world from .*"

    regex_compiled = re.compile(regex)
    timeout_time: float = time.time() + timeout
    while time.time() < timeout_time:
        try:
            line = second_serial.readline().decode("utf-8")
            logger.debug(f"Second serial: -{line}-")
        except serial.SerialException:
            logger.exception("Second serial")
        if regex_compiled.search(line):
            logger.debug("Second serial - found")
            break
    else:
        raise AssertionError(f"Second serial - regex not found: {regex}")
