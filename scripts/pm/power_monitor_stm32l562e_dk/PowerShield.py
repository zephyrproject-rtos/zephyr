# Copyright: (c)  2024, Intel Corporation
# Author: Arkadiusz Cholewinski <arkadiuszx.cholewinski@intel.com>

import csv
import logging
import queue
import re
import threading
import time

from scripts.pm.power_monitor_stm32l562e_dk.AbstractPowerMonitor import PowerMonitor
from scripts.pm.power_monitor_stm32l562e_dk.PowerShieldConfig import PowerShieldConf
from scripts.pm.power_monitor_stm32l562e_dk.PowerShieldData import PowerShieldData
from scripts.pm.power_monitor_stm32l562e_dk.SerialHandler import SerialHandler
from scripts.pm.power_monitor_stm32l562e_dk.UnityFunctions import UnityFunctions


class PowerShield(PowerMonitor):
    def __init__(self):
        """
        Initializes the PowerShield.
        """
        self.handler = None
        self.dataQueue = queue.Queue()
        self.acqComplete = False
        self.acqStart = False
        self.target_voltage = None
        self.target_temperature = None
        self.acqTimeoutThread = None
        self.power_shield_conf = PowerShieldConf()
        self.power_shield_data = PowerShieldData()

    def init(self, power_device_path: str):
        """
        Initializes the power monitor by setting up a serial handler
        with the given device path and a baud rate of 3686400.
        """
        self.handler = SerialHandler(power_device_path, 3686400)
        self.connect()
        # self.reset()
        self.take_control()
        self.set_voltage(self.power_shield_conf.target_voltage)
        self.set_format(self.power_shield_conf.data_format)
        self.set_func_mode(self.power_shield_conf.function_mode)
        self.target_temperature = self.get_temperature(self.power_shield_conf.temperature_unit)
        self.target_voltage = self.get_voltage_level()

    def connect(self):
        """Opens the connection using the SerialHandler."""
        self.handler.open()

    def disconnect(self):
        """Closes the connection using the SerialHandler."""
        self.handler.close()

    def send_command(self, command: str, expected_ack: str = None, ack: bool = False) -> str:
        """
        Sends a command to the device, retrieves the response,
        and optionally verifies the acknowledgment.

        :param command: The command to send.
        :param expected_ack: The expected acknowledgment response (e.g., "ack htc").
        :return: The response received from the device.
        """
        if not self.handler.is_open():
            logging.info(f"Error: Connection is not open. Cannot send command: {command}")
            return ""

        logging.debug(f"Sending command: {command}")
        self.handler.send_cmd(command)
        if ack:
            response = self.handler.receive_cmd()
            logging.debug(f"Response: {response}")

            # Check if the response contains the expected acknowledgment
            if expected_ack and expected_ack not in response:
                logging.error(f"Error: Expected acknowledgment '{expected_ack}' not found.")
                return ""

            return response
        return 0

    def test_communication(self):
        """
        Sends a version command to the device.
        """
        if not self.handler.is_open():
            logging.error("Error: Connection is not open. Cannot send version command.")
            return ""
        command = 'version'
        logging.info(f"Sending command: {command}")
        self.handler.send_cmd(command)
        response = self.handler.receive_cmd()
        logging.info(f"Response: {response}")
        return response

    def reset(self):
        """
        Sends the reset command ('PSRST') to the power monitor device,
        closes the connection, waits for the reset process to complete,
        and repeatedly attempts to reconnect until successful.
        """
        command = "psrst"

        if not self.handler.is_open():
            logging.error("Error: Connection is not open. Cannot reset the device.")
            return

        logging.info(f"Sending reset command: {command}")
        self.handler.send_cmd(command)

        # Close the connection
        self.handler.close()
        self.handler.serial_connection = None

        time.sleep(5)
        # Attempt to reopen the connection
        try:
            self.handler.open()
            logging.info("Connection reopened after reset.")
        except Exception as e:
            logging.error(f"Failed to reopen connection after reset: {e}")

    def get_voltage_level(self) -> float:
        """
        Sends the 'volt get' command and returns the voltage value as a float.

        :return: The voltage level as a float.
        """
        command = 'volt get'
        response = self.send_command(command, expected_ack="ack volt get", ack=True)

        # If response contains the expected acknowledgment, extract and return the voltage
        if response:
            parts = response.split()
            try:
                if len(parts) >= 5:
                    # Use regex to find a string that matches the pattern, e.g., "3292-03"
                    match = re.search(r'(\d+)-(\d+)', parts[5])
                    if match:
                        # Extract the base (3292) and exponent (03)
                        base = match.group(1)
                        exponent = match.group(2)

                        # Construct the scientific notation string (e.g., 3292e-03)
                        voltage_str = f"{base}e-{exponent}"

                        # Convert the string into a float
                        voltage = float(voltage_str)

                        # Return the voltage as a float
                        self.target_voltage = round(voltage, 3)
                        return self.target_voltage
            except ValueError:
                logging.error("Error: Could not convert temperature value.")
                return None
        else:
            logging.warning("No response for voltage command.")
        return None

    def get_temperature(self, unit: str = PowerShieldConf.TemperatureUnit.CELCIUS) -> float:
        """
        Sends the temperature command and returns the temperature as a float.

        :param unit: The unit to request the temperature in, either 'degc' or 'degf'.
        :return: The temperature value as a float.
        """
        # Send the temp command with the unit
        response = self.send_command(f"temp {unit}", expected_ack=f"ack temp {unit}", ack=True)

        # If response contains the expected acknowledgment, extract the temperature
        if response:
            try:
                # Example response format: "PowerShield > ack temp degc 28.0"
                parts = response.split()
                if len(parts) >= 5 and parts[5].replace('.', '', 1).isdigit():
                    # Extract temperature and convert to float
                    self.target_temetarute = float(parts[5])
                    logging.info(f"Temperature: {self.target_temetarute} {unit}")
                    return self.target_temetarute
                else:
                    print("Error: Temperature value not found in response.")
                    return None
            except ValueError:
                logging.error("Error: Could not convert temperature value.")
                return None
        else:
            logging.warning("No response for temp command.")
        return None

    def take_control(self) -> str:
        """
        Sends the 'htc' command and verifies the acknowledgment.

        :return: The acknowledgment response or error message.
        """
        return self.send_command("htc", expected_ack="ack htc", ack=True)

    def set_format(self, data_format: str = PowerShieldConf.DataFormat.ASCII_DEC):
        """
        Sets the measurement data format.
        The format can be either ASCII (decimal) or Binary (hexadecimal).

        :param data_format: The data format to set.
                            Options are 'ascii_dec' or 'bin_hexa'.
        :return: None
        """
        # Validate the input format
        if data_format not in vars(PowerShieldConf.DataFormat).values():
            logging.warning(
                f"Error: Invalid format '{data_format}'. "
                "Valid options are 'ascii_dec' or 'bin_hexa'."
            )
            return

        command = f"format {data_format}"
        response = self.send_command(command, expected_ack=f"ack format {data_format}", ack=True)

        # If response contains the expected acknowledgment, the format was set successfully
        if response:
            logging.info(f"Data format set to {data_format}.")
        else:
            logging.warning(f"Failed to set data format to {data_format}.")

    def set_frequency(self, frequency: str):
        """
        Sets the sampling frequency for the measurement.
        The frequency can be any valid value from the list.

        :param frequency: The sampling frequency to set.
        Valid options include:
        {100 k, 50 k, 20 k, 10 k, 5 k, 2 k, 1 k, 500, 200, 100, 50, 20, 10, 5, 2, 1}.

        :return: None
        """
        # Validate the input frequency
        if frequency not in vars(PowerShieldConf.SamplingFrequency).values():
            logging.warning(
                f"Error: Invalid frequency '{frequency}'."
                "Valid options are:"
                "100k, 50k, 20k, 10k, 5k, 2k, 1k, 500, 200, 100, 50, 20, 10, 5, 2, 1."
            )
            return

        command = f"freq {frequency}"
        response = self.send_command(command, expected_ack=f"ack freq {frequency}", ack=True)

        if response:
            logging.info(f"Sampling frequency set to {frequency}.")
        else:
            logging.warning(f"Failed to set sampling frequency to {frequency}.")

    def set_acquire_time(self, acquire_time: str = '0'):
        command = f"acqtime {acquire_time}"
        response = self.send_command(command, expected_ack=f"ack acqtime {acquire_time}", ack=True)

        if response:
            logging.info(f"Acquisition time set to {acquire_time}.")
        else:
            logging.warning(f"Failed to set acquisition time to {acquire_time}.")

    def set_voltage(self, voltage: str):
        command = f"volt {voltage}"
        response = self.send_command(command, expected_ack=f"ack volt {voltage}", ack=True)

        if response:
            logging.info(f"Voltage set to {voltage}.")
        else:
            logging.warning(f"Failed to set voltage to {voltage}.")

    def set_func_mode(self, function_mode: str = PowerShieldConf.FunctionMode.HIGH):
        """
        Sets the acquisition mode for current measurement.
        The function_mode can be either 'optim' or 'high'.

        - 'optim': Priority on current resolution (100 nA - 10 mA) with max freq at 100 kHz.
        - 'high': High current (30 µA - 10 mA), high frequency (50-100 kHz), high resolution.

        :param mode: The acquisition mode. Must be either 'optim' or 'high'.
        :return: None
        """
        # Validate the input format
        if function_mode not in vars(PowerShieldConf.FunctionMode).values():
            logging.warning(
                f"Error: Invalid format '{function_mode}'."
                "Valid options are 'ascii_dec' or 'bin_hexa'."
            )
            return

        command = f"funcmode {function_mode}"
        response = self.send_command(
            command, expected_ack=f"ack funcmode {function_mode}", ack=True
        )

        if response:
            logging.info(f"Data format set to {function_mode}.")
        else:
            logging.warning(f"Failed to set data format to {function_mode}.")

    def acq_data(self):
        """
        Continuously reads data from the serial port and puts it
        into a queue until acquisition is complete.
        """
        logging.info("Started data acquisition...")
        while True:
            # Read the first byte
            first_byte = self.handler.read_bytes(1)
            if len(first_byte) < 1 or self.acqComplete:  # Exit conditions
                logging.info("Stopping data acquisition...")
                return

            # Check if it's metadata
            if first_byte == b'\xf0':  # Metadata marker
                second_byte = self.handler.read_bytes(1)
                # Handle metadata types
                metadata_type = second_byte[0]
                self.handle_metadata(metadata_type)
            else:
                # Not metadata, treat as data
                if self.acqStart:
                    second_byte = self.handler.read_bytes(1)
                    data = []
                    data.append(first_byte)
                    if len(second_byte) < 1 or self.acqComplete:
                        logging.info("Stopping data acquisition...")
                        return
                    data.append(second_byte)
                    amps = UnityFunctions.convert_to_amps(
                        UnityFunctions.bytes_to_twobyte_values(data)
                    )
                    self.dataQueue.put([amps])

    def handle_metadata(self, metadata_type):
        if metadata_type == 0xF1:
            logging.info("Received Metadata: ASCII error message.")
            # self.handle_metadata_error()
        elif metadata_type == 0xF2:
            logging.info("Received Metadata: ASCII information message.")
            # self.handle_metadata_info()
        elif metadata_type == 0xF3:
            logging.info("Received Metadata: Timestamp message.")
            self.handle_metadata_timestamp()
            self.acqStart = True
        elif metadata_type == 0xF4:
            logging.info("Received Metadata: End of acquisition tag.")
            self.handle_metadata_end()
            self.handle_summary()
        elif metadata_type == 0xF5:
            logging.info("Received Metadata: Overcurrent detected.")
            # self.handle_metadata_overcurrent()
        else:
            logging.warning(f"Unknown Metadata Type: {metadata_type:#04x}")

    def handle_summary(self):
        s = ""
        while True:
            # Read the first byte
            x = self.handler.read_bytes(1)
            if len(x) < 1 or x == 0xF0:
                self.acqComplete = True
                return s.replace("\0", "").strip().replace("\r", "").replace("\n\n\n", "\n")
            s += str(x, encoding='ascii', errors='ignore')

    def handle_metadata_end(self):
        """
        Handle metadata end of acquisition message.
        """
        # Read the next 2 bytes
        metadata_bytes = self.handler.read_bytes(2)
        if len(metadata_bytes) < 2:
            logging.warning("Incomplete end of acquisition metadata reveived.")
            return
        # Check for end tags (last 2 bytes)
        end_tag_1 = metadata_bytes[0]
        end_tag_2 = metadata_bytes[1]
        if end_tag_1 != 0xFF or end_tag_2 != 0xFF:
            logging.warning("Invalid metadata end tags received.")
            return

    def handle_metadata_timestamp(self):
        """
        Handle metadata timestamp message. Parses and displays the timestamp and buffer load.
        """
        # Read the next 7 bytes (timestamp + buffer load + end tags)
        metadata_bytes = self.handler.read_bytes(7)
        if len(metadata_bytes) < 7:
            logging.warning("Incomplete timestamp metadata received.")
            return

        # Parse the timestamp (4 bytes, big-endian)
        timestamp_ms = int.from_bytes(metadata_bytes[0:4], byteorder='big', signed=False)
        # Parse the buffer Tx load value (1 byte)
        buffer_load = metadata_bytes[4]
        # Check for end tags (last 2 bytes)
        end_tag_1 = metadata_bytes[5]
        end_tag_2 = metadata_bytes[6]
        if end_tag_1 != 0xFF or end_tag_2 != 0xFF:
            logging.warning("Invalid metadata end tags received.")
            return

        # Display parsed values
        logging.info(f"Metadata Timestamp: {timestamp_ms} ms")
        logging.info(f"Buffer Tx Load: {buffer_load}%")

    def start_measurement(self):
        """
        Starts the measurement by sending the 'start' command. Once the measurement starts,
        data can be received continuously until the 'stop' command is sent.

        :return: None
        """
        command = "start"
        self.acqComplete = False
        self.send_command(command)

        raw_to_file_Thread = threading.Thread(
            target=self.raw_to_file, args=(self.power_shield_conf.output_file,)
        )
        raw_to_file_Thread.start()
        logging.info("Measurement started. Receiving data...")
        self.acq_data()
        raw_to_file_Thread.join()

    def raw_to_file(self, outputFilePath: str):
        # Open a CSV file for writing
        with open(outputFilePath, 'w', newline='') as outputFile:
            writer = csv.writer(outputFile)
            while True:
                if self.dataQueue.empty() and bool(self.acqComplete):
                    outputFile.close()
                    break
                if not self.dataQueue.empty():
                    data = self.dataQueue.get()
                    writer.writerow(data)
                    outputFile.flush()
                else:
                    time.sleep(0.1)

    def measure(self, time: int, freq: str = None, reset: bool = False):
        self.power_shield_conf.acquisition_time = time
        _time, self.power_shield_conf.acquisition_time_unit = UnityFunctions.convert_acq_time(time)

        if reset:
            self.reset()
        self.take_control()
        self.set_format(self.power_shield_conf.data_format)
        if freq is not None:
            self.set_frequency(freq)
        else:
            self.set_frequency(self.power_shield_conf.sampling_freqency)
        self.set_acquire_time(
            UnityFunctions.convert_to_scientific_notation(
                time=_time, unit=self.power_shield_conf.acquisition_time_unit
            )
        )
        self.start_measurement()

    def get_data(self, unit: str = PowerShieldConf.MeasureUnit.RAW_DATA):
        if self.acqComplete:
            # Open the CSV file
            with open(self.power_shield_conf.output_file) as file:
                csv_reader = csv.reader(file)
                for row in csv_reader:
                    self.power_shield_data.data.append(row[0])
                if unit == PowerShieldConf.MeasureUnit.CURRENT_RMS:
                    self.power_shield_data.current_RMS = UnityFunctions.calculate_rms(
                        self.power_shield_data.data
                    )
                    return self.power_shield_data.current_RMS
                elif unit == PowerShieldConf.MeasureUnit.POWER:
                    _delta_time = self.power_shield_conf.acquisition_time
                    self.power_shield_data.power = 0
                    for data in self.power_shield_data.data:
                        self.power_shield_data.power += float(
                            float(data) * float(_delta_time) * float(self.target_voltage)
                        )
                    return self.power_shield_data.power
                elif unit == PowerShieldConf.MeasureUnit.RAW_DATA:
                    return self.power_shield_data.data
                else:
                    logging.warning("Unknown unit of requested data")
        else:
            logging.info("Acquisition not complete.")
        return None
