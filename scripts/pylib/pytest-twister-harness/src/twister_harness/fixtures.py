# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import copy
import logging
import time
import subprocess
from pathlib import Path
from typing import Generator, Type

import pytest

from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.device.factory import DeviceFactory
from twister_harness.twister_harness_config import DeviceConfig, TwisterHarnessConfig
from twister_harness.helpers.shell import Shell
from twister_harness.helpers.mcumgr import MCUmgr
from twister_harness.helpers.utils import find_in_config

from twister_harness.helpers.domains_helper import ZEPHYR_BASE
sys.path.insert(0, os.path.join(ZEPHYR_BASE, 'scripts')) # import zephyr_module in environment.py
sys.path.insert(0, os.path.join(ZEPHYR_BASE, 'scripts', 'pylib', 'twister'))
from twisterlib.hardwaremap import HardwareMap

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
def unlaunched_dut(request: pytest.FixtureRequest, device_object: DeviceAdapter) -> Generator[DeviceAdapter, None, None]:
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
def is_mcumgr_available() -> None:
    if not MCUmgr.is_available():
        pytest.skip('mcumgr not available')


@pytest.fixture()
def mcumgr(is_mcumgr_available: None, dut: DeviceAdapter) -> Generator[MCUmgr, None, None]:
    yield MCUmgr.create_for_serial(dut.device_config.serial)


@pytest.fixture(scope='session')
def harness_devices(request, twister_harness_config):
    """Return harness_device list object."""

    class TwisterOptionsWrapper:

        device_flash_timeout: float = 60.0  # [s]
        device_flash_with_test: bool = True
        flash_before: bool = False

    harness_device_yml = request.config.getoption('--harness_device_map')
    harness_apps = request.config.getoption('--harness_apps')
    logger.info(f'harness_device_yml:{harness_device_yml}')
    logger.info(f'harness_apps:{harness_apps}')

    harness_app_list = harness_apps.split(' ')
    logger.info(f'harness_app:{harness_app_list}')

    # reuse twister HardwareMap class to load harness device config yaml
    options = TwisterOptionsWrapper()
    harness_device_hwm = HardwareMap(options)
    harness_device_hwm.load(harness_device_yml)
    logger.info(f'harness_device_hwm[0]:{harness_device_hwm.duts[0]}')

    if not harness_device_hwm.duts or (len(harness_app_list) != len(harness_device_hwm.duts)):
        pytest.skip("Skipping all tests due to wrong harness setting.")

    # reuse most dut config for harness_device, only build and flash different app into them
    dut_device_config: DeviceConfig = twister_harness_config.devices[0]
    logger.info(f'dut_device_config:{dut_device_config}')

    harness_devices = []
    for index, harness_hw in enumerate(harness_device_hwm.duts):
        harness_app = harness_app_list[index]
        # split harness_app into appname, extra_config
        harness_app = harness_app.split('-')
        appname, extra_config = harness_app[0], harness_app[1:]
        extra_config = ['-' + config for config in extra_config]
        # build harness_app image for harness device
        build_dir = f'./harness_{harness_hw.platform}_{os.path.basename(appname)}'
        cmd = ['west', 'build', appname, '-b', harness_hw.platform, '--build-dir', build_dir] + extra_config
        logger.info(' '.join(cmd))
        logger.info(os.getcwd())
        subprocess.call(cmd)

        # update the specific configuration for harness_hw
        harness_device_config = copy.deepcopy(dut_device_config)
        harness_device_config.id = harness_hw.id
        harness_device_config.serial = harness_hw.serial
        harness_device_config.build_dir = Path(build_dir)
        logger.info(f'harness_device_config:{harness_device_config}')

        # init harness device as DuT
        device_class: Type[DeviceAdapter] = DeviceFactory.get_device(harness_device_config.type)
        device_object = device_class(harness_device_config)
        device_object.initialize_log_files(request.node.name)
        harness_devices.append(device_object)

    try:
        for device_object in harness_devices:
            device_object.launch()
        yield harness_devices
    finally:  # to make sure we close all running processes execution
        for device_object in harness_devices:
            device_object.close()
