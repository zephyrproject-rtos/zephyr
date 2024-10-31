# Copyright (c) 2024 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0
from current_analyzer import current_RMS

def test_wakeup_timer(current_measurement_output):
    data = current_measurement_output
    # Empirical RMS values in mA with descriptive keys
    rms_expected = {
        "stop 2": 0.26,  # State 0
        "active": 95,  # State 1
    }

    value_in_amps = current_RMS(data)
    value_in_amps = value_in_amps[0:2]
    # Convert to milliamps
    value_in_miliamps = [value * 1e4 for value in value_in_amps]

    def print_state_rms(state_label, rms_expected_value, rms_measured_value):
        tolerance = rms_expected_value / 10

        print(f"{state_label}:")
        print(f"Expected RMS: {rms_expected_value:.2f} mA")
        print(f"Tolerance: {tolerance:.2f} mA")
        print(f"Current RMS: {rms_measured_value:.2f} mA")

        assert (rms_expected_value - tolerance) < rms_measured_value < (rms_expected_value + tolerance), \
            f"RMS value out of tolerance for {state_label}"

    # Define the state labels
    state_labels = ["stop 2", "active"]

    # Loop through the states and print RMS values
    for state_label, rms_value in zip(state_labels, value_in_miliamps):
        print_state_rms(state_label, rms_expected[state_label], rms_value)
