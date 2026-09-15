# Copyright: (c)  2025, Intel Corporation
# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import json
import logging
import os
from typing import TYPE_CHECKING

import pytest
from twister_harness import DeviceAdapter

if TYPE_CHECKING:
    from abstract.PowerMonitor import PowerMonitor


def pytest_addoption(parser):
    parser.addoption('--testdata')
    parser.addoption(
        '--probe-class',
        action='store',
        default=os.environ.get('PROBE_CLASS', 'stm_powershield'),
        choices=['stm_powershield', 'general_powershield'],
    )


def determine_scope(_fixture_name, config):
    if dut_scope := config.getoption('--dut-scope', None):
        return dut_scope
    return 'function'


def _get_pm_probe_fixture_value(dut: DeviceAdapter) -> str | None:
    for fixture in dut.device_config.fixtures or []:
        if fixture.startswith('pm_probe:'):
            return fixture.split(':', 1)[1]
    return None


@pytest.fixture(scope=determine_scope)
def measurement_duts(
    duts: list[DeviceAdapter],
) -> tuple[DeviceAdapter, DeviceAdapter | None]:
    if not duts:
        pytest.fail('No DUTs were reserved for the power test')

    primary_dut = duts[0]
    monitor_dut = duts[1] if len(duts) > 1 else None
    return primary_dut, monitor_dut


@pytest.fixture(scope=determine_scope)
def probe_class(
    request: pytest.FixtureRequest,
    measurement_duts: tuple[DeviceAdapter, DeviceAdapter | None],
) -> PowerMonitor:
    primary_dut, monitor_dut = measurement_duts
    probe_name = request.config.getoption('--probe-class')
    probe = None

    if probe_name == 'stm_powershield':
        probe_path = _get_pm_probe_fixture_value(primary_dut)
        if not probe_path:
            pytest.skip('pm_probe fixture not found for stm_powershield')

        from stm32l562e_dk.PowerShield import PowerShield

        probe = PowerShield()
        probe.connect(probe_path)
        probe.init()
    elif probe_name == 'general_powershield':
        if monitor_dut is None:
            pytest.skip(
                'general_powershield requires a second DUT reserved through required_devices'
            )

        from general_power import GeneralPowerShield

        probe = GeneralPowerShield()
        probe.connect(monitor_dut)
        probe.init()
    else:
        pytest.fail(f'Unsupported probe class: {probe_name}')

    try:
        yield probe
    finally:
        if probe is not None:
            probe.disconnect()


@pytest.fixture(name='test_data', scope='session')
def fixture_test_data(request: pytest.FixtureRequest) -> dict:
    measurements = request.config.getoption('--testdata')
    if not measurements:
        pytest.fail('--testdata must be provided')

    measurements = measurements.replace("'", '"')
    measurements_dict = json.loads(measurements)

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
            logging.error('Missing required test data key: %s', key)
            pytest.fail(f'Missing required test data key: {key}')

    return measurements_dict
