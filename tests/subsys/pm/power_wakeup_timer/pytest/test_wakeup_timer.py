# Copyright (c) 2025 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

from scripts.pm.power_monitor_stm32l562e_dk.UnityFunctions import UnityFunctions


def test_wakeup_timer(current_measurement_output, expected_values):
    data = current_measurement_output
    pm_states_expected_values = expected_values.items()

    value_in_amps = UnityFunctions.current_RMS(data, exclude_first_600=True, num_peaks=1)
    # Convert to milliamps
    value_in_miliamps = [value * 1e4 for value in value_in_amps]

    def print_state_rms(state_label, rms_expected_value, rms_measured_value):
        tolerance = rms_expected_value / 10

        print(f"{state_label}:")
        print(f"Expected RMS: {rms_expected_value:.2f} mA")
        print(f"Tolerance: {tolerance:.2f} mA")
        print(f"Current RMS: {rms_measured_value:.2f} mA")

        assert (
            (rms_expected_value - tolerance) < rms_measured_value < (rms_expected_value + tolerance)
        ), f"RMS value out of tolerance for {state_label}"

    # Define the state labels
    state_labels = ["state_2", "active"]

    # Convert dict_items to dictionary if needed
    pm_states_expected_values = dict(pm_states_expected_values)
    # Now you can use indexing or key-based access
    for state_label, rms_value in zip(state_labels, value_in_miliamps, strict=False):
        rms_expected_value = pm_states_expected_values[state_label]['rms']
        print_state_rms(state_label, rms_expected_value, rms_value)
