# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import time
import logging
import pytest  # noqa # pylint: disable=unused-import

from twister_harness.device.device_abstract import DeviceAbstract

logger = logging.getLogger(__name__)


def wait_for_message(iter_stdout, message, timeout=60):
    time_started = time.time()
    for line in iter_stdout:
        if line:
            logger.debug("#: " + line)
        if message in line:
            return True
        if time.time() > time_started + timeout:
            return False


def test_shell_print_help(dut: DeviceAbstract):
    time.sleep(1)  # wait for application initialization on DUT

    dut.write(b'help\n')
    assert wait_for_message(dut.iter_stdout, "Available commands")


def test_shell_print_version(dut: DeviceAbstract):
    time.sleep(1)  # wait for application initialization on DUT

    dut.write(b'kernel version\n')
    assert wait_for_message(dut.iter_stdout, "Zephyr version")
