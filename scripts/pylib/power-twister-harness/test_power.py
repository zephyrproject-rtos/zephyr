# Copyright: (c)  2024, Intel Corporation

import json
import logging

import pytest
from twister_harness import DeviceAdapter

import scripts.pm.utils.UnityFunctions as UnityFunctions
from scripts.pm.abstract.PowerMonitor import PowerMonitor

logger = logging.getLogger(__name__)


@pytest.mark.parametrize("probe_class", ['stm_powershield'], indirect=True)
def test_power_harness(request, probe_class: PowerMonitor, probe_path, dut: DeviceAdapter):
    measurements = request.config.getoption("--testdata")
    measurements = measurements.replace("'", '"')
    measurements_dict = json.loads(measurements)[0]

    elements_to_trim = measurements_dict['elements_to_trim']
    min_peak_distance = measurements_dict['min_peak_distance']
    min_peak_height = measurements_dict['min_peak_height']
    peak_padding = measurements_dict['peak_padding']
    measurement_time = measurements_dict['measurement_time']
    num_of_transitions = measurements_dict['num_of_transitions']
    rms = measurements_dict['rms']

    probe_port = probe_path
    probe = probe_class()
    probe.init(power_device_path=probe_port)
    probe.measure(time=measurement_time)
    data = probe.get_data()

    value_in_amps = UnityFunctions.current_RMS(
        data,
        trim=elements_to_trim,
        num_peaks=num_of_transitions,
        peak_distance=min_peak_distance,
        peak_height=min_peak_height,
        padding=peak_padding,
    )
    # Convert to milliamps
    value_in_miliamps = [value * 1e4 for value in value_in_amps]

    logger.debug(value_in_miliamps)

    for current_value, rms_value in zip(rms, value_in_miliamps, strict=False):
        print_state_rms("state_label", current_value, rms_value)


def print_state_rms(state_label, rms_expected_value, rms_measured_value):
    tolerance = rms_expected_value / 10

    logger.debug(f"{state_label}:")
    logger.debug(f"Expected RMS: {rms_expected_value:.2f} mA")
    logger.debug(f"Tolerance: {tolerance:.2f} mA")
    logger.debug(f"Current RMS: {rms_measured_value:.2f} mA")

    assert (
        (rms_expected_value - tolerance) < rms_measured_value < (rms_expected_value + tolerance)
    ), f"RMS value out of tolerance for {state_label}"
