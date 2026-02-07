# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import yaml


@dataclass
class ChannelPair:
    """
    Configuration for ADC channel pair (positive and negative).
    for differential type, set channels_p and channels_n the same
    """

    channels_p: int
    channels_n: int

    def __post_init__(self):
        """Validate channel pair configuration."""
        if self.channels_p < 0 or self.channels_n < 0:
            raise ValueError(f"must be non-negative, got p={self.channels_p}, n={self.channels_n}")


@dataclass
class RouteConfig:
    """Configuration for a single measurement route."""

    id: int
    name: str
    shunt_resistor: float  # ohms
    gain: float = 1.0
    type: str = "single"
    channels: ChannelPair = None

    def __post_init__(self):
        """Validate route configuration after initialization."""
        if self.id < 0:
            raise ValueError(f"Route ID must be non-negative, got {self.id}")
        if self.shunt_resistor <= 0:
            raise ValueError(f"Shunt resistor must be positive, got {self.shunt_resistor}")
        if self.gain <= 0:
            raise ValueError(f"Gain must be positive, got {self.gain}")
        if self.type not in ["single", "diff", "differential"]:
            raise ValueError(f"Type must be 'single', 'diff', or 'differential', got {self.type}")

    @property
    def current_scale_factor(self) -> float:
        """Calculate current scaling factor based on shunt resistor and gain."""
        return 1.0 / (self.shunt_resistor * self.gain)

    def voltage_to_current(self, voltage: float) -> float:
        """Convert measured voltage to current using shunt resistor."""
        return voltage / (self.shunt_resistor * self.gain)

    @property
    def is_differential(self) -> bool:
        """Check if this route uses differential measurement."""
        return self.type in ["diff", "differential"]


@dataclass
class CalibrationConfig:
    """Calibration settings for the probe."""

    offset: float = 0.0
    scale: float = 1.0

    def __post_init__(self):
        """Validate calibration configuration."""
        if self.scale == 0:
            raise ValueError("Scale factor cannot be zero")

    def apply_calibration(self, raw_value: float) -> float:
        """Apply calibration to a raw measurement value."""
        return (raw_value + self.offset) * self.scale


@dataclass
class ProbeSettings:
    """Complete probe configuration settings."""

    device_id: str
    routes: list[RouteConfig] = field(default_factory=list)
    calibration: CalibrationConfig = field(default_factory=CalibrationConfig)

    def __post_init__(self):
        """Validate probe settings after initialization."""
        if not self.device_id:
            raise ValueError("Device ID cannot be empty")

        # Check for duplicate route IDs
        route_ids = [route.id for route in self.routes]
        if len(route_ids) != len(set(route_ids)):
            raise ValueError("Duplicate route IDs found")

    @classmethod
    def from_yaml(cls, yaml_path: Path) -> 'ProbeSettings':
        """Load probe settings from YAML file."""
        with open(yaml_path) as file:
            data = yaml.safe_load(file)

        return cls.from_dict(data)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> 'ProbeSettings':
        """Create ProbeSettings from dictionary."""
        # Parse routes
        routes = []
        for route_data in data.get('routes', []):
            # Parse channels
            channels_data = route_data.get('channels', {})
            channels = ChannelPair(
                channels_p=channels_data.get('channels_p', 0),
                channels_n=channels_data.get('channels_n', 0),
            )

            route = RouteConfig(
                id=route_data['id'],
                name=route_data['name'],
                shunt_resistor=route_data['shunt_resistor'],
                gain=route_data.get('gain', 1.0),
                type=route_data.get('type', 'single'),
                channels=channels,
            )
            routes.append(route)

        # Parse calibration
        cal_data = data.get('calibration', {})
        calibration = CalibrationConfig(**cal_data)

        return cls(device_id=data['device_id'], routes=routes, calibration=calibration)

    def to_dict(self) -> dict[str, Any]:
        """Convert ProbeSettings to dictionary."""
        return {
            'device_id': self.device_id,
            'routes': [
                {
                    'id': route.id,
                    'name': route.name,
                    'shunt_resistor': route.shunt_resistor,
                    'gain': route.gain,
                    'type': route.type,
                    'channels': {
                        'channels_p': route.channels.channels_p,
                        'channels_n': route.channels.channels_n,
                    },
                }
                for route in self.routes
            ],
            'calibration': {'offset': self.calibration.offset, 'scale': self.calibration.scale},
        }

    def to_yaml(self, yaml_path: Path) -> None:
        """Save probe settings to YAML file."""
        with open(yaml_path, 'w') as file:
            yaml.dump(self.to_dict(), file, default_flow_style=False, indent=2)

    def get_route_by_id(self, route_id: int) -> RouteConfig | None:
        """Get route configuration by ID."""
        for route in self.routes:
            if route.id == route_id:
                return route
        return None

    def get_route_by_name(self, name: str) -> RouteConfig | None:
        """Get route configuration by name."""
        for route in self.routes:
            if route.name == name:
                return route
        return None

    @property
    def route_count(self) -> int:
        """Get total number of configured routes."""
        return len(self.routes)

    @property
    def route_names(self) -> list[str]:
        """Get list of all route names."""
        return [route.name for route in self.routes]

    @property
    def channel_set(self) -> set[int]:
        """Get list of all channels"""
        channels = []
        for _route in self.routes:
            channels += [_route.channels.channels_p, _route.channels.channels_n]

        return set(channels)

    def validate_measurement_data(self, data: dict[int, list[float]]) -> bool:
        """Validate that measurement data matches configured routes."""
        configured_ids = {route.id for route in self.routes}
        data_ids = set(data.keys())

        if not data_ids.issubset(configured_ids):
            missing_configs = data_ids - configured_ids
            raise ValueError(f"Measurement data contains unconfigured routes: {missing_configs}")

        return True


# Example usage and factory functions
def load_default_probe_settings() -> ProbeSettings:
    """Load default probe settings for ADC power monitor."""
    return ProbeSettings(
        device_id="adc_power_monitor_01",
        routes=[
            RouteConfig(
                id=0,
                name="VDD_CORE",
                shunt_resistor=0.1,
                gain=1.0,
                type="single",
                channels=ChannelPair(channels_p=0, channels_n=3),
            ),
            RouteConfig(
                id=1,
                name="VDD_IO",
                shunt_resistor=0.1,
                gain=1.0,
                type="single",
                channels=ChannelPair(channels_p=4, channels_n=7),
            ),
        ],
        calibration=CalibrationConfig(offset=0.0, scale=1.0),
    )


def create_probe_settings_template(output_path: Path) -> None:
    """Create a template probe settings YAML file."""
    template = load_default_probe_settings()
    template.to_yaml(output_path)


# Example usage
if __name__ == "__main__":
    # Load from YAML
    settings_path = Path("probe_settings.yaml")

    try:
        probe_settings = ProbeSettings.from_yaml(settings_path)
        print(f"Loaded settings for device: {probe_settings.device_id}")
        print(f"Configured routes: {probe_settings.route_names}")

        # Access specific route
        core_route = probe_settings.get_route_by_name("VDD_CORE")
        if core_route:
            print(f"VDD_CORE shunt resistor: {core_route.shunt_resistor} ohms")
            print(
                "VDD_CORE channels: P={core_route.channels.channels_p},"
                + f"N={core_route.channels.channels_n}"
            )
            print(f"VDD_CORE type: {core_route.type}")
            print(f"VDD_CORE is differential: {core_route.is_differential}")

            # Convert voltage measurement to current
            voltage_reading = 0.05  # 50mV
            current = core_route.voltage_to_current(voltage_reading)
            print(f"Current: {current:.3f} A")

        # Show all routes
        for route in probe_settings.routes:
            print(
                f"Route {route.id}: {route.name} - P:{route.channels.channels_p},"
                + f" N:{route.channels.channels_n} ({route.type})"
            )

    except FileNotFoundError:
        print("Creating default probe settings...")
        default_settings = load_default_probe_settings()
        default_settings.to_yaml(settings_path)
        print(f"Default settings saved to {settings_path}")
