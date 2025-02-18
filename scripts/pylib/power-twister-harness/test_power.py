# Copyright: (c)  2024, Intel Corporation

import json
import logging

from twister_harness import DeviceAdapter

from scripts.pm.power_monitor_stm32l562e_dk.PowerShield import PowerShield
from scripts.pm.power_monitor_stm32l562e_dk.UnityFunctions import UnityFunctions

logger = logging.getLogger(__name__)


def test_power_harness(request, dut: DeviceAdapter):
    measurements = request.config.getoption("--testdata")
    measurements = measurements.replace("'", '"')
    measurements_dict = json.loads(measurements)[0]

    measurement_time = measurements_dict['measurement_time']
    num_of_transitions = measurements_dict['num_of_transitions']
    rms = measurements_dict['rms']

    probe = request.config.getoption("--pm_probe")
    probe_port = request.config.getoption("--pm_probe_port")

    if probe == 'stm_powershield':
        PM_Device = PowerShield()
        PM_Device.init(power_device_path=probe_port)
        PM_Device.measure(time=measurement_time)
        data = PM_Device.get_data()

    value_in_amps = UnityFunctions.current_RMS(
        data, exclude_first_600=True, num_peaks=num_of_transitions
    )
    # Convert to milliamps
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
