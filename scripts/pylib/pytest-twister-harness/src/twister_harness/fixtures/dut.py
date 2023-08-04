# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging
from typing import Generator, Type

import pytest

from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.device.factory import DeviceFactory
from twister_harness.twister_harness_config import DeviceConfig, TwisterHarnessConfig

logger = logging.getLogger(__name__)


@pytest.fixture(scope='session')
def twister_harness_config(request: pytest.FixtureRequest) -> TwisterHarnessConfig:
    """Return twister_harness_config object."""
    twister_harness_config: TwisterHarnessConfig = request.config.twister_harness_config  # type: ignore
    return twister_harness_config


@pytest.fixture(scope='session')
def device_object(twister_harness_config: TwisterHarnessConfig) -> Generator[DeviceAdapter, None, None]:
    """Return device object - without run application."""
    device_config: DeviceConfig = twister_harness_config.devices[0]
    device_type = device_config.type
    device_class: Type[DeviceAdapter] = DeviceFactory.get_device(device_type)
    device_object = device_class(device_config)
    try:
        yield device_object
    finally:  # to make sure we close all running processes execution
        device_object.close()


@pytest.fixture(scope='function')
def dut(request: pytest.FixtureRequest, device_object: DeviceAdapter) -> Generator[DeviceAdapter, None, None]:
    """Return launched device - with run application."""
    test_name = request.node.name
    device_object.initialize_log_files(test_name)
    try:
        device_object.launch()
        yield device_object
    finally:  # to make sure we close all running processes execution
        device_object.close()
