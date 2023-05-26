# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging
from typing import Generator, Type

import pytest

from twister_harness.device.device_abstract import DeviceAbstract
from twister_harness.device.factory import DeviceFactory
from twister_harness.twister_harness_config import DeviceConfig, TwisterHarnessConfig

logger = logging.getLogger(__name__)


@pytest.fixture(scope='function')
def dut(request: pytest.FixtureRequest) -> Generator[DeviceAbstract, None, None]:
    """Return device instance."""
    twister_harness_config: TwisterHarnessConfig = request.config.twister_harness_config  # type: ignore
    device_config: DeviceConfig = twister_harness_config.devices[0]
    device_type = device_config.type

    device_class: Type[DeviceAbstract] = DeviceFactory.get_device(device_type)

    device = device_class(device_config)

    try:
        device.connect()
        device.generate_command()
        device.initialize_log_files()
        device.flash_and_run()
        device.connect()
        yield device
    except KeyboardInterrupt:
        pass
    finally:  # to make sure we close all running processes after user broke execution
        device.disconnect()
        device.stop()
