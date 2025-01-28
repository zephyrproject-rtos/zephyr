# Copyright (c) 2025 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

import logging

from scripts.pm.power_monitor_stm32l562e_dk.UnityFunctions import UnityFunctions


def test_residency_time(power_monitor_measure, expected_values):
    data = power_monitor_measure
    pm_states_expected_values = expected_values.items()

    value_in_amps = UnityFunctions.calculate_rms(data, exclude_first_600=True, num_peaks=6)
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
    state_labels = ["k_cpu_idle", "state_0", "state_0", "state_1", "state_1", "state_2", "active"]

    # Convert dict_items to dictionary if needed
    pm_states_expected_values = dict(pm_states_expected_values)
    # Now you can use indexing or key-based access
    for state_label, rms_value in zip(state_labels, value_in_miliamps, strict=False):
        rms_expected_value = pm_states_expected_values[state_label]['rms']
        print_state_rms(state_label, rms_expected_value, rms_value)
