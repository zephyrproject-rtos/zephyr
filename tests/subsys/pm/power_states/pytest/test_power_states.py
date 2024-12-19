# Copyright (c) 2024 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

import logging

from scripts.pm.power_monitor_stm32l562e_dk.UnityFunctions import UnityFunctions


def test_power_states_without_pm_device(power_monitor_measure):
    data = power_monitor_measure
    # Empirical RMS values in mA with descriptive keys
    rms_expected = {
        "k_cpu_idle": 56.0,  # State 0
        "state 0": 4.0,  # State 0
        "state 1": 1.3,  # State 1
        "state 2": 0.27,  # State 2
        "active": 134,  # Active
    }

    value_in_amps = UnityFunctions.current_RMS(data, exclude_first_600=True, num_peaks=4)
    #  Convert to milliamps
    value_in_miliamps = [value * 1e4 for value in value_in_amps]
    logging.info(f"Current RMS [mA]: {value_in_miliamps}")

    def print_state_rms(state_label, rms_expected_value, rms_measured_value):
        tolerance = rms_expected_value / 10

        logging.info(f"{state_label}:")
        logging.info(f"Expected RMS: {rms_expected_value:.2f} mA")
        logging.info(f"Tolerance: {tolerance:.2f} mA")
        logging.info(f"Current RMS: {rms_measured_value:.2f} mA")

        assert (
            (rms_expected_value - tolerance) < rms_measured_value < (rms_expected_value + tolerance)
        ), f"RMS value out of tolerance for {state_label}"

    # Define the state labels
    state_labels = ["k_cpu_idle", "state 0", "state 1", "state 2", "active"]

    # Loop through the states and print RMS values
    for state_label, rms_value in zip(state_labels, value_in_miliamps, strict=False):
        print_state_rms(state_label, rms_expected[state_label], rms_value)
