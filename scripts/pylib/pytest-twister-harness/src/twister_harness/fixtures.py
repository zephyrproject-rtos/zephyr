# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging
from typing import Generator, Type

import pytest
import time

from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.device.factory import DeviceFactory
from twister_harness.twister_harness_config import DeviceConfig, TwisterHarnessConfig
from twister_harness.helpers.shell import Shell
from twister_harness.helpers.mcumgr import MCUmgr

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


def determine_scope(fixture_name, config):
    if dut_scope := config.getoption("--dut-scope", None):
        return dut_scope
    return 'function'


@pytest.fixture(scope=determine_scope)
def dut(request: pytest.FixtureRequest, device_object: DeviceAdapter) -> Generator[DeviceAdapter, None, None]:
    """Return launched device - with run application."""
    device_object.initialize_log_files(request.node.name)
    try:
        device_object.launch()
        yield device_object
    finally:  # to make sure we close all running processes execution
        device_object.close()


@pytest.fixture(scope=determine_scope)
def shell(dut: DeviceAdapter) -> Shell:
    """Return ready to use shell interface"""
    shell = Shell(dut, timeout=20.0)
    logger.info('Wait for prompt')
    if not shell.wait_for_prompt():
        pytest.fail('Prompt not found')
    if dut.device_config.type == 'hardware':
        # after booting up the device, there might appear additional logs
        # after first prompt, so we need to wait and clear the buffer
        time.sleep(0.5)
        dut.clear_buffer()
    return shell


@pytest.fixture(scope='session')
def is_mcumgr_available() -> None:
    if not MCUmgr.is_available():
        pytest.skip('mcumgr not available')


@pytest.fixture()
def mcumgr(is_mcumgr_available: None, dut: DeviceAdapter) -> Generator[MCUmgr, None, None]:
    yield MCUmgr.create_for_serial(dut.device_config.serial)
