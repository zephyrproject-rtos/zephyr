# Copyright (c) 2024 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0
from current_analyzer import current_RMS
import os

def test_expected_rms_deep_sleep(current_measurement_output):
    data = current_measurement_output
    # Empirical RMS values in mA with descriptive keys
    rms_expected = {
        "k_cpu_idle": 57.0,  # k_cpu_idle
        "state 0": 4.0,  # State 0
        "state 1": 1.2,  # State 1
        "state 2": 0.27, # State 2
        "active": 92.7    # Active
    }

    value_in_amps = current_RMS(data)
    os.rename("measurementData.csv","test_expected_rms_deep_sleep.csv")

    # Convert to milliamps
    value_in_miliamps = [value * 1e4 for value in value_in_amps]
    value_in_miliamps = value_in_miliamps[0:7]

    def print_state_rms(state_label, rms_expected_value, rms_measured_value):
        tolerance = rms_expected_value / 10

        print(f"{state_label}:")
        print(f"Expected RMS: {rms_expected_value:.2f} mA")
        print(f"Tolerance: {tolerance:.2f} mA")
        print(f"Current RMS: {rms_measured_value:.2f} mA")

        assert (rms_expected_value - tolerance) < rms_measured_value < (rms_expected_value + tolerance), \
            f"RMS value out of tolerance for {state_label}"

    # Define the state labels
    state_labels = ["k_cpu_idle", "state 0", "state 0", "state 1", "state 1","state 2", "active"]

    # Loop through the states and print RMS values
    for state_label, rms_value in zip(state_labels, value_in_miliamps):
        print_state_rms(state_label, rms_expected[state_label], rms_value)
