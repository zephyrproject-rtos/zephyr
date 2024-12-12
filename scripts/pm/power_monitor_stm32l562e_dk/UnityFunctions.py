# Copyright: (c)  2024, Intel Corporation
# Author: Arkadiusz Cholewinski <arkadiuszx.cholewinski@intel.com>

import numpy as np


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
