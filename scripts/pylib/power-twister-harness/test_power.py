# Copyright (c) 2024 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import logging

import pytest
from abstract.PowerMonitor import PowerMonitor
from twister_harness import DeviceAdapter
from utils.UtilityFunctions import current_RMS

logger = logging.getLogger(__name__)


@pytest.mark.parametrize("probe_class", ['stm_powershield'], indirect=True)
def test_power_harness(probe_class: PowerMonitor, test_data, request, dut: DeviceAdapter):
    """
    This test measures and validates the RMS current values from the power monitor
    and compares them against expected RMS values.

    Arguments:
    probe_class -- The Abstract class of the power monitor.
    test_data -- Fixture to prepare data.
    request -- Request object that provides access to test configuration.
    dut -- The Device Under Test (DUT) that the power monitor is measuring.
    """

    # Initialize the probe with the provided path
    probe = probe_class  # Instantiate the power monitor probe

    # Get test data
    measurements_dict = test_data

    # Start the measurement process with the provided duration
    probe.measure(time=measurements_dict['measurement_duration'])

    # Retrieve the measured data
    data = probe.get_data()

    # Calculate the RMS current values using utility functions
    rms_values_measured = current_RMS(
        data,
        trim=measurements_dict['elements_to_trim'],
        num_peaks=measurements_dict['num_of_transitions'],
        peak_distance=measurements_dict['min_peak_distance'],
        peak_height=measurements_dict['min_peak_height'],
        padding=measurements_dict['peak_padding'],
    )

    # Convert measured values from amps to milliamps for comparison
    # Measured states: ['idle', 'state 0', 'state 1', 'state 2', 'active']
    rms_values_in_milliamps = [value * 1e3 for value in rms_values_measured]

    # Log the calculated values in milliamps for debugging purposes
    logger.debug(f"Measured RMS values in mA: {rms_values_in_milliamps}")
    logger.debug(f"Expected RMS values in mA: {measurements_dict['expected_rms_values']}")

    tuples = zip(measurements_dict['expected_rms_values'], rms_values_in_milliamps, strict=False)
    if not tuple:
        pytest.skip('Measured values not provided')

    logger.debug("Check if measured values are in tolerance")
    measure_passed = True
    for expected_rms_value, measured_rms_value in tuples:
        if not is_within_tolerance(
            measured_rms_value, expected_rms_value, measurements_dict['tolerance_percentage']
        ):
            logger.error(f"Measured RMS value {measured_rms_value} mA is out of tolerance.")
            measure_passed = False

    assert measure_passed, (
        f"Measured RMS value in mA is out of tolerance "
        f"{measurements_dict['tolerance_percentage']} %"
    )


def is_within_tolerance(measured_rms_value, expected_rms_value, tolerance_percentage) -> bool:
    """
    Checks if the measured RMS value is within the acceptable tolerance.

    Arguments:
    measured_rms_value -- The measured RMS current value in milliamps.
    expected_rms_value -- The expected RMS current value in milliamps.
    tolerance_percentage -- The allowed tolerance as a percentage of the expected value.

    Returns:
    bool -- True if the measured value is within the tolerance, False otherwise.
    """
    # Calculate tolerance as a percentage of the expected value
    tolerance = (tolerance_percentage / 100) * expected_rms_value

    # Log the values for debugging purposes
    logger.debug(f"Expected RMS: {expected_rms_value:.3f} mA")
    logger.debug(f"Tolerance: {tolerance:.3f} mA")
    logger.debug(f"Measured  RMS: {measured_rms_value:.3f} mA")
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
    # Check if the measured value is within the range of expected ± tolerance
    if (expected_rms_value - tolerance) < measured_rms_value < (expected_rms_value + tolerance):
        return True

    logger.error(
        f"Measured RMS value: {measured_rms_value:.3f} mA is out of tolerance: {tolerance:.3f} mA"
    )
    return False
