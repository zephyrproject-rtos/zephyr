# Copyright: (c)  2025, Intel Corporation
import json
import logging

import pytest
from twister_harness import DeviceAdapter


def pytest_addoption(parser):
    parser.addoption('--testdata')


@pytest.fixture
def probe_class(request, probe_path):
    path = probe_path  # Get path of power device
    if request.param == 'stm_powershield':
        from stm32l562e_dk.PowerShield import PowerShield

        probe = PowerShield()  # Instantiate the power monitor probe
        probe.connect(path)
        probe.init()

    yield probe

    if request.param == 'stm_powershield':
        probe.disconnect()


@pytest.fixture(name='probe_path', scope='session')
def fixture_probe_path(request, dut: DeviceAdapter):
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
