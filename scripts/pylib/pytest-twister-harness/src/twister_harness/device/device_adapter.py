# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import abc
import logging
import os
import shutil
import threading
from datetime import datetime
from pathlib import Path

from twister_harness.device.device_connection import DeviceConnection, create_device_connections
from twister_harness.exceptions import TwisterHarnessException
from twister_harness.twister_harness_config import DeviceConfig

logger = logging.getLogger(__name__)


class DeviceAdapter(abc.ABC):
    """
    This class defines a common interface for all device types (hardware,
    simulator, QEMU) used in tests to gathering device output and send data to
    it.
    """

    _west: str | None = None

    def __init__(self, device_config: DeviceConfig) -> None:
        self.device_config: DeviceConfig = device_config
        self.base_timeout: float = device_config.base_timeout
        self._reader_started: threading.Event = threading.Event()
        self.command: list[str] = []

        self.connections: list[DeviceConnection] = create_device_connections(device_config)
        self._log_files: list[Path] = [conn.log_path for conn in self.connections]

    @property
    def env(self) -> dict[str, str]:
        env = os.environ.copy()
        return env

    def launch(self) -> None:
        """Launch the test application on the target device.

        This method performs the complete device initialization sequence:

        - Close any previously running application (cleanup)
        - Generate the execution command if not already set
        - Add any extra test arguments to the command
        - Start reader threads to capture device output
        - Launch the application (flash for hardware, execute for simulators)

        The launch process varies by device type:

        - Hardware devices: Flash the application and establish serial communication.
          May connect before or after flashing depending on device configuration.
        - QEMU/simulators: Start subprocess and establish FIFO/pipe communication
        - Native simulators: Execute binary and connect via process pipes
        """
        self.close()

        if not self.command:
            self.generate_command()
            if self.device_config.extra_test_args:
                self.command.extend(self.device_config.extra_test_args.split())

        self.start_reader()
        self._device_launch()

    def close(self) -> None:
        """Disconnect, close device and close reader threads."""
        self.disconnect()
        self._close_device()
        self.stop_reader()
        self._clear_internal_resources()

    def connect(self, retry_s: int = 0) -> None:
        """Connect to device - allow for output gathering."""
        for connection in self.connections:
            connection.connect()

    def disconnect(self) -> None:
        """Disconnect device - block output gathering."""
        for connection in self.connections:
            connection.disconnect()

    def check_connection(self, connection_index: int = 0) -> None:
        """Validate that the specified connection index exists."""
        if connection_index >= len(self.connections):
            msg = f'Connection index {connection_index} is out of range.'
            logger.error(msg)
            raise TwisterHarnessException(msg)

    def readline(self, connection_index: int = 0, **kwargs) -> str:
        """Read a single line from device output.

        :param connection_index: Connection port to read from (0=main UART, 1=second core, etc.)
        :param kwargs: Additional keyword arguments (timeout, print_output)
        :returns: Single line of text without trailing newlines
        """
        self.check_connection(connection_index)
        if kwargs.get('timeout') is None:
            kwargs['timeout'] = self.base_timeout
        return self.connections[connection_index].readline(**kwargs)

    def readlines(self, connection_index: int = 0, **kwargs) -> list[str]:
        """Read all available output lines produced by device from internal buffer."""
        self.check_connection(connection_index)
        return self.connections[connection_index].readlines(**kwargs)

    def readlines_until(self, connection_index: int = 0, **kwargs) -> list[str]:
        """Read lines from device output until a specific condition is met.

        This method provides flexible ways to collect device output: wait for a specific
        pattern, collect a fixed number of lines, or get all currently available lines.

        :param connection_index: Index of the connection/port to read from (default: 0).
            For hardware devices: 0 = main UART, 1 = second core UART, etc.
            For QEMU and native: always use 0 (only one connection available)
        :param kwargs: Keyword arguments passed to underlying connection including:

            - regex - Regular expression pattern to search for
            - num_of_lines - Exact number of lines to read
            - timeout - Maximum time in seconds to wait (uses base_timeout if not provided)
            - print_output - Whether to log each line as it's read (default: True)
        :returns: List of output lines without trailing newlines
        :raises AssertionError: If timeout expires before condition is met
        """
        self.check_connection(connection_index)
        return self.connections[connection_index].readlines_until(**kwargs)

    def clear_buffer(self) -> None:
        """Remove all available output produced by device from internal buffer."""
        for connection in self.connections:
            connection.clear_buffer()

    def write(self, data: bytes, connection_index: int = 0) -> None:
        """Write data bytes to the target device.

        Sends raw bytes to the device through the appropriate communication channel.
        The underlying transport mechanism varies by device type: Hardware devices
        write to serial/UART port, QEMU devices write to FIFO queue, and Native
        simulators write to process stdin pipes.

        :param data: Raw bytes to send to the device
        :param connection_index: Index of the connection/port to write to (default: 0)
        """
        self.check_connection(connection_index)
        if not self.connections[connection_index].is_device_connected():
            msg = f'Cannot write to not connected device on connection index {connection_index}.'
            logger.error(msg)
            raise TwisterHarnessException(msg)
        self.connections[connection_index]._write_to_device(data)

    def initialize_log_files(self, test_name: str = '') -> None:
        """
        Initialize log files (e.g. handler.log) by adding header with
        information about performed test and current time.
        """
        for log_file_path in self._log_files:
            with open(log_file_path, 'a+') as log_file:
                log_file.write(f'\n==== Test {test_name} started at {datetime.now()} ====\n')

    def start_reader(self) -> None:
        """Start internal reader threads for all connections."""
        if self._reader_started.is_set():
            # reader already started
            return
        self._reader_started.set()
        for connection in self.connections:
            connection.start_reader_thread(self._reader_started)

    def stop_reader(self) -> None:
        """Stop internal reader threads for all connections."""
        if not self._reader_started.is_set():
            # reader already stopped
            return
        self._reader_started.clear()
        for connection in self.connections:
            connection.join_reader_thread(self.base_timeout)

    def _clear_internal_resources(self) -> None:
        for connection in self.connections:
            connection._clear_internal_resources()

    @property
    def west(self) -> str:
        """
        Return a path to west or if not found - raise an error. Once found
        west path is stored as internal property to save time of looking for it
        in the next time.
        """
        if self._west is None:
            self._west = shutil.which('west')
            if self._west is None:
                msg = 'west not found'
                logger.error(msg)
                raise TwisterHarnessException(msg)
        return self._west

    @abc.abstractmethod
    def generate_command(self) -> None:
        """
        Generate and set command which will be used during flashing or running
        device.
        """

    @abc.abstractmethod
    def _device_launch(self) -> None:
        """Launch the application on the target device."""

    @abc.abstractmethod
    def _close_device(self) -> None:
        """Stop application on the target device."""

    def is_device_connected(self) -> bool:
        """
        Check if the primary device connection is active.

        Added to keep backward compatibility as it is used in fixtures.
        """
        return self.connections and self.connections[0].is_device_connected()

    def is_reader_started(self) -> bool:
        """Check if the reader thread is started."""
        return self._reader_started.is_set()
