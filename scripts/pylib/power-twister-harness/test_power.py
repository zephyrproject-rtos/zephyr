# Copyright (c) 2024 Intel Corporation
# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import os
from typing import TYPE_CHECKING

import pytest
from twister_harness import DeviceAdapter
from utils.UtilityFunctions import current_RMS

if TYPE_CHECKING:
    from abstract.PowerMonitor import PowerMonitor

logger = logging.getLogger(__name__)


def _dump_general_power_outputs(
    probe: PowerMonitor,
    platform: str,
) -> bool:
    """Dump general power monitor outputs when the helper supports them."""
    dumped = False
    if hasattr(probe, 'dump_voltage'):
        assert probe.dump_voltage(platform)
        dumped = True
    if hasattr(probe, 'dump_current'):
        assert probe.dump_current(platform)
        dumped = True
    if hasattr(probe, 'dump_power'):
        assert probe.dump_power(platform)
        dumped = True
    return dumped


def test_power_harness(
    probe_class: PowerMonitor,
    test_data: dict,
    measurement_duts: tuple[DeviceAdapter, DeviceAdapter | None],
):
    """Measure power on the primary DUT and validate the results."""
    dut, _monitor_dut = measurement_duts
    probe = probe_class

    build_dir_path = str(dut.device_config.build_dir)
    if os.path.exists(build_dir_path) and hasattr(probe, 'power_shield_conf'):
        probe.power_shield_conf.output_file = os.path.join(
            build_dir_path,
            'power_raw_data.csv',
        )

    probe.measure(test_data['measurement_duration'])
    data = probe.get_data()

    if _dump_general_power_outputs(probe, dut.device_config.platform):
        return

    rms_values_measured = current_RMS(
        data,
        trim=test_data['elements_to_trim'],
        num_peaks=test_data['num_of_transitions'],
        peak_distance=test_data['min_peak_distance'],
        peak_height=test_data['min_peak_height'],
        padding=test_data['peak_padding'],
    )

    rms_values_in_milliamps = [value * 1e3 for value in rms_values_measured]
    logger.debug('Measured RMS values in mA: %s', rms_values_in_milliamps)
    logger.debug(
        'Expected RMS values in mA: %s',
        test_data['expected_rms_values'],
    )

    if not rms_values_in_milliamps:
        pytest.skip('Measured values not provided')

    measure_passed = True
    for expected_rms_value, measured_rms_value in zip(
        test_data['expected_rms_values'],
        rms_values_in_milliamps,
        strict=False,
    ):
        if not is_within_tolerance(
            measured_rms_value,
            expected_rms_value,
            test_data['tolerance_percentage'],
        ):
            logger.error(
                'Measured RMS value %s mA is out of tolerance.',
                measured_rms_value,
            )
            measure_passed = False

    assert measure_passed, (
        f'Measured RMS value in mA is out of tolerance {test_data['tolerance_percentage']} %'
    )


def is_within_tolerance(
    measured_rms_value: float,
    expected_rms_value: float,
    tolerance_percentage: float,
) -> bool:
    """Check whether the measured RMS value is within tolerance."""
    tolerance = (tolerance_percentage / 100) * expected_rms_value

    logger.debug('Expected RMS: %.3f mA', expected_rms_value)
    logger.debug('Tolerance: %.3f mA', tolerance)
    logger.debug('Measured RMS: %.3f mA', measured_rms_value)
    logger.info(
        'RECORD: ['
        '{'
        f'"expected_rms_ua": {expected_rms_value:.3f}'
        '}'
        ',{'
        f'"tolerance_ua": {tolerance:.3f}'
        '}'
        ',{'
        f'"measured_rms_ua": {measured_rms_value:.3f}'
        '}'
        ']'
    )

    if (expected_rms_value - tolerance) < measured_rms_value < (expected_rms_value + tolerance):
        return True

    logger.error(
        'Measured RMS value: %.3f mA is out of tolerance: %.3f mA',
        measured_rms_value,
        tolerance,
    )
    return False
