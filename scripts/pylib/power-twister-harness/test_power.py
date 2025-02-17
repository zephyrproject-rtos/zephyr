# Copyright: (c)  2024, Intel Corporation

import json
import logging

import pytest
import utils.UtilityFunctions as UtilityFunctions
from abstract.PowerMonitor import PowerMonitor
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


@pytest.mark.parametrize("probe_class", ['stm_powershield'], indirect=True)
def test_power_harness(probe_class: PowerMonitor, probe_path, request, dut: DeviceAdapter):
    """
    This test measures and validates the RMS current values from the power monitor
    and compares them against expected RMS values.

    Arguments:
    probe_class -- The Abstract class of the power monitor.
    probe_path -- The path to the probe device.
    request -- Request object that provides access to test configuration.
    dut -- The Device Under Test (DUT) that the power monitor is measuring.
    """

    # Get test data from the configuration and parse it into a dictionary
    measurements = request.config.getoption("--testdata")
    measurements = measurements.replace("'", '"')  # Ensure the data is properly formatted as JSON
    measurements_dict = json.loads(measurements)

    # Extract individual measurement parameters from the config
    elements_to_trim = measurements_dict['elements_to_trim']
    min_peak_distance = measurements_dict['min_peak_distance']
    min_peak_height = measurements_dict['min_peak_height']
    peak_padding = measurements_dict['peak_padding']
    measurement_duration = measurements_dict['measurement_duration']
    num_of_transitions = measurements_dict['num_of_transitions']
    expected_rms_values = measurements_dict['expected_rms_values']
    tolerance_percentage = measurements_dict['tolerance_percentage']

    # Initialize the probe with the provided path
    probe_port = probe_path
    probe = probe_class()  # Instantiate the power monitor probe
    probe.init(power_device_path=probe_port)

    # Start the measurement process with the provided duration
    probe.measure(time=measurement_duration)

    # Retrieve the measured data
    data = probe.get_data()

    # Calculate the RMS current values using utility functions
    rms_values_measured = UtilityFunctions.current_RMS(
        data,
        trim=elements_to_trim,
        num_peaks=num_of_transitions,
        peak_distance=min_peak_distance,
        peak_height=min_peak_height,
        padding=peak_padding,
    )

    # Convert measured values from amps to milliamps for comparison
    rms_values_in_milliamps = [value * 1e4 for value in rms_values_measured]

    # Log the calculated values in milliamps for debugging purposes
    logger.debug(f"Measured RMS values in mA: {rms_values_in_milliamps}")

    for expected_rms_value, measured_rms_value in zip(
        expected_rms_values, rms_values_in_milliamps, strict=False
    ):
        assert is_within_tolerance(measured_rms_value, expected_rms_value, tolerance_percentage)


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
    logger.debug(f"Expected RMS: {expected_rms_value:.2f} mA")
    logger.debug(f"Tolerance: {tolerance:.2f} mA")
    logger.debug(f"Measured  RMS: {measured_rms_value:.2f} mA")

    # Check if the measured value is within the range of expected ± tolerance
    return (expected_rms_value - tolerance) < expected_rms_value < (expected_rms_value + tolerance)
