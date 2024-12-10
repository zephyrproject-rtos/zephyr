# Copyright (c) 2024 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0
import numpy as np
from scipy import signal

def current_RMS(data):
    peaks, data = find_peaks(data)
    # Adding the start (0) and end (length of data) to the list of indices
    indices = np.concatenate(([0], np.array(peaks), [len(data)]))

    # Splitting the data based on index ranges
    split_data = [data[indices[i]+40:indices[i+1]-40] for i in range(len(indices)-1)]

    # Function to calculate RMS for a chunk of data
    def calculate_rms(chunks):
        rms = []
        for chunk in chunks:
            rms_value = np.sqrt(np.mean(np.square(chunk)))
            rms.append(rms_value)
        return rms

    # Calculate RMS for each chunk
    rms_values = calculate_rms(split_data)

    return rms_values

def find_peaks(data):
    data_trimmed = data[600:] # drop the startup section
    peaks = signal.find_peaks(data_trimmed, distance=40, height=0.01)[0]
    return peaks, data_trimmed
