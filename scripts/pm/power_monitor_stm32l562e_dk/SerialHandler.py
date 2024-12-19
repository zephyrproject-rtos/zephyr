# Copyright: (c)  2024, Intel Corporation
# Author: Arkadiusz Cholewinski <arkadiuszx.cholewinski@intel.com>

import logging

import serial


class SerialHandler:
    def __init__(self, port: str, baudrate: int = 9600, timeout: float = 1.0):
        """
        Initializes the class for serial communication.

        :param port: The serial port name (e.g., 'COM1', '/dev/ttyUSB0').
        :param baudrate: The baud rate for the connection (default is 9600).
        :param timeout: The timeout for read operations in seconds (default is 1.0).
        """
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial_connection = None

    def open(self):
        """
        Opens the serial connection.
        """
        if self.serial_connection is None:
            try:
                self.serial_connection = serial.Serial(
                    self.port, self.baudrate, timeout=self.timeout
                )
                logging.info(
                    "Connection to %s at %d baud opened successfully.", self.port, self.baudrate
                )
            except serial.SerialException as e:
                logging.error("Error opening serial port %s: %s", self.port, str(e))
                self.serial_connection = None

    def close(self):
        """Closes the serial connection."""
        if self.serial_connection and self.serial_connection.is_open:
            self.serial_connection.close()
            logging.info("Serial connection closed.")

    def send_cmd(self, cmd: str):
        """
        Sends a command to the serial device with a newline, and prints it.

        :param cmd: The command to be sent.
        """
        if self.serial_connection and self.serial_connection.is_open:
            try:
                self.serial_connection.write((cmd + "\r\n").encode('ascii'))
            except serial.SerialException as e:
                logging.error(f"Error sending command: {e}")

    def read_bytes(self, count: int):
        if self.serial_connection:
            x = self.serial_connection.read(count)
            return x

    def receive_cmd(self) -> str:
        """
        Reads data from the serial device until no more data is available.

        :return: The processed received data as a string.
        """
        s = ""
        if self.serial_connection and self.serial_connection.is_open:
            while True:
                x = self.serial_connection.read()
                if len(x) < 1 or x == 0xF0:
                    return s.replace("\0", "").strip().replace("\r", "").replace("\n\n\n", "\n")
                s += str(x, encoding='ascii', errors='ignore')

    def is_open(self) -> bool:
        """Checks if the connection is open."""
        return self.serial_connection and self.serial_connection.is_open

    def __del__(self):
        """Closes the connection if the object is deleted."""
        self.close()
        # Shut down logging
        logging.shutdown()
