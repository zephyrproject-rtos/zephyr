# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import time
import logging

from twister_harness.device.device_abstract import DeviceAbstract

logger = logging.getLogger(__name__)


def wait_for_message(dut: DeviceAbstract, message, timeout=20):
    time_started = time.time()
    for line in dut.iter_stdout:
        if line:
            logger.debug("#: " + line)
        if message in line:
            return True
        if time.time() > time_started + timeout:
            return False


def wait_for_prompt(dut: DeviceAbstract, prompt='uart:~$', timeout=20):
    time_started = time.time()
    while True:
        dut.write(b'\n')
        for line in dut.iter_stdout:
            if prompt in line:
                logger.debug('Got prompt')
                return True
        if time.time() > time_started + timeout:
            return False


def test_shell_print_help(dut: DeviceAbstract):
    wait_for_prompt(dut)
    dut.write(b'help\n')
    assert wait_for_message(dut, "Available commands")


def test_shell_print_version(dut: DeviceAbstract):
    wait_for_prompt(dut)
    dut.write(b'kernel version\n')
    assert wait_for_message(dut, "Zephyr version")
