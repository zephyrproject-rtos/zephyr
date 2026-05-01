# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass


@dataclass
class ChannelConfig:
    """Configuration for a single ADC channel."""

    mode: str
    verf_mv: int


@dataclass
class ProbeCap:
    """Probe capacity configuration."""

    channel_count: int
    resolution: int
    channels: dict[str, ChannelConfig]

    @classmethod
    def from_dict(cls, data: dict) -> 'ProbeCap':
        """Create ADCSettings instance from dictionary."""
        channels = {}
        for channel_id, channel_data in data.get('channels', {}).items():
            channels[channel_id] = ChannelConfig(
                mode=channel_data['mode'], verf_mv=channel_data['verf_mv']
            )

        return cls(
            channel_count=data['channel_count'], resolution=data['resolution'], channels=channels
        )

    def to_dict(self) -> dict:
        """Convert ADCSettings instance to dictionary."""
        channels_dict = {}
        for channel_id, channel_config in self.channels.items():
            channels_dict[channel_id] = {
                'mode': channel_config.mode,
                'verf_mv': channel_config.verf_mv,
            }

        return {
            'channel_count': self.channel_count,
            'resolution': self.resolution,
            'channels': channels_dict,
        }

    def get_channel(self, channel_id: str) -> ChannelConfig | None:
        """Get channel configuration by ID."""
        return self.channels.get(channel_id)

    def add_channel(self, channel_id: str, mode: str, verf_mv: int) -> None:
        """Add a new channel configuration."""
        self.channels[channel_id] = ChannelConfig(mode=mode, verf_mv=verf_mv)
        self.channel_count = len(self.channels)


# Example usage
if __name__ == "__main__":
    # Example JSON data
    json_data = {
        "channel_count": 2,
        "resolution": 12,
        "channels": {
            "0": {"mode": "single", "verf_mv": 3300},
            "1": {"mode": "single", "verf_mv": 3300},
        },
    }

    # Create ADCSettings from JSON
    probe_cap = ProbeCap.from_dict(json_data)
    print(f"ADC Settings: {probe_cap}")

    # Access channel configuration
    channel_0 = probe_cap.get_channel("0")
    if channel_0:
        print(f"Channel 0 - Mode: {channel_0.mode}, Vref: {channel_0.verf_mv}mV")

    # Convert back to dictionary
    settings_dict = probe_cap.to_dict()
    print(f"Back to dict: {settings_dict}")
