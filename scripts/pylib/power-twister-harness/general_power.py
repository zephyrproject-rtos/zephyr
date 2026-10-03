# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import csv
import logging
import os
import re
import sys
import time
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any

import yaml
from twister_harness import DeviceAdapter, Shell

try:
    from abstract.PowerMonitor import PowerMonitor
except ImportError:
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
    from abstract.PowerMonitor import PowerMonitor


def normalize_name(name: str) -> str:
    """Normalize a platform name for generated file paths."""
    return name.replace('/', '_')


@dataclass
class ChannelConfig:
    """Configuration for a single ADC channel."""

    mode: str
    verf_mv: int


@dataclass
class ProbeCap:
    """General ADC capability description."""

    channel_count: int
    resolution: int
    channels: dict[int, ChannelConfig]

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> ProbeCap:
        channels: dict[int, ChannelConfig] = {}
        for channel_id, channel_data in data.get('channels', {}).items():
            channels[int(channel_id)] = ChannelConfig(
                mode=channel_data['mode'],
                verf_mv=channel_data['verf_mv'],
            )
        return cls(
            channel_count=data['channel_count'],
            resolution=data['resolution'],
            channels=channels,
        )


@dataclass
class Reading:
    """Single ADC reading with raw and converted values."""

    raw: int
    voltage_mv: int


@dataclass
class ChannelReadings:
    """Readings captured for one ADC channel."""

    sample_count: int
    readings: list[Reading]

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> ChannelReadings:
        return cls(
            sample_count=data['sample_count'],
            readings=[
                Reading(raw=item['raw'], voltage_mv=item['voltage_mv']) for item in data['readings']
            ],
        )

    def get_average_voltage(self) -> float:
        """Return an average voltage with min/max trimming when possible."""
        if not self.readings:
            return 0.0
        values = [reading.voltage_mv for reading in self.readings]
        if len(values) <= 2:
            return sum(values) / len(values)
        values.sort()
        trimmed = values[1:-1]
        return sum(trimmed) / len(trimmed)


@dataclass
class ADCDevice:
    """ADC readings grouped by channel name."""

    channels: dict[str, ChannelReadings]

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> ADCDevice:
        return cls(
            channels={
                name: ChannelReadings.from_dict(channel_data) for name, channel_data in data.items()
            },
        )

    def get_channel(self, channel_name: str) -> ChannelReadings | None:
        return self.channels.get(channel_name)


@dataclass
class ADCReadingsData:
    """One sequence of ADC readings collected from the monitor DUT."""

    sequence_number: int
    adcs: dict[str, ADCDevice]

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> ADCReadingsData:
        return cls(
            sequence_number=data['sequence_number'],
            adcs={
                adc_name: ADCDevice.from_dict(adc_data)
                for adc_name, adc_data in data['adcs'].items()
            },
        )

    def get_adc(self, adc_name: str) -> ADCDevice | None:
        return self.adcs.get(adc_name)


@dataclass
class ChannelPair:
    """Positive and negative ADC channel pair for one route."""

    channels_p: int
    channels_n: int


@dataclass
class RouteConfig:
    """Current route configuration for a monitored rail."""

    id: int
    name: str
    shunt_resistor: float
    gain: float = 1.0
    type: str = 'single'
    channels: ChannelPair | None = None

    @property
    def is_differential(self) -> bool:
        return self.type in {'diff', 'differential'}

    def voltage_to_current(self, voltage: float) -> float:
        return voltage / (self.shunt_resistor * self.gain)


@dataclass
class CalibrationConfig:
    """Calibration settings applied to measured voltages."""

    offset: float = 0.0
    scale: float = 1.0

    def apply_calibration(self, raw_value: float) -> float:
        return (raw_value + self.offset) * self.scale


@dataclass
class ProbeSettings:
    """General ADC probe configuration."""

    device_id: str
    routes: list[RouteConfig] = field(default_factory=list)
    calibration: CalibrationConfig = field(default_factory=CalibrationConfig)

    @classmethod
    def from_yaml(cls, yaml_path: str | Path) -> ProbeSettings:
        with open(yaml_path, encoding='utf-8') as file:
            data = yaml.safe_load(file)
        return cls.from_dict(data)

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> ProbeSettings:
        routes: list[RouteConfig] = []
        for route_data in data.get('routes', []):
            channels_data = route_data.get('channels', {})
            routes.append(
                RouteConfig(
                    id=route_data['id'],
                    name=route_data['name'],
                    shunt_resistor=route_data['shunt_resistor'],
                    gain=route_data.get('gain', 1.0),
                    type=route_data.get('type', 'single'),
                    channels=ChannelPair(
                        channels_p=channels_data.get('channels_p', 0),
                        channels_n=channels_data.get('channels_n', 0),
                    ),
                )
            )
        calibration = CalibrationConfig(**data.get('calibration', {}))
        return cls(
            device_id=data['device_id'],
            routes=routes,
            calibration=calibration,
        )

    @property
    def channel_set(self) -> set[int]:
        channels: set[int] = set()
        for route in self.routes:
            channels.add(route.channels.channels_p)
            channels.add(route.channels.channels_n)
        return channels


def default_probe_settings() -> ProbeSettings:
    """Return built-in settings for the general ADC monitor path."""
    return ProbeSettings(
        device_id='general_adc_power_monitor',
        routes=[
            RouteConfig(
                id=0,
                name='route_0',
                shunt_resistor=0.1,
                gain=1.0,
                type='single',
                channels=ChannelPair(channels_p=0, channels_n=1),
            ),
        ],
        calibration=CalibrationConfig(offset=0.0, scale=1.0),
    )


def load_probe_settings() -> ProbeSettings:
    """Load settings from PROBE_SETTING_PATH or use built-in defaults."""
    probe_setting_path = os.environ.get('PROBE_SETTING_PATH', '')
    if probe_setting_path:
        config_path = os.path.join(probe_setting_path, 'probe_settings.yaml')
        if os.path.exists(config_path):
            return ProbeSettings.from_yaml(config_path)
    return default_probe_settings()


class GeneralPowerShield(PowerMonitor):
    """Power monitor backed by a second DUT running a general ADC image."""

    def __init__(self):
        self.device_id = 0
        self.is_initialized = False
        self.logger = logging.getLogger(__name__)
        self.dft: DeviceAdapter | None = None
        self.probe_cap: ProbeCap | None = None
        self.probe_settings: ProbeSettings | None = None
        self.samples: list[ADCReadingsData] = []
        self.voltage_mv: dict[str, list[float]] = {}
        self.current_ma: dict[str, list[float]] = {}
        self.shell_mode = False
        self.shell: Shell | None = None

    def connect(self, dft: DeviceAdapter):
        """Connect to the monitor DUT and read its ADC capabilities."""
        self.dft = dft
        time.sleep(0.1)
        shell_mode = os.environ.get('POWER_SHIELD_SHELL', '')
        if shell_mode:
            self.dft.write(b't\n')
            self.dft.readlines_until(
                regex=r'Please specify a subcommand',
                timeout=5,
                print_output=True,
            )
            self.shell_mode = True
            self.shell = Shell(self.dft, prompt='uart:', timeout=2)

        if self.shell_mode and self.shell:
            configs = self.shell.exec_command('adc status', timeout=2.0)
        else:
            self.dft.write(b'\r')
            configs = self.dft.readlines_until(
                regex=r'==== end of adc features ===',
                timeout=5,
                print_output=True,
            )

        if configs:
            parsed = self._parse_adc_config_log('\n'.join(configs))
            self.probe_cap = ProbeCap.from_dict(parsed)
            if self.probe_cap.channel_count:
                self.logger.info('General ADC power monitor connected successfully')
                return

        self.logger.info('General ADC power monitor connect failed')
        self.logger.info('%s', configs)

    def init(self, device_id: str | None = None) -> bool:
        """Load probe settings and mark the general ADC monitor ready."""
        try:
            self.probe_settings = load_probe_settings()
            if self.probe_settings.device_id:
                self.device_id = device_id
            self.is_initialized = True
            self.logger.info('General ADC power monitor initialized')
            return True
        except (OSError, yaml.YAMLError, KeyError, ValueError) as exc:
            self.logger.error(
                'Failed to initialize general ADC power monitor: %s',
                exc,
            )
            self.is_initialized = False
            return False

    def disconnect(self):
        """Close the monitor DUT connection."""
        if self.dft:
            self.dft.close()

    def measure(self, duration: int) -> None:
        """Collect ADC readings from the monitor DUT for a duration."""
        if not self.is_initialized:
            raise RuntimeError('Power monitor not initialized. Call init() first.')

        self.samples = []
        start_time = time.time()
        end_time = start_time + duration

        while time.time() < end_time:
            if self.shell_mode and self.shell:
                measures = self.shell.exec_command('adc read', timeout=5)
            else:
                self.dft.write(b'M')
                measures = self.dft.readlines_until(
                    regex=r'==== end of reading ===',
                    timeout=5,
                    print_output=True,
                )

            measure_data = self._parse_adc_data_log('\n'.join(measures))
            if measure_data:
                self.samples.append(ADCReadingsData.from_dict(measure_data))
            time.sleep(0.1)

    def get_data(self, duration: int = 0) -> dict[str, list[float]]:
        """Convert captured route voltages to current data."""
        del duration
        if not self.is_initialized:
            raise RuntimeError('Power monitor not initialized. Call init() first.')

        voltage_data = self._extract_voltage_data_from_samples(self.samples)
        current_data, route_voltage = self._process_routes(voltage_data)
        self.voltage_mv = route_voltage
        self.current_ma = current_data
        return current_data

    def dump_power(self, filename: str | None = None, fake_run: bool = False):
        """Write route power samples to CSV."""
        if not self.current_ma or not self.voltage_mv:
            self.logger.warning('No current or voltage data available')
            return False

        power_mw: dict[str, list[float]] = {}
        common_routes = set(self.voltage_mv) & set(self.current_ma)
        if not common_routes:
            self.logger.warning('No common routes found for power dump')
            return False

        for route in common_routes:
            voltage_samples = self.voltage_mv[route]
            current_samples = self.current_ma[route]
            count = min(len(voltage_samples), len(current_samples))
            if count == 0:
                continue
            power_mw[route] = [
                (voltage_samples[i] * current_samples[i]) / 1000.0 for i in range(count)
            ]

        if not power_mw:
            self.logger.warning('No power data calculated')
            return False

        if fake_run:
            self.logger.info('power_mw %s', power_mw)
            return True

        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        dump_name = (
            f'{normalize_name(filename)}_power_data_{timestamp}.csv'
            if filename
            else f'unknown_platform_power_data_{timestamp}.csv'
        )
        dump_path = os.path.join(self.dft.device_config.build_dir, dump_name)
        self._write_route_csv(dump_path, power_mw, 'Power_mW')
        self.logger.info('Power data dumped to %s', dump_path)
        return True

    def dump_current(self, filename: str | None = None, fake_run: bool = False):
        """Write route current samples to CSV."""
        if not self.current_ma:
            self.logger.warning('No current data available to dump')
            return False
        if fake_run:
            self.logger.info('current_ma %s', self.current_ma)
            return True
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        dump_name = (
            f'{normalize_name(filename)}_current_data_{timestamp}.csv'
            if filename
            else f'unknown_platform_current_data_{timestamp}.csv'
        )
        dump_path = os.path.join(self.dft.device_config.build_dir, dump_name)
        self._write_route_csv(dump_path, self.current_ma, 'Current_mA')
        self.logger.info('Current data dumped to %s', dump_path)
        return True

    def dump_voltage(self, filename: str | None = None, fake_run: bool = False):
        """Write route voltage samples to CSV."""
        if not self.voltage_mv:
            self.logger.warning('No voltage data available to dump')
            return False
        if fake_run:
            self.logger.info('voltage_mv %s', self.voltage_mv)
            return True
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        dump_name = (
            f'{normalize_name(filename)}_voltage_data_{timestamp}.csv'
            if filename
            else f'unknown_platform_voltage_data_{timestamp}.csv'
        )
        dump_path = os.path.join(self.dft.device_config.build_dir, dump_name)
        self._write_route_csv(dump_path, self.voltage_mv, 'Voltage_mV')
        self.logger.info('Voltage data dumped to %s', dump_path)
        return True

    def _write_route_csv(
        self,
        dump_path: str,
        route_data: dict[str, list[float]],
        suffix: str,
    ) -> None:
        """Write one route-based CSV file."""
        route_names = list(route_data.keys())
        max_samples = max(len(samples) for samples in route_data.values())
        with open(dump_path, 'w', newline='', encoding='utf-8') as csvfile:
            writer = csv.writer(csvfile)
            headers = ['Sample_Index'] + [f'{route}_{suffix}' for route in route_names]
            writer.writerow(headers)
            for index in range(max_samples):
                row: list[float | str | int] = [index]
                for route in route_names:
                    if index < len(route_data[route]):
                        row.append(route_data[route][index])
                    else:
                        row.append('')
                writer.writerow(row)

    def _extract_voltage_data_from_samples(
        self,
        samples: list[ADCReadingsData],
    ) -> dict[str, list[float]]:
        """Extract averaged voltage data by channel from captured samples."""
        voltage_data: dict[str, list[float]] = {}
        channels_in_use = self.probe_settings.channel_set if self.probe_settings else set()

        for sample in samples:
            for adc_name in sample.adcs:
                adc_device = sample.get_adc(adc_name)
                for channel_name in adc_device.channels:
                    channel_id = channel_name.replace('channel_', '', 1)
                    voltage_data.setdefault(channel_id, [])
                    if channels_in_use and int(channel_id) not in channels_in_use:
                        continue
                    adc_readings = adc_device.get_channel(channel_name)
                    voltage_data[channel_id].append(adc_readings.get_average_voltage())
        return voltage_data

    def _process_routes(
        self,
        voltage_data: dict[str, list[float]],
    ) -> tuple[dict[str, list[float]], dict[str, list[float]]]:
        """Convert voltage samples for configured routes into currents."""
        current_data: dict[str, list[float]] = {}
        route_voltage: dict[str, list[float]] = {}
        for route in self.probe_settings.routes:
            channels_p = str(route.channels.channels_p)
            channels_n = str(route.channels.channels_n)
            route_channels = {channels_p, channels_n}
            if not route_channels.issubset(voltage_data.keys()):
                self.logger.warning('Missing channel data for route %s', route.name)
                continue
            if len(voltage_data[channels_p]) != len(voltage_data[channels_n]):
                self.logger.warning('Sample length mismatch for route %s', route.name)
                continue

            if route.is_differential:
                current_data[route.name] = [
                    route.voltage_to_current(
                        self.probe_settings.calibration.apply_calibration(value)
                    )
                    for value in voltage_data[channels_p]
                ]
                continue

            currents: list[float] = []
            route_voltages: list[float] = []
            for value_p, value_n in zip(
                voltage_data[channels_p],
                voltage_data[channels_n],
                strict=False,
            ):
                calibrated_p = self.probe_settings.calibration.apply_calibration(value_p)
                calibrated_n = self.probe_settings.calibration.apply_calibration(value_n)
                currents.append(route.voltage_to_current(abs(calibrated_p - calibrated_n)))
                route_voltages.append(calibrated_p)
            current_data[route.name] = currents
            route_voltage[route.name] = route_voltages

        return current_data, route_voltage

    @staticmethod
    def _parse_adc_config_log(log_text: str) -> dict[str, Any]:
        """Parse the monitor DUT ADC capability log into a dictionary."""
        result: dict[str, Any] = {'channels': {}}
        channel_count_match = re.search(r'CHANNEL_COUNT:\s*(\d+)', log_text)
        resolution_match = re.search(r'Resolution:\s*(\d+)', log_text)
        if channel_count_match:
            result['channel_count'] = int(channel_count_match.group(1))
        if resolution_match:
            result['resolution'] = int(resolution_match.group(1))

        channel_pattern = r'channel_id\s+(\d+)\s+features:(.*?)(?=channel_id\s+\d+|$)'
        for channel_id, features_text in re.findall(
            channel_pattern,
            log_text,
            re.DOTALL,
        ):
            channel_idx = int(channel_id)
            result['channels'][channel_idx] = {}
            if 'is single mode' in features_text:
                result['channels'][channel_idx]['mode'] = 'single'
            elif 'is diff mode' in features_text:
                result['channels'][channel_idx]['mode'] = 'diff'
            verf_match = re.search(r'verf is (\d+)\s*mv', features_text)
            if verf_match:
                result['channels'][channel_idx]['verf_mv'] = int(verf_match.group(1))
        return result

    @staticmethod
    def _parse_adc_data_log(log_text: str) -> dict[str, Any]:
        """Parse one ADC reading block from the monitor DUT log."""
        result: dict[str, Any] = {'sequence_number': None, 'adcs': {}}
        current_adc = None
        current_channel = None

        for line in log_text.strip().split('\n'):
            line = line.strip()
            if line.startswith('ADC sequence reading'):
                sequence_match = re.search(r'\[(\d+)\]', line)
                if sequence_match:
                    result['sequence_number'] = int(sequence_match.group(1))
                continue

            if line.startswith('- adc@'):
                adc_match = re.search(
                    r'- adc@([0-9a-fA-F]+),\s*channel\s+(\d+),\s*'
                    r'(\d+)\s+sequence samples',
                    line,
                )
                if adc_match:
                    adc_address = adc_match.group(1)
                    current_channel = int(adc_match.group(2))
                    sample_count = int(adc_match.group(3))
                    current_adc = f'adc@{adc_address}'
                    result['adcs'].setdefault(current_adc, {})
                    result['adcs'][current_adc][f'channel_{current_channel}'] = {
                        'sample_count': sample_count,
                        'readings': [],
                    }
                continue

            if line.startswith('- - ') and current_adc and current_channel is not None:
                reading_match = re.search(r'- - (\d+) = (\d+)mV', line)
                if reading_match:
                    result['adcs'][current_adc][f'channel_{current_channel}']['readings'].append(
                        {
                            'raw': int(reading_match.group(1)),
                            'voltage_mv': int(reading_match.group(2)),
                        }
                    )
        return result
