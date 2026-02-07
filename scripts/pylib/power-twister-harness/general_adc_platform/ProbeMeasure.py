# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass


@dataclass
class Reading:
    """Individual ADC reading with raw value and voltage."""

    raw: int
    voltage_mv: int


@dataclass
class ChannelReadings:
    """Readings for a single ADC channel."""

    sample_count: int
    readings: list[Reading]

    @classmethod
    def from_dict(cls, data: dict) -> 'ChannelReadings':
        """Create ChannelReadings instance from dictionary."""
        readings = [
            Reading(raw=reading['raw'], voltage_mv=reading['voltage_mv'])
            for reading in data['readings']
        ]

        return cls(sample_count=data['sample_count'], readings=readings)

    def to_dict(self) -> dict:
        """Convert ChannelReadings instance to dictionary."""
        return {
            'sample_count': self.sample_count,
            'readings': [
                {'raw': reading.raw, 'voltage_mv': reading.voltage_mv} for reading in self.readings
            ],
        }

    def get_average_voltage(self) -> float:
        """Calculate average voltage from all readings, excluding highest and lowest values."""
        if not self.readings:
            return 0.0
        if len(self.readings) <= 2:
            # If we have 2 or fewer readings, return the average of all
            return sum(reading.voltage_mv for reading in self.readings) / len(self.readings)

        # Get all voltage values and sort them
        voltages = [reading.voltage_mv for reading in self.readings]
        voltages.sort()

        # Remove the lowest and highest values
        trimmed_voltages = voltages[1:-1]

        return sum(trimmed_voltages) / len(trimmed_voltages)

    def get_average_raw(self) -> float:
        """Calculate average raw value from all readings, excluding highest and lowest values."""
        if not self.readings:
            return 0.0
        if len(self.readings) <= 2:
            # If we have 2 or fewer readings, return the average of all
            return sum(reading.raw for reading in self.readings) / len(self.readings)

        # Get all raw values and sort them
        raw_values = [reading.raw for reading in self.readings]
        raw_values.sort()

        # Remove the lowest and highest values
        trimmed_raw_values = raw_values[1:-1]

        return sum(trimmed_raw_values) / len(trimmed_raw_values)


@dataclass
class ADCDevice:
    """Readings for a single ADC device with multiple channels."""

    channels: dict[str, ChannelReadings]

    @classmethod
    def from_dict(cls, data: dict) -> 'ADCDevice':
        """Create ADCDevice instance from dictionary."""
        channels = {}
        for ch_name, channel_data in data.items():
            channels[ch_name] = ChannelReadings.from_dict(channel_data)

        return cls(channels=channels)

    def to_dict(self) -> dict:
        """Convert ADCDevice instance to dictionary."""
        return {
            channel_name: channel_readings.to_dict()
            for channel_name, channel_readings in self.channels.items()
        }

    def get_channel(self, ch_name: str) -> ChannelReadings | None:
        """Get channel readings by name."""
        return self.channels.get(ch_name)


@dataclass
class ADCReadingsData:
    """Complete ADC readings data structure."""

    sequence_number: int
    adcs: dict[str, ADCDevice]

    @classmethod
    def from_dict(cls, data: dict) -> 'ADCReadingsData':
        """Create ADCReadingsData instance from dictionary."""
        adcs = {}
        for adc_key, adc_device_data in data['adcs'].items():
            adcs[adc_key] = ADCDevice.from_dict(adc_device_data)

        return cls(sequence_number=data['sequence_number'], adcs=adcs)

    def to_dict(self) -> dict:
        """Convert ADCReadingsData instance to dictionary."""
        return {
            'sequence_number': self.sequence_number,
            'adcs': {adc_name: adc_device.to_dict() for adc_name, adc_device in self.adcs.items()},
        }

    def get_adc(self, adc_name: str) -> ADCDevice | None:
        """Get ADC device by name."""
        return self.adcs.get(adc_name)

    def get_channel_readings(self, adc_name: str, ch_name: str) -> ChannelReadings | None:
        """Get specific channel readings from specific ADC."""
        adc = self.get_adc(adc_name)
        if adc:
            return adc.get_channel(ch_name)
        return None


# Example usage and utility functions
def calculate_power_consumption(
    voltage_readings: ChannelReadings, current_readings: ChannelReadings
) -> float:
    """Calculate average power consumption from voltage and current readings."""
    avg_voltage = voltage_readings.get_average_voltage() / 1000.0  # Convert to volts
    avg_current = current_readings.get_average_voltage() / 1000.0  # Assuming current in mA
    return avg_voltage * avg_current  # Power in watts


if __name__ == "__main__":
    # Example JSON data
    json_data = {
        "sequence_number": 1,
        "adcs": {
            "ADC_0": {
                "channel_0": {
                    "sample_count": 5,
                    "readings": [
                        {"raw": 36, "voltage_mv": 65},
                        {"raw": 35, "voltage_mv": 63},
                        {"raw": 36, "voltage_mv": 65},
                        {"raw": 35, "voltage_mv": 63},
                        {"raw": 36, "voltage_mv": 65},
                    ],
                },
                "channel_1": {
                    "sample_count": 5,
                    "readings": [
                        {"raw": 0, "voltage_mv": 0},
                        {"raw": 0, "voltage_mv": 0},
                        {"raw": 1, "voltage_mv": 1},
                        {"raw": 0, "voltage_mv": 0},
                        {"raw": 1, "voltage_mv": 1},
                    ],
                },
            }
        },
    }

    # Create ADCReadingsData from JSON
    adc_data = ADCReadingsData.from_dict(json_data)
    print(f"Sequence Number: {adc_data.sequence_number}")

    # Access specific channel readings
    channel_0_readings = adc_data.get_channel_readings("ADC_0", "channel_0")
    if channel_0_readings:
        print(f"Channel 0 average voltage: {channel_0_readings.get_average_voltage():.2f} mV")
        print(f"Channel 0 average raw: {channel_0_readings.get_average_raw():.2f}")

    # Convert back to dictionary
    data_dict = adc_data.to_dict()
    print(f"Converted back to dict: {data_dict}")

    # Print all readings for debugging
    for adc_name, adc_device in adc_data.adcs.items():
        print(f"\nADC: {adc_name}")
        for channel_name, channel_readings in adc_device.channels.items():
            print(f"  {channel_name}: {len(channel_readings.readings)} readings")
            for i, reading in enumerate(channel_readings.readings):
                print(f"    Reading {i + 1}: Raw={reading.raw}, Voltage={reading.voltage_mv}mV")
