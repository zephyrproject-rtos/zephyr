# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import csv
import json
import logging
import os
import re
import sys
import time
from datetime import datetime

import yaml
from twister_harness import Shell

try:
    # Primary import attempt: import PowerMonitor from abstract module
    from abstract.PowerMonitor import PowerMonitor
except ImportError:
    # Fallback import strategy: explicitly add parent directory to sys.path
    # and retry the import. This handles cases where the module path resolution
    # fails due to different execution contexts or Python path configurations
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
    from abstract.PowerMonitor import PowerMonitor

try:
    # Import DeviceAdapter from twister_harness for device communication
    from twister_harness import DeviceAdapter
except ImportError:
    # Fallback: create a dummy DeviceAdapter class if twister_harness is not available
    # This allows the module to be imported and tested in environments where
    # twister_harness is not installed
    class DeviceAdapter:
        """A simple dummy class"""

        def __init__(self):
            pass

        def init(self):
            """dummy init"""

        def __str__(self):
            """String representation of the object."""
            return "DummyClass"


from general_adc_platform.ProbeCap import ProbeCap
from general_adc_platform.ProbeMeasure import ADCDevice, ADCReadingsData, ChannelReadings
from general_adc_platform.ProbeSettings import ProbeSettings


def normalize_name(name):
    '''
    normalize name for path
    '''
    return name.replace('/', '_')


class GeneralPowerShield(PowerMonitor):
    """
    A simple implementation of PowerMonitor for general ADC platforms.
    This implementation simulates power measurements for demonstration purposes.
    In a real implementation, this would interface with actual ADC hardware.
    init process is: __init__-> connect -> init, see conftest.probe_class
    """

    def __init__(self):
        self.device_id = 0
        self.is_initialized = False
        self.sample_rate = 1000  # samples per second
        self.logger = logging.getLogger(__name__)
        self.dft: DeviceAdapter = None
        self.probe_cap: ProbeCap = None
        self.probe_settings: ProbeSettings = None
        self.samples: list(ADCReadingsData) = []
        self.voltage_mv = {}
        self.current_ma = {}
        self.shell_mode = False
        self.shell = None

    def connect(self, dft: DeviceAdapter):
        """Opens the connection using the SerialHandler."""
        self.dft = dft
        time.sleep(0.1)
        shell_mode = os.environ.get('POWER_SHIELD_SHELL', '')
        if shell_mode:
            self.dft.write(b't\n')
            self.dft.readlines_until(
                regex=r"Please specify a subcommand",
                timeout=5,
                print_output=True,
            )
            self.shell_mode = True
            self.shell = Shell(self.dft, prompt="uart:", timeout=2)

        if self.shell_mode and self.shell:
            configs = self.shell.exec_command('adc status', timeout=2.0)
        else:
            self.dft.write(b'\r')
            configs = self.dft.readlines_until(
                regex=r"==== end of adc features ===",
                timeout=5,
                print_output=True,
            )

        if configs:
            self.probe_cap = ProbeCap.from_dict(self._parse_adc_config_log("\n".join(configs)))
            if self.probe_cap and self.probe_cap.channel_count:
                self.logger.info("General ADC Power Monitor connect successfully")
                return

        self.logger.info("General power shield connect failed, no valid response")
        self.logger.info(f"{configs}")

    def init(self, device_id: str = None) -> bool:
        """
        Initialize the power monitor with the specified device ID.

        Args:
            device_id (str): Address/identifier of the power monitor device

        Returns:
            bool: True if initialization successful, False otherwise
        """
        try:
            # Get probe settings path from environment variable
            probe_setting_path = os.environ.get('PROBE_SETTING_PATH', '.')

            # Construct path to probe_settings.yaml
            power_shield_config_path = os.path.join(probe_setting_path, 'probe_settings.yaml')
            # Check if the config file exists
            if not os.path.exists(power_shield_config_path):
                self.logger.warning(
                    "Power shield config file not found at: %s", power_shield_config_path
                )
                # Fallback: look for config file in the same directory as this script
                script_dir = os.path.dirname(os.path.abspath(__file__))
                fallback_config_path = os.path.join(script_dir, 'probe_settings.yaml')

                if os.path.exists(fallback_config_path):
                    power_shield_config_path = fallback_config_path
                    self.logger.info("Found fallback config file at: %s", fallback_config_path)
                else:
                    self.logger.error(
                        "Config file not found in fallback location: %s", fallback_config_path
                    )
                    return False

            # Read and parse the YAML configuration file
            self.probe_settings = ProbeSettings.from_yaml(power_shield_config_path)
            self.logger.info("Loaded power shield configuration from: %s", power_shield_config_path)
            self.logger.info(f"Loaded power shield settings: {self.probe_settings}")

            # Apply configuration settings
            if self.probe_settings and self.probe_settings.device_id:
                self.device_id = device_id

            if self.shell_mode:
                self.logger.info("Power monitor initialized successfully with shell mode")

            # Mark as initialized
            self.is_initialized = True
            self.logger.info(
                "Power monitor initialized successfully with device ID: %s", self.device_id
            )
            return True

        except (OSError, yaml.YAMLError, KeyError) as e:
            self.logger.error("Failed to initialize power monitor: %s", e)
            self.is_initialized = False
            return False

    def disconnect(self):
        """Closes the connection using the SerialHandler."""
        if self.dft:
            self.dft.close()

    def measure(self, duration: int) -> None:
        """
        Start a power measurement for the specified duration.

        Args:
            duration (int): The duration of the measurement in seconds

        Raises:
            RuntimeError: If the monitor is not initialized
        """
        if not self.is_initialized:
            raise RuntimeError("Power monitor not initialized. Call init() first.")

        self.logger.info("Starting power measurement for %d seconds", duration)

        # For this simulation, we just log the action
        self.logger.info("Power measurement started")

        start_time = time.time()
        end_time = start_time + duration

        while time.time() < end_time:
            if self.shell_mode:
                measures = self.shell.exec_command('adc read', timeout=5)
            else:
                self.dft.write(b'M')
                measures = self.dft.readlines_until(
                    regex=r"==== end of reading ===",
                    timeout=5,
                    print_output=True,
                )

            measure_data: dict = self._parse_adc_data_log("\n".join(measures))
            if measure_data:
                self.samples += [ADCReadingsData.from_dict(measure_data)]

            # Optional: Add a small delay to prevent overwhelming the device
            # This can be adjusted based on your hardware requirements
            time.sleep(0.1)  # 100ms delay between measurements

        elapsed_time = time.time() - start_time
        self.logger.info("Power measurement completed. Actual duration: %.2f seconds", elapsed_time)
        self.logger.debug("Total samples collected: %d", len(self.samples))

    def get_data(self, duration: int = 0) -> list[float]:
        """
        Measure current for the specified duration and return the data
        in aligned with the probe caps, the date in one measure depends
        on the adc_power_shield application settings CONFIG_SEQUENCE_SAMPLES,
        and the measure interval is depends on adc hardware and fixed delay
        (100ms) in the application.
        Args:
            duration (int): The numbers of measured data to use,
            if 0, we will use all measured data
        Returns:
            List[float]: An array of measured current values in amperes
        Raises:
            RuntimeError: If the monitor is not initialized
        """
        if not self.is_initialized:
            raise RuntimeError("Power monitor not initialized. Call init() first.")

        # Extract voltage data from samples
        voltage_data = self._extract_voltage_data_from_samples(self.samples)

        # Process routes and calculate current data
        current_data, route_voltage = self._process_routes_and_calculate_current(voltage_data)

        # Store results
        self.voltage_mv = route_voltage
        self.current_ma = current_data
        return current_data

    def _extract_voltage_data_from_samples(self, samples: list) -> dict[str, list[float]]:
        """
        Extract voltage data from all samples for channels in use.

        Returns:
            dict: Voltage data by channel
        """
        voltage_data = {}  # by channel
        channels_in_use = self.probe_settings.channel_set if self.probe_settings else set()

        for sample in samples:
            for _adc_name in sample.adcs:
                adc_device: ADCDevice = sample.get_adc(_adc_name)
                for _channel_name in adc_device.channels:
                    _ch = _channel_name.replace("channel_", "", 1)

                    # Initialize channel list if not exists
                    if _ch not in voltage_data:
                        voltage_data[_ch] = []

                    adc_readings: ChannelReadings = adc_device.get_channel(_channel_name)

                    # Only process channels that are in use (or all if no channel_set defined)
                    if not channels_in_use or int(_ch) in channels_in_use:
                        channel_readings: float = adc_readings.get_average_voltage()
                        voltage_data[_ch].append(channel_readings)

        return voltage_data

    def _process_routes_and_calculate_current(
        self, voltage_data: dict[str, list[float]]
    ) -> tuple[dict, dict]:
        """
        Process all routes and calculate current data.
        Args:
          voltage_data: Voltage data by channel

        Returns:
          tuple: (current_data, route_voltage) dictionaries
        """
        current_data = {}
        route_voltage = {}  # by route name
        for _route in self.probe_settings.routes:
            # Validate route channels
            if not self._validate_route_channels(_route, voltage_data):
                continue

            # Calculate current for this route
            route_current, route_volt = self._calculate_route_current(_route, voltage_data)

            current_data[_route.name] = route_current
            if route_volt:  # Only add if there's voltage data (non-differential mode)
                route_voltage[_route.name] = route_volt

        return current_data, route_voltage

    def _validate_route_channels(self, route, voltage_data: dict[str, list[float]]) -> bool:
        """
        Validate that route channels have data and matching lengths.

        Args:
            route: Route object to validate
            voltage_data: Voltage data by channel

        Returns:
              bool: True if route is valid, False otherwise
        """
        route_channels = {str(route.channels.channels_p), str(route.channels.channels_n)}

        # Check if channels exist in voltage data
        if not route_channels.issubset(voltage_data.keys()):
            self.logger.warning(f"channel data is not measured for {route.name} skip")
            self.logger.warning(f"voltage_data keys {voltage_data.keys()} skip")
            self.logger.warning(f"channels {route_channels}")
            return False

        # Check if channel data lengths match
        channels_p_str = str(route.channels.channels_p)
        channels_n_str = str(route.channels.channels_n)

        if len(voltage_data[channels_p_str]) != len(voltage_data[channels_n_str]):
            self.logger.warning(f"len mismatch {route.name}: {channels_p_str} vs {channels_n_str}")
            self.logger.warning(f"channels_p : {len(voltage_data[channels_p_str])}")
            self.logger.warning(f"channels_n : {len(voltage_data[channels_n_str])}")
            return False

        return True

    def _calculate_route_current(
        self, route, voltage_data: dict[str, list[float]]
    ) -> tuple[list[float], list[float]]:
        """
        Calculate current data for a specific route.

        Args:
            route: Route object
            voltage_data: Voltage data by channel

        Returns:
            tuple: (current_data_list, voltage_data_list)
        """
        current_data_list = []
        voltage_data_list = []

        channels_p_str = str(route.channels.channels_p)
        channels_n_str = str(route.channels.channels_n)

        if route.is_differential:
            # In differential mode _vp equal to _vn
            current_data_list = self._calculate_differential_current(
                route, voltage_data[channels_p_str]
            )
        else:
            # Non-differential mode
            current_data_list, voltage_data_list = self._calculate_non_differential_current(
                route, voltage_data[channels_p_str], voltage_data[channels_n_str]
            )

        return current_data_list, voltage_data_list

    def _calculate_differential_current(self, route, voltage_p_data: list[float]) -> list[float]:
        """
        Calculate current for differential mode route.

        Args:
            route: Route object with calibration and conversion methods
            voltage_p_data: Voltage data from positive channel

        Returns:
            list: Current data in amperes
        """
        current_data = []

        for _vp in voltage_p_data:
            _cal_vp = self.probe_settings.calibration.apply_calibration(_vp)
            _cur_data = route.voltage_to_current(_cal_vp)
            current_data.append(_cur_data)

        return current_data

    def _calculate_non_differential_current(
        self, route, voltage_p_data: list[float], voltage_n_data: list[float]
    ) -> tuple[list[float], list[float]]:
        """
        Calculate current for non-differential mode route.

        Args:
            route: Route object with calibration and conversion methods
            voltage_p_data: Voltage data from positive channel
            voltage_n_data: Voltage data from negative channel

        Returns:
            tuple: (current_data, voltage_data) lists
        """
        current_data = []
        voltage_data = []

        for _vp, _vn in zip(voltage_p_data, voltage_n_data, strict=False):
            _cal_vp = self.probe_settings.calibration.apply_calibration(_vp)
            _cal_vn = self.probe_settings.calibration.apply_calibration(_vn)
            _cur_data = route.voltage_to_current(abs(_cal_vp - _cal_vn))
            current_data.append(_cur_data)
            voltage_data.append(_cal_vp)

        return current_data, voltage_data

    def dump_power(self, filename: str = None, fake_run: bool = False):
        """
        Calculate and dump power data (P = V × I) for routes to CSV file

        Args:
            filename (str, optional): Output CSV filename. If None, generates timestamp-based name

        Returns:
            bool: True if successful, False otherwise
        """

        if not self.current_ma or not self.voltage_mv:
            self.logger.warning("No current or voltage data available to calculate power")
            return False

        try:
            # Calculate power data (P = V × I)
            power_mw = {}

            # Find common routes between voltage and current data
            common_routes = set(self.voltage_mv.keys()) & set(self.current_ma.keys())

            if not common_routes:
                self.logger.warning("No common routes found between voltage and current data")
                return False

            for route in common_routes:
                voltage_samples = self.voltage_mv[route]
                current_samples = self.current_ma[route]

                # Ensure both have the same number of samples
                min_samples = min(len(voltage_samples), len(current_samples))

                if min_samples == 0:
                    self.logger.warning("No samples available for route %s", route)
                    continue

                power_mw[route] = []
                for i in range(min_samples):
                    # Power = Voltage (mV) × Current (mA) / 1000 = Power (mW)
                    power = (voltage_samples[i] * current_samples[i]) / 1000.0
                    power_mw[route].append(power)

            if not power_mw:
                self.logger.warning("No power data calculated")
                return False

            # Generate filename if not provided
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            if filename is None:
                dump_name = f"unknown_platform_power_data_{timestamp}.csv"
            else:
                dump_name = f"{normalize_name(filename)}_power_data_{timestamp}.csv"

            # Determine the maximum number of samples across all routes
            max_samples = max(len(samples) for samples in power_mw.values())

            if fake_run:
                self.logger.info(f"power_mw {power_mw}")
                return True

            # Write CSV file
            dump_path = os.path.join(self.dft.device_config.build_dir, dump_name)
            with open(dump_path, 'w', newline='', encoding='utf-8') as csvfile:
                # Get route names for headers
                route_names = list(power_mw.keys())

                # Create CSV writer with headers
                writer = csv.writer(csvfile)

                # Write header row
                headers = ['Sample_Index'] + [f'{route}_Power_mW' for route in route_names]
                writer.writerow(headers)

                # Write data rows
                for i in range(max_samples):
                    row = [i]  # Sample index
                    for route in route_names:
                        # Add power value if available, otherwise empty
                        if i < len(power_mw[route]):
                            row.append(f"{power_mw[route][i]:.6f}")  # Format to 6 decimal places
                        else:
                            row.append('')  # Empty cell for missing data
                    writer.writerow(row)

            self.logger.info("Power data successfully dumped to %s", dump_path)
            self.logger.info("Exported %s samples for %s routes", max_samples, len(route_names))

            # Log power statistics
            for route in route_names:
                avg_power = sum(power_mw[route]) / len(power_mw[route])
                max_power = max(power_mw[route])
                min_power = min(power_mw[route])
                self.logger.info(
                    "Route %s: Avg=%.3fmW, Max=%.3fmW, Min=%.3fmW",
                    route,
                    avg_power,
                    max_power,
                    min_power,
                )

            return True

        except OSError as e:
            self.logger.error("Failed to write power data to CSV: %s", e)
            return False
        except (ValueError, KeyError, ZeroDivisionError) as e:
            self.logger.error("Error processing power data: %s", e)
            return False

    def dump_current(self, filename: str = None, fake_run: bool = False):
        """
        Dump current data for routes to CSV file

        Args:
            filename (str, optional): Output CSV filename. If None, generates timestamp-based name

        Returns:
            bool: True if successful, False otherwise
        """

        if not self.current_ma:
            self.logger.warning("No current data available to dump")
            return False

        try:
            # Generate filename if not provided
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            if filename is None:
                dump_name = f"unknown_platform_current_data_{timestamp}.csv"
            else:
                dump_name = f"{normalize_name(filename)}_current_data_{timestamp}.csv"

            # Determine the maximum number of samples across all routes
            if self.current_ma:
                max_samples = max(len(samples) for samples in self.current_ma.values())
            else:
                self.logger.warning("No current samples to dump")
                return False

            if fake_run:
                self.logger.info(f"current_ma {self.current_ma}")
                return True

            # Write CSV file
            dump_path = os.path.join(self.dft.device_config.build_dir, dump_name)
            with open(dump_path, 'w', newline='', encoding='utf-8') as csvfile:
                # Get route names for headers
                route_names = list(self.current_ma.keys())

                # Create CSV writer with headers
                writer = csv.writer(csvfile)

                # Write header row
                headers = ['Sample_Index'] + [f'{route}_Current_mA' for route in route_names]
                writer.writerow(headers)

                # Write data rows
                for i in range(max_samples):
                    row = [i]  # Sample index
                    for route in route_names:
                        # Add current value if available, otherwise empty
                        if i < len(self.current_ma[route]):
                            row.append(self.current_ma[route][i])
                        else:
                            row.append('')
                    writer.writerow(row)

            self.logger.info(f"Current data dumped to {dump_path}")
            return True

        except Exception as e:
            self.logger.error(f"Failed to dump current data: {e}")
            return False

    def dump_voltage(self, filename: str = None, fake_run: bool = False):
        """
        Dump voltage data for routes to CSV file

        Args:
            filename (str, optional): Output CSV filename. If None, generates timestamp-based name

        Returns:
            bool: True if successful, False otherwise
        """

        if not self.voltage_mv:
            self.logger.warning("No voltage data available to dump")
            return False

        try:
            # Generate filename if not provided
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            if filename is None:
                dump_name = f"unknown_platform_voltage_data_{timestamp}.csv"
            else:
                dump_name = f"{normalize_name(filename)}_voltage_data_{timestamp}.csv"

            # Determine the maximum number of samples across all routes
            if self.voltage_mv:
                max_samples = max(len(samples) for samples in self.voltage_mv.values())
            else:
                self.logger.warning("No voltage samples to dump")
                return False

            if fake_run:
                self.logger.info(f"voltage {self.voltage_mv}")
                return True

            # Write CSV file
            dump_path = os.path.join(self.dft.device_config.build_dir, dump_name)
            with open(dump_path, 'w', newline='', encoding='utf-8') as csvfile:
                # Get route names for headers
                route_names = list(self.voltage_mv.keys())

                # Create CSV writer with headers
                writer = csv.writer(csvfile)

                # Write header row
                headers = ['Sample_Index'] + [f'{route}_Voltage_mV' for route in route_names]
                writer.writerow(headers)

                # Write data rows
                for i in range(max_samples):
                    row = [i]  # Sample index
                    for route in route_names:
                        # Add voltage value if available, otherwise empty
                        if i < len(self.voltage_mv[route]):
                            row.append(self.voltage_mv[route][i])
                        else:
                            row.append('')  # Empty cell for missing data
                    writer.writerow(row)

            self.logger.info(f"Voltage data successfully dumped to {dump_path}")
            self.logger.info(f"Exported {max_samples} samples for {len(route_names)} routes")
            return True

        except OSError as e:
            self.logger.error(f"Failed to write voltage data to CSV: {e}")
            return False
        except Exception as e:
            self.logger.error(f"Unexpected error while dumping voltage data: {e}")
            return False

    def close(self) -> None:
        """
        Close the power monitor connection and cleanup resources.
        """
        if self.is_initialized:
            self.logger.info("Closing General ADC Power Monitor")

            # In a real implementation, you would:
            # - Stop any ongoing measurements
            # - Close device connections
            # - Free allocated resources
            # - Reset hardware to default state
            self.disconnect()
            self.is_initialized = False
            self.device_id = None
            self.logger.info("General ADC Power Monitor closed")

    def get_device_info(self) -> dict:
        """
        Get information about the connected device.

        Returns:
            dict: Device information
        """
        return self.probe_cap.to_dict()

    @staticmethod
    def _parse_adc_config_log(log_text: str) -> dict:
        """
        Parse ADC features log and convert to dictionary format
        """
        result = {}

        # Extract channel count and resolution
        channel_count_match = re.search(r'CHANNEL_COUNT:\s*(\d+)', log_text)
        resolution_match = re.search(r'Resolution:\s*(\d+)', log_text)

        if channel_count_match:
            result['channel_count'] = int(channel_count_match.group(1))

        if resolution_match:
            result['resolution'] = int(resolution_match.group(1))

        # Extract channel information
        result['channels'] = {}

        # Find all channel blocks
        channel_pattern = r'channel_id\s+(\d+)\s+features:(.*?)(?=channel_id\s+\d+|$)'
        channel_matches = re.findall(channel_pattern, log_text, re.DOTALL)

        for channel_id, features_text in channel_matches:
            channel_id = int(channel_id)
            result['channels'][channel_id] = {}

            # Parse features for each channel
            # Extract mode
            if 'is single mode' in features_text:
                result['channels'][channel_id]['mode'] = 'single'
            elif 'is diff mode' in features_text:
                result['channels'][channel_id]['mode'] = 'diff'

            # Extract verf (voltage reference)
            verf_match = re.search(r'verf is (\d+)\s*mv', features_text)
            if verf_match:
                result['channels'][channel_id]['verf_mv'] = int(verf_match.group(1))

        return result

    @staticmethod
    def _parse_adc_data_log(log_text: str) -> dict:
        """
        Parse ADC sequence reading log into a structured dictionary.

        Args:
            log_text: The raw log text containing ADC readings

        Returns:
            Dictionary with parsed ADC data
        """
        result = {"sequence_number": None, "adcs": {}}

        lines = log_text.strip().split('\n')
        current_adc = None
        current_channel = None

        for line in lines:
            line = line.strip()

            # Parse sequence header
            if line.startswith("ADC sequence reading"):
                sequence_match = re.search(r'\[(\d+)\]', line)
                if sequence_match:
                    result["sequence_number"] = int(sequence_match.group(1))

            # Parse ADC and channel header - Updated to handle actual format
            elif line.startswith("- adc@"):
                # Handle format: "- adc@4003b000, channel 0, 5 sequence samples (differential = 0):"
                adc_match = re.search(
                    r'- adc@([0-9a-fA-F]+),\s*channel\s+(\d+),\s*(\d+)\s+sequence samples', line
                )
                if adc_match:
                    adc_address = adc_match.group(1)
                    channel_num = int(adc_match.group(2))
                    sample_count = int(adc_match.group(3))

                    current_adc = f"adc@{adc_address}"
                    current_channel = channel_num

                    if current_adc not in result["adcs"]:
                        result["adcs"][current_adc] = {}

                    result["adcs"][current_adc][f"channel_{current_channel}"] = {
                        "sample_count": sample_count,
                        "readings": [],
                    }
                else:
                    # Handle incomplete ADC line (like the one at line 635)
                    adc_simple_match = re.search(r'- adc@([0-9a-fA-F]+)', line)
                    if adc_simple_match:
                        adc_address = adc_simple_match.group(1)
                        current_adc = f"adc@{adc_address}"
                        # Initialize if not exists, but don't set current_channel yet
                        if current_adc not in result["adcs"]:
                            result["adcs"][current_adc] = {}

            # Parse individual readings
            elif line.startswith("- - ") and current_adc and current_channel is not None:
                reading_match = re.search(r'- - (\d+) = (\d+)mV', line)
                if reading_match:
                    raw_value = int(reading_match.group(1))
                    voltage_mv = int(reading_match.group(2))

                    result["adcs"][current_adc][f"channel_{current_channel}"]["readings"].append(
                        {"raw": raw_value, "voltage_mv": voltage_mv}
                    )

        return result


def log_setup():
    '''
    log setup
    '''
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(logging.DEBUG)
    formatter = logging.Formatter(
        "%(asctime)s - %(name)s -%(filename)s : %(lineno)d - %(levelname)s - %(message)s"
    )
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    return logger


if __name__ == "__main__":
    log_setup()
    log_text = """==== start of adc features ===
CHANNEL_COUNT: 2
Resolution: 12
channel_id 0 features:
- is single mode
- verf is 3300 mv
channel_id 1 features:
- is single mode
- verf is 3300 mv
==== end of adc features ==="""

    parsed_data = GeneralPowerShield._parse_adc_config_log(log_text)
    print(json.dumps(parsed_data, indent=2))
    log_data = """ADC sequence reading [41]:
- adc@4003b000, channel 0, 5 sequence samples (diff = 0):
- - 155 = 124mV
- - 198 = 159mV
- - 201 = 161mV
- - 198 = 159mV
- - 198 = 159mV
- adc@4003b000, channel 3, 5 sequence samples (diff = 0):
- - 4095 = 3299mV
- - 4095 = 3299mV
- - 4095 = 3299mV
- - 4095 = 3299mV
- - 4095 = 3299mV
- adc@4003b000, channel 4, 5 sequence samples (diff = 0):
- - 2159 = 1739mV
- - 2159 = 1739mV
- - 2160 = 1740mV
- - 2160 = 1740mV
- - 2158 = 1738mV
- adc@4003b000, channel 7, 5 sequence samples (diff = 0):
- - 4095 = 3299mV
- - 4095 = 3299mV
- - 4095 = 3299mV
- - 4095 = 3299mV
- - 4095 = 3299mV
==== end of reading ===
"""
    # Parse with detailed structure
    parsed_measure = GeneralPowerShield._parse_adc_data_log(log_data)
    print("Detailed parsing:")
    print(parsed_measure)
    print("\n" + "=" * 50 + "\n")

    _gps = GeneralPowerShield()
    _gps.init()
    samples = []
    samples += [ADCReadingsData.from_dict(parsed_measure)]
    _gps.samples = samples
    parsed_measure = _gps._extract_voltage_data_from_samples(_gps.samples)
    print(f"{parsed_measure}")
    _gps.get_data()
    assert _gps.dump_voltage(fake_run=True)
    assert _gps.dump_current(fake_run=True)
    assert _gps.dump_power(fake_run=True)
