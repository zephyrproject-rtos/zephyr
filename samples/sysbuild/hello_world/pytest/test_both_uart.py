#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

import logging
import re
import time

import pytest
import serial
from twister_harness import DeviceAdapter
from twister_harness.fixtures import determine_scope

logger = logging.getLogger("test_both_uart")
logger.setLevel(logging.DEBUG)


@pytest.fixture(scope=determine_scope)
def dut(request: pytest.FixtureRequest, device_object: DeviceAdapter):
    device_object.initialize_log_files(request.node.name)

    device_object.second_serial = None
    second_serial_port_name = device_object.device_config.serial
    if "if00" in device_object.device_config.serial:
        second_serial_port_name = device_object.device_config.serial.replace("if00", "if02")
    elif "if02" in device_object.device_config.serial:
        second_serial_port_name = device_object.device_config.serial.replace("if02", "if00")

    if second_serial_port_name != device_object.device_config.serial:
        device_object.second_serial = serial.Serial(
            port=second_serial_port_name,
            baudrate=device_object.device_config.baud,
            timeout=1,
            rtscts=True,
        )
    device_object.launch()
    try:
        yield device_object
    finally:
        device_object.close()
        if device_object.second_serial is not None:
            device_object.second_serial.close()


def test_both_uart(dut: DeviceAdapter):
    """
    Verify logs from both uarts
    """
    timeout = 5
    regex = "Hello world from .*"

    # verify first port
    dut.readlines_until(regex=regex, print_output=True, timeout=timeout)

    # verify second port
    # skip if there is no second port
    if dut.second_serial is not None:
        regex_compiled = re.compile(regex)
        timeout_time: float = time.time() + timeout
        while time.time() < timeout_time:
            try:
                line = dut.second_serial.readline().decode("utf-8")
                logger.debug(f"Second serial: -{line}-")
            except serial.SerialException:
                logger.exception("Second serial")
            if regex_compiled.search(line):
                logger.debug("Second serial - found")
                break
        else:
            raise AssertionError(f"Second serial - regex not found: {regex}")
    else:
        logger.error("Second serial - skipped")
