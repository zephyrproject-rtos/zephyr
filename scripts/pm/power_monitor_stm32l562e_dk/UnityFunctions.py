# Copyright: (c)  2024, Intel Corporation
# Author: Arkadiusz Cholewinski <arkadiuszx.cholewinski@intel.com>

import numpy as np
from scipy import signal


class UnityFunctions:
    @staticmethod
    def convert_acq_time(value):
        """
        Converts an acquisition time value to a more readable format with units.
        - Converts values to m (milli), u (micro), or leaves them as is for whole numbers.

        :param value: The numeric value to convert.
        :return: A tuple with the value and the unit as separate elements.
        """
        if value < 1e-3:
            # If the value is smaller than 1 millisecond (10^-3), express in micro (u)
            return f"{value * 1e6:.0f}", "us"
        elif value < 1:
            # If the value is smaller than 1 but larger than or equal to 1 milli (10^-3)
            return f"{value * 1e3:.0f}", "ms"
        else:
            # If the value is 1 or larger, express in seconds (s)
            return f"{value:.0f}", "s"

    @staticmethod
    def calculate_rms(data):
        """
        Calculate the Root Mean Square (RMS) of a given data array.

        :param data: List or numpy array containing the data
        :return: RMS value
        """
        # Convert to a numpy array for easier mathematical operations
        data_array = np.array(data, dtype=np.float64)  # Convert to float64 to avoid type issues

        # Calculate the RMS value
        rms = np.sqrt(np.mean(np.square(data_array)))
        return rms

    @staticmethod
    def bytes_to_twobyte_values(data):
        value = int.from_bytes(data[0], 'big') << 8 | int.from_bytes(data[1], 'big')
        return value

    @staticmethod
    def convert_to_amps(value):
        """
        Convert amps to watts
        """
        amps = (value & 4095) * (16 ** (0 - (value >> 12)))
        return amps

    @staticmethod
    def convert_to_scientific_notation(time: int, unit: str) -> str:
        """
        Converts time to scientific notation based on the provided unit.
        :param time: The time value to convert.
        :param unit: The unit of the time ('us', 'ms', or 's').
        :return: A string representing the time in scientific notation.
        """
        if unit == 'us':  # microseconds
            return f"{time}-6"
        elif unit == 'ms':  # milliseconds
            return f"{time}-3"
        elif unit == 's':  # seconds
            return f"{time}"
        else:
            raise ValueError("Invalid unit. Use 'us', 'ms', or 's'.")

    @staticmethod
    def current_RMS(data, exclude_first_600=True, num_peaks=1):
        """
        Function to process a given data array, find peaks, split data into chunks,
        and then compute the RMS value for each chunk. The first 600 elements can be excluded
        based on the parameter, and the number of peaks to consider is configurable.

        Args:
        - data (list or numpy array): The input data for RMS calculation.
        - exclude_first_600 (bool): Whether to exclude the first 600 elements of the data.
        - num_peaks (int): The number of peaks to consider for chunking.

        Returns:
        - rms_values (list): A list of RMS values calculated from the data chunks.
        """

        # Optionally exclude the first 600 elements of the data
        if exclude_first_600:
            data = data[600:]

        # Convert the data to a list of floats for consistency
        data = [float(x) for x in data]

        # Find the peaks in the data using the `find_peaks` function
        peaks = signal.find_peaks(data, distance=40, height=0.008)[0]

        # Check if we have enough peaks, otherwise raise an exception
        if len(peaks) < num_peaks:
            raise ValueError(
                f"Not enough peaks detected. Expected at least {num_peaks}, but found {len(peaks)}."
            )

        # Limit the number of peaks based on the `num_peaks` parameter
        peaks = peaks[:num_peaks]

        # Add the start (index 0) and end (index of the last data point) to the list of peak indices
        indices = np.concatenate(([0], np.array(peaks), [len(data)]))

        # Split the data into chunks based on the peak indices
        # with padding of 40 elements at both ends
        split_data = [data[indices[i] + 40 : indices[i + 1] - 40] for i in range(len(indices) - 1)]

        # Function to calculate RMS for a given list of data chunks
        def calculate_rms(chunks):
            """
            Helper function to compute RMS values for each data chunk.

            Args:
            - chunks (list): A list of data chunks.

            Returns:
            - rms (list): A list of RMS values, one for each data chunk.
            """
            rms = []
            for chunk in chunks:
                # Calculate RMS by taking the square root of the mean of squared values
                rms_value = np.sqrt(np.mean(np.square(chunk)))
                rms.append(rms_value)
            return rms

        # Calculate RMS for each chunk of data
        rms_values = calculate_rms(split_data)

        # Return the calculated RMS values
        return rms_values
