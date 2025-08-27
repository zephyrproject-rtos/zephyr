# Copyright: (c)  2025, Intel Corporation
# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

import json
import logging
import os
from collections.abc import Generator

import pytest
from twister_harness import DeviceAdapter
from twister_harness.device.factory import DeviceFactory
from twister_harness.exceptions import TwisterHarnessException
from twister_harness.twister_harness_config import DeviceConfig
from utils.UartSettings import UARTParser, UARTSettings


def pytest_addoption(parser):
    parser.addoption('--testdata')


@pytest.fixture
def probe_class(request, probe_path, dft):
    path = probe_path  # Get path of power device
    probe = None
    if request.param == 'stm_powershield':
        from stm32l562e_dk.PowerShield import PowerShield

        probe = PowerShield()  # Instantiate the power monitor probe
        probe.connect(path)
        probe.init()

    if request.param == 'general_powershield':
        from general_adc_platform.GeneralAdcPowerMonitor import GeneralPowerShield

        probe = GeneralPowerShield()  # Instantiate the power monitor probe
        probe.connect(dft)
        probe.init()

    yield probe

    if request.param in ['stm_powershield', 'general_powershield']:
        probe.disconnect()


@pytest.fixture(name='probe_path', scope='session')
def fixture_probe_path(request, dut: DeviceAdapter):
    probe_port = None
    for fixture in dut.device_config.fixtures:
        if fixture.startswith('pm_probe'):
            probe_port = fixture.split(':')[1]
    return probe_port


@pytest.fixture(name='test_data', scope='session')
def fixture_test_data(request):
    # Get test data from the configuration and parse it into a dictionary
    measurements = request.config.getoption("--testdata")
    measurements = measurements.replace("'", '"')  # Ensure the data is properly formatted as JSON
    measurements_dict = json.loads(measurements)

    # Data validation
    required_keys = [
        'elements_to_trim',
        'min_peak_distance',
        'min_peak_height',
        'peak_padding',
        'measurement_duration',
        'num_of_transitions',
        'expected_rms_values',
        'tolerance_percentage',
    ]

    for key in required_keys:
        if key not in measurements_dict:
            logging.error(f"Missing required test data key: {key}")
            pytest.fail(f"Missing required test data key: {key}")

    return measurements_dict


def determine_scope(fixture_name, config):
    """Determine fixture scope based on command line options."""
    if dut_scope := config.getoption("--dut-scope", None):
        return dut_scope
    return 'function'


@pytest.fixture(scope='session')
def dft_object(
    request: pytest.FixtureRequest, probe_path: str, dut: DeviceAdapter
) -> Generator[DeviceAdapter, None, None]:
    """Return device object - without run application."""
    if probe_path:
        parser = UARTParser()
        settings: UARTSettings = parser.parse(probe_path)
        logging.info(f"uart settings {settings}")
        build_dir = dut.device_config.build_dir / "power_shield"
        if not os.path.exists(build_dir):
            os.mkdir(build_dir, 0o755)
        if settings:
            device_config: DeviceConfig = DeviceConfig(
                type="hardware",
                build_dir=build_dir,
                base_timeout=dut.device_config.base_timeout,
                flash_timeout=dut.device_config.flash_timeout,
                platform="genernal_adc_platform",
                serial=settings.port,
                baud=settings.baudrate,
                runner="",
                runner_params="",
                id="",
                product="General",
                serial_pty=None,
                flash_before="",
                west_flash_extra_args="",
                pre_script="",
                post_script="",
                post_flash_script="",
                fixtures="",
                extra_test_args="",
            )
            device_class: DeviceAdapter = DeviceFactory.get_device(device_config.type)
            device_object = device_class(device_config)
            try:
                yield device_object
            finally:  # to make sure we close all running processes execution
                device_object.close()


@pytest.fixture(scope=determine_scope)
def dft(request: pytest.FixtureRequest, dft_object) -> Generator[DeviceAdapter, None, None]:
    """Return launched HardwareAdapter device - with hardware device flashed and connected."""
    # Get twister harness config from pytest request
    # Initialize log files with test node name
    dft_object.initialize_log_files(request.node.name)
    try:
        # Launch the hardware adapter (flash and connect)
        try:
            dft_object.launch()
            logging.info(
                'DFT: HardwareAdapter launched for device %s', dft_object.device_config.serial
            )
        except TwisterHarnessException as e:
            logging.warning(f'TwisterFlashException ignored during launch: {e}')
        yield dft_object
    finally:
        # Ensure proper cleanup
        dft_object.close()
        logging.info('DFT: HardwareAdapter closed')
