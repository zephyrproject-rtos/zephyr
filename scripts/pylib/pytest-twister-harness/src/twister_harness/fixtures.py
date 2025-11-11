# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging
from pathlib import Path
from typing import Generator, Type, Callable

import pytest
import time

from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.device.factory import DeviceFactory
from twister_harness.twister_harness_config import DeviceConfig, TwisterHarnessConfig
from twister_harness.helpers.shell import Shell
from twister_harness.helpers.mcumgr import MCUmgr, MCUmgrBle
from twister_harness.helpers.utils import find_in_config
from twister_harness.helpers.config_reader import ConfigReader

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
def unlaunched_dut(
    request: pytest.FixtureRequest, device_object: DeviceAdapter
) -> Generator[DeviceAdapter, None, None]:
    """Return device object - with logs connected, but not run"""
    device_object.initialize_log_files(request.node.name)
    try:
        yield device_object
    finally:  # to make sure we close all running processes execution
        device_object.close()


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
    if prompt := find_in_config(Path(dut.device_config.app_build_dir) / 'zephyr' / '.config',
                                'CONFIG_SHELL_PROMPT_UART'):
        shell.prompt = prompt
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
def required_build_dirs(request: pytest.FixtureRequest) -> list[str]:
    return request.config.getoption('--required-build')


@pytest.fixture()
def mcumgr(device_object: DeviceAdapter) -> Generator[MCUmgr, None, None]:
    """Fixture to create an MCUmgr instance for serial connection."""
    if not MCUmgr.is_available():
        pytest.skip('mcumgr not available')
    yield MCUmgr.create_for_serial(device_object.device_config.serial)


@pytest.fixture()
def mcumgr_ble(device_object: DeviceAdapter) -> Generator[MCUmgrBle, None, None]:
    """Fixture to create an MCUmgr instance for BLE connection."""
    if not MCUmgrBle.is_available():
        pytest.skip('mcumgr for ble not available')

    for fixture in device_object.device_config.fixtures:
        if fixture.startswith('usb_hci:'):
            hci_name = fixture.split(':', 1)[1]
            break
    else:
        pytest.skip('usb_hci fixture not found')

    try:
        hci_index = int(hci_name.split('hci')[-1])
    except ValueError:
        pytest.skip(f'Invalid HCI name: {hci_name}. Expected format is "hciX".')

    peer_name = find_in_config(
        Path(device_object.device_config.app_build_dir) / 'zephyr' / '.config', 'CONFIG_BT_DEVICE_NAME'
    ) or 'Zephyr'

    yield MCUmgrBle.create_for_ble(hci_index, peer_name)


@pytest.fixture
def config_reader() -> Callable[[str | Path], ConfigReader]:
    """
    Pytest fixture that provides a ConfigReader instance for reading configuration files.

    This fixture allows tests to easily create a ConfigReader object by passing
    the path to a configuration file. The ConfigReader reads the file and
    provides a method to access the configuration data.

    Returns:
        Callable[[str, Path], ConfigReader]: A function that takes a file path
        (as a string or Path object) and returns an instance of ConfigReader.

    Example:
        def test_config_value(config_reader):
            config = config_reader("build_dir/zephyr/.config")
            assert config.read("some_key") == "expected_value"
    """
    def inner(file):
        return ConfigReader(file)

    return inner
