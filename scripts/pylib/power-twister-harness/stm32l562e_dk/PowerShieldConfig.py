# Copyright: (c)  2025, Intel Corporation
# Author: Arkadiusz Cholewinski <arkadiuszx.cholewinski@intel.com>
from dataclasses import dataclass, field


@dataclass
class PowerShieldConf:
    class PowerMode:
        """
        Class representing power mode
        """

        AUTO = "auto"  # Power-on when acquisition start
        ON = "on"  # Power-on manually
        OFF = "off"  # Power-off manually

    class MeasureUnit:
        """
        Class representing measure units.
        """

        VOLTAGE = "voltage"  # Target Volatege
        CURRENT_RMS = "current_rms"  # Current RMS value
        POWER = "power"  # Total power consumption
        RAW_DATA = "rawdata"  # Get Raw Data (current probes)

    class TemperatureUnit:
        """
        Class representing temperature units.
        """

        CELSIUS = "degc"  # Celsius temperature unit
        FAHRENHEIT = "degf"  # Fahrenheit temperature unit

    class FunctionMode:
        """
        Class representing functional modes of a power monitor.
        """

        OPTIM = "optim"  # Optimized mode for lower power or efficiency
        HIGH = "high"  # High performance mode

    class DataFormat:
        """
        Class representing different data formats for representation.
        """

        ASCII_DEC = "ascii_dec"  # ASCII encoded decimal format
        BIN_HEXA = "bin_hexa"  # Binary/hexadecimal format

    class SamplingFrequency:
        """
        Class representing various sampling frequencies.
        """

        FREQ_100K = "100k"  # 100 kHz frequency
        FREQ_50K = "50k"  # 50 kHz frequency
        FREQ_20K = "20k"  # 20 kHz frequency
        FREQ_10K = "10k"  # 10 kHz frequency
        FREQ_5K = "5k"  # 5 kHz frequency
        FREQ_2K = "2k"  # 2 kHz frequency
        FREQ_1K = "1k"  # 1 kHz frequency
        FREQ_500 = "500"  # 500 Hz frequency
        FREQ_200 = "200"  # 200 Hz frequency
        FREQ_100 = "100"  # 100 Hz frequency
        FREQ_50 = "50"  # 50 Hz frequency
        FREQ_20 = "20"  # 20 Hz frequency
        FREQ_10 = "10"  # 10 Hz frequency
        FREQ_5 = "5"  # 5 Hz frequency
        FREQ_2 = "2"  # 2 Hz frequency
        FREQ_1 = "1"  # 1 Hz frequency

    output_file: str = "rawData.csv"
    sampling_frequency: str = SamplingFrequency.FREQ_1K
    data_format: str = DataFormat.BIN_HEXA
    function_mode: str = FunctionMode.HIGH
    target_voltage: str = "3300m"
    temperature_unit: str = TemperatureUnit.CELSIUS
    acquisition_time: str = field(default=None, init=False)
