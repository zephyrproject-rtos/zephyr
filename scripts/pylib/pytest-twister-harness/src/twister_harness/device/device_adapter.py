# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import abc
import logging
import os
import queue
import re
import shutil
import threading
import time
from datetime import datetime
from pathlib import Path

from twister_harness.exceptions import (
    TwisterHarnessException,
    TwisterHarnessTimeoutException,
)
from twister_harness.twister_harness_config import DeviceConfig

logger = logging.getLogger(__name__)


class DeviceAdapter(abc.ABC):
    """
    This class defines a common interface for all device types (hardware,
    simulator, QEMU) used in tests to gathering device output and send data to
    it.
    """

    def __init__(self, device_config: DeviceConfig) -> None:
        """
        :param device_config: device configuration
        """
        self.device_config: DeviceConfig = device_config
        self.base_timeout: float = device_config.base_timeout
        self._device_read_queue: queue.Queue = queue.Queue()
        self._reader_thread: threading.Thread | None = None
        self._device_run: threading.Event = threading.Event()
        self._device_connected: threading.Event = threading.Event()
        self.command: list[str] = []
        self._west: str | None = None

        self.handler_log_path: Path = device_config.build_dir / 'handler.log'
        self._log_files: list[Path] = [self.handler_log_path]

    def __repr__(self) -> str:
        return f'{self.__class__.__name__}()'

    @property
    def env(self) -> dict[str, str]:
        env = os.environ.copy()
        return env

    def launch(self) -> None:
        """
        Start by closing previously running application (no effect if not
        needed). Then, flash and run test application. Finally, start an
        internal reader thread capturing an output from a device.
        """
        self.close()
        self._clear_internal_resources()

        if not self.command:
            self.generate_command()

        if self.device_config.type != 'hardware':
            self._flash_and_run()

        self._device_run.set()
        self._start_reader_thread()
        self.connect()

        if self.device_config.type == 'hardware':
            # On hardware, flash after connecting to COM port, otherwise some messages
            # from target can be lost.
            self._flash_and_run()

    def close(self) -> None:
        """Disconnect, close device and close reader thread."""
        if not self._device_run.is_set():
            # device already closed
            return
        self.disconnect()
        self._close_device()
        self._device_run.clear()
        self._join_reader_thread()

    def connect(self) -> None:
        """Connect to device - allow for output gathering."""
        if self.is_device_connected():
            logger.debug('Device already connected')
            return
        if not self.is_device_running():
            msg = 'Cannot connect to not working device'
            logger.error(msg)
            raise TwisterHarnessException(msg)
        self._connect_device()
        self._device_connected.set()

    def disconnect(self) -> None:
        """Disconnect device - block output gathering."""
        if not self.is_device_connected():
            logger.debug("Device already disconnected")
            return
        self._disconnect_device()
        self._device_connected.clear()

    def readline(self, timeout: float | None = None, print_output: bool = True) -> str:
        """
        Read line from device output. If timeout is not provided, then use
        base_timeout.
        """
        timeout = timeout or self.base_timeout
        if self.is_device_connected() or not self._device_read_queue.empty():
            data = self._read_from_queue(timeout)
        else:
            msg = 'No connection to the device and no more data to read.'
            logger.error(msg)
            raise TwisterHarnessException('No connection to the device and no more data to read.')
        if print_output:
            logger.debug('#: %s', data)
        return data

    def readlines_until(
            self,
            regex: str | None = None,
            num_of_lines: int | None = None,
            timeout: float | None = None,
            print_output: bool = True,
    ) -> list[str]:
        """
        Read available output lines produced by device from internal buffer
        until following conditions:

        1. If regex is provided - read until regex regex is found in read
           line (or until timeout).
        2. If num_of_lines is provided - read until number of read lines is
           equal to num_of_lines (or until timeout).
        3. If none of above is provided - return immediately lines collected so
           far in internal buffer.

        If timeout is not provided, then use base_timeout.
        """
        timeout = timeout or self.base_timeout
        if regex:
            regex_compiled = re.compile(regex)
        lines: list[str] = []
        if regex or num_of_lines:
            timeout_time: float = time.time() + timeout
            while time.time() < timeout_time:
                try:
                    line = self.readline(0.1, print_output)
                except TwisterHarnessTimeoutException:
                    continue
                lines.append(line)
                if regex and regex_compiled.search(line):
                    break
                if num_of_lines and len(lines) == num_of_lines:
                    break
            else:
                msg = 'Read from device timeout occurred'
                logger.error(msg)
                raise TwisterHarnessTimeoutException(msg)
        else:
            lines = self.readlines(print_output)
        return lines

    def readlines(self, print_output: bool = True) -> list[str]:
        """
        Read all available output lines produced by device from internal buffer.
        """
        lines: list[str] = []
        while not self._device_read_queue.empty():
            line = self.readline(0.1, print_output)
            lines.append(line)
        return lines

    def clear_buffer(self) -> None:
        """
        Remove all available output produced by device from internal buffer
        (queue).
        """
        self.readlines(print_output=False)

    def write(self, data: bytes) -> None:
        """Write data bytes to device."""
        if not self.is_device_connected():
            msg = 'No connection to the device'
            logger.error(msg)
            raise TwisterHarnessException(msg)
        self._write_to_device(data)

    def initialize_log_files(self, test_name: str = '') -> None:
        """
        Initialize log files (e.g. handler.log) by adding header with
        information about performed test and current time.
        """
        for log_file_path in self._log_files:
            with open(log_file_path, 'a+') as log_file:
                log_file.write(f'\n==== Test {test_name} started at {datetime.now()} ====\n')

    def _start_reader_thread(self) -> None:
        self._reader_thread = threading.Thread(target=self._handle_device_output, daemon=True)
        self._reader_thread.start()

    def _handle_device_output(self) -> None:
        """
        This method is dedicated to run it in separate thread to read output
        from device and put them into internal queue and save to log file.
        """
        with open(self.handler_log_path, 'a+') as log_file:
            while self.is_device_running():
                if self.is_device_connected():
                    output = self._read_device_output().decode(errors='replace').strip()
                    if output:
                        self._device_read_queue.put(output)
                        log_file.write(f'{output}\n')
                        log_file.flush()
                else:
                    # ignore output from device
                    self._flush_device_output()
                    time.sleep(0.1)

    def _read_from_queue(self, timeout: float) -> str:
        """Read data from internal queue"""
        try:
            data: str | object = self._device_read_queue.get(timeout=timeout)
        except queue.Empty as exc:
            raise TwisterHarnessTimeoutException(f'Read from device timeout occurred ({timeout}s)') from exc
        return data

    def _join_reader_thread(self) -> None:
        if self._reader_thread is not None:
            self._reader_thread.join(self.base_timeout)
        self._reader_thread = None

    def _clear_internal_resources(self) -> None:
        self._reader_thread = None
        self._device_read_queue = queue.Queue()
        self._device_run.clear()
        self._device_connected.clear()

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
    def _flash_and_run(self) -> None:
        """Flash and run application on a device."""

    @abc.abstractmethod
    def _connect_device(self) -> None:
        """Connect with the device (e.g. via serial port)."""

    @abc.abstractmethod
    def _disconnect_device(self) -> None:
        """Disconnect from the device (e.g. from serial port)."""

    @abc.abstractmethod
    def _close_device(self) -> None:
        """Stop application"""

    @abc.abstractmethod
    def _read_device_output(self) -> bytes:
        """
        Read device output directly through serial, subprocess, FIFO, etc.
        Even if device is not connected, this method has to return something
        (e.g. empty bytes string). This assumption is made to maintain
        compatibility between various adapters and their reading technique.
        """

    @abc.abstractmethod
    def _write_to_device(self, data: bytes) -> None:
        """Write to device directly through serial, subprocess, FIFO, etc."""

    @abc.abstractmethod
    def _flush_device_output(self) -> None:
        """Flush device connection (serial, subprocess output, FIFO, etc.)"""

    @abc.abstractmethod
    def is_device_running(self) -> bool:
        """Return true if application is running on device."""

    @abc.abstractmethod
    def is_device_connected(self) -> bool:
        """Return true if device is connected."""
