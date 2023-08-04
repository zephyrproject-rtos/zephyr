# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import time
import logging

from twister_harness import DeviceAdapter
from twister_harness.exceptions import TwisterHarnessTimeoutException

logger = logging.getLogger(__name__)


def wait_for_message(dut: DeviceAdapter, message, timeout=20):
    time_started = time.time()
    while True:
        line = dut.readline(timeout=timeout)
        if message in line:
            return True
        if time.time() > time_started + timeout:
            return False


def wait_for_prompt(dut: DeviceAdapter, prompt='uart:~$', timeout=20):
    time_started = time.time()
    while True:
        dut.write(b'\n')
        try:
            line = dut.readline(timeout=0.5)
        except TwisterHarnessTimeoutException:
            # ignore read timeout and try to send enter once again
            continue
        if prompt in line:
            logger.debug('Got prompt')
            return True
        if time.time() > time_started + timeout:
            return False


def test_shell_print_help(dut: DeviceAdapter):
    wait_for_prompt(dut)
    dut.write(b'help\n')
    assert wait_for_message(dut, "Available commands")


def test_shell_print_version(dut: DeviceAdapter):
    wait_for_prompt(dut)
    dut.write(b'kernel version\n')
    assert wait_for_message(dut, "Zephyr version")
