
import logging
import pytest
import re
import os
from twister_harness import DeviceAdapter
from scripts.pm.power_monitor_stm32l562e_dk.PowerShield import PowerShield
from scripts.pm.power_monitor_stm32l562e_dk.UnityFunctions import UnityFunctions
import parametrize_from_file

logger = logging.getLogger(__name__)

PARAM_FILE = os.environ.get("TESTDATA_FILE", "default_params.yaml")

@parametrize_from_file(path=PARAM_FILE)
def test_power_harness(name, rms, num_of_transitions, measurement_time, request):

    logger.info(f'Test Name: {name}')
    logger.info(f'Expected values [rms]: {rms}')

    probe = request.config.getoption("--probe")
    probe_port = request.config.getoption("--probe-port")
    data = []
    if probe == 'stm_powershield':
        PM_Device = PowerShield()
        PM_Device.init(power_device_path=probe_port)
        PM_Device.measure(time=measurement_time)
        data = PM_Device.get_data()

    value_in_amps = UnityFunctions.current_RMS(data, exclude_first_600=True, num_peaks=num_of_transitions)
    # # Convert to milliamps
    value_in_miliamps = [value * 1e4 for value in value_in_amps]

    logger.info(value_in_miliamps)

    for current_value, rms_value in zip(rms, value_in_miliamps, strict=False):
        print_state_rms("state_label", current_value, rms_value)


def print_state_rms(state_label, rms_expected_value, rms_measured_value):
    tolerance = rms_expected_value / 10

    logger.info(f"{state_label}:")
    logger.info(f"Expected RMS: {rms_expected_value:.2f} mA")
    logger.info(f"Tolerance: {tolerance:.2f} mA")
    logger.info(f"Current RMS: {rms_measured_value:.2f} mA")

    assert (
        (rms_expected_value - tolerance) < rms_measured_value < (rms_expected_value + tolerance)
    ), f"RMS value out of tolerance for {state_label}"
