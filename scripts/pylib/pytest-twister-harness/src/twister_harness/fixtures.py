# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import copy
import logging
import time
from pathlib import Path
from typing import Generator, Type, Callable, List

import pytest

from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.device.factory import DeviceFactory
from twister_harness.twister_harness_config import DeviceConfig, TwisterHarnessConfig
from twister_harness.helpers.shell import Shell
from twister_harness.helpers.mcumgr import MCUmgr, MCUmgrBle
from twister_harness.helpers.utils import find_in_config
from twister_harness.helpers.config_reader import ConfigReader
from twister_harness.hwmap import HardwareMap

logger = logging.getLogger(__name__)


@pytest.fixture(scope='session')
def twister_harness_config(request: pytest.FixtureRequest) -> TwisterHarnessConfig:
    """Return twister_harness_config object."""
    twister_harness_config: TwisterHarnessConfig = request.config.twister_harness_config  # type: ignore
    return twister_harness_config


@pytest.fixture(scope='session')
def device_object(
    twister_harness_config: TwisterHarnessConfig,
) -> Generator[DeviceAdapter, None, None]:
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
def dut(
    request: pytest.FixtureRequest, device_object: DeviceAdapter
) -> Generator[DeviceAdapter, None, None]:
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
    if prompt := find_in_config(
        Path(dut.device_config.app_build_dir) / 'zephyr' / '.config',
            'CONFIG_SHELL_PROMPT_UART'
    ):
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


@pytest.fixture(scope=determine_scope)
def harness_devices(
    request: pytest.FixtureRequest,
    dut: DeviceAdapter,
    twister_harness_config: TwisterHarnessConfig,
    harness_build_dirs: List[str],
) -> Generator[List[DeviceAdapter], None, None]:
    """
    Create a list of harness device objects.

    Args:
        request (pytest.FixtureRequest): The pytest fixture request.
        dut (DeviceAdapter): The main device under test.
        twister_harness_config (TwisterHarnessConfig): The Twister Harness configuration.
        harness_build_dirs (List[str]): List of build directories for harness devices.

    Yields:
        List[DeviceAdapter]: List of harness device adapter instances.
    """
    harness_devices_yaml: str | None = None
    for fixture in dut.device_config.fixtures:
        if fixture.startswith('harness_devices_yaml'):
            harness_devices_yaml = fixture.split(sep=':', maxsplit=1)[1]
            break
    if not harness_devices_yaml:
        pytest.fail('No harness_devices_yaml fixture, do not need to call this pytest fixture')
        return
    if not Path(harness_devices_yaml).exists():
        pytest.fail(f'harness_devices_yaml: {harness_devices_yaml} not found')
        return

    harness_device_hwm = HardwareMap()
    harness_device_hwm.load(harness_devices_yaml)
    logger.info(f'harness_device_hwm[0]: {harness_device_hwm.duts[0]}')

    if not harness_device_hwm.duts or (len(harness_build_dirs) > len(harness_device_hwm.duts)):
        pytest.fail(
            "Please check the harness_devices yaml, the number of devices and images do not match"
        )
        return

    # Reuse most dut config for harness_device, only build and flash different app into them
    dut_device_config: DeviceConfig = twister_harness_config.devices[0]
    logger.info(f'dut_device_config: {dut_device_config}')

    harness_devices_list: List[DeviceAdapter] = []
    for index, harness_build_dir in enumerate(harness_build_dirs):
        harness_hw = harness_device_hwm.duts[index]

        # Update the specific configuration for harness_hw
        harness_device_config = copy.deepcopy(dut_device_config)
        harness_device_config.id = harness_hw.id
        harness_device_config.serial = harness_hw.serial
        harness_device_config.build_dir = Path(harness_build_dir)
        logger.info(f'harness_device_config: {harness_device_config}')

        # Init harness device as DuT
        device_class: Type[DeviceAdapter] = DeviceFactory.get_device(harness_device_config.type)
        device_obj = device_class(harness_device_config)
        device_obj.initialize_log_files(request.node.name)
        harness_devices_list.append(device_obj)

    try:
        for device_obj in harness_devices_list:
            device_obj.launch()
        yield harness_devices_list
    finally:
        for device_obj in harness_devices_list:
            device_obj.close()


@pytest.fixture(scope=determine_scope)
def harness_shells(harness_devices: List[DeviceAdapter]) -> List[Shell]:
    """
    Create a list of ready-to-use shell interfaces for harness devices.

    Args:
        harness_devices (List[DeviceAdapter]): List of harness device adapter instances.

    Returns:
        List[Shell]: List of shell interfaces for harness devices.
    """
    shells: List[Shell] = []
    for device_obj in harness_devices:
        shell_interface = Shell(device_obj, timeout=20.0)
        prompt = find_in_config(
            Path(device_obj.device_config.app_build_dir) / 'zephyr' / '.config',
            'CONFIG_SHELL_PROMPT_UART',
        )
        if prompt:
            shell_interface.prompt = prompt
        logger.info('Wait for prompt')
        if not shell_interface.wait_for_prompt():
            pytest.fail('Prompt not found')
        if device_obj.device_config.type == 'hardware':
            # After booting up the device, there might appear additional logs
            # after first prompt, so we need to wait and clear the buffer
            time.sleep(0.5)
            device_obj.clear_buffer()
        shells.append(shell_interface)
    return shells


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

    peer_name = (
        find_in_config(
            Path(device_object.device_config.app_build_dir) / 'zephyr' / '.config',
            'CONFIG_BT_DEVICE_NAME',
        )
        or 'Zephyr'
    )

    yield MCUmgrBle.create_for_ble(hci_index, peer_name)


@pytest.fixture
def config_reader() -> Callable[[str | Path], ConfigReader]:
    """
    Pytest fixture that provides a ConfigReader instance for reading configuration files.

    This fixture allows tests to easily create a ConfigReader object by passing
    the path to a configuration file. The ConfigReader reads the file and
    provides a method to access the configuration data.

    Returns:
        Callable[[str | Path], ConfigReader]: A function that takes a file path
        (as a string or Path object) and returns an instance of ConfigReader.

    Example:
        def test_config_value(config_reader):
            config = config_reader("build_dir/zephyr/.config")
            assert config.read("some_key") == "expected_value"
    """

    def inner(file):
        return ConfigReader(file)

    return inner
