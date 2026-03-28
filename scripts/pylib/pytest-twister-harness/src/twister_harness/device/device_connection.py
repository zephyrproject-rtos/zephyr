# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import abc
import logging
import os
import re

if os.name != 'nt':
    import pty
import queue
import subprocess
import threading
import time
from pathlib import Path

import serial

from twister_harness.device.fifo_handler import FifoHandler
from twister_harness.exceptions import TwisterHarnessException, TwisterHarnessTimeoutException
from twister_harness.twister_harness_config import DeviceConfig, DeviceSerialConfig

logger = logging.getLogger(__name__)


class DeviceConnection(abc.ABC):
    """
    Interface for device communication transport layer.
    Handles the actual connection mechanism (Serial/UART, FIFO, process, etc.)
    """

    def __init__(self, log_path: Path, timeout: float) -> None:
        """Initialize the device connection"""
        self.log_path = log_path
        self.timeout = timeout
        self._device_read_queue: queue.Queue = queue.Queue()
        self._reader_thread: threading.Thread | None = None
        # Prefix for log messages to differentiate between multiple connections
        self.log_prefix: str = ''

    def start_reader_thread(self, reader_started: threading.Event) -> None:
        """Start the internal reader thread for the device connection."""
        if self._reader_thread is None or not self._reader_thread.is_alive():
            self._reader_thread = threading.Thread(
                target=self._handle_device_output, args=(reader_started,), daemon=True
            )
            self._reader_thread.start()

    def join_reader_thread(self, timeout: float) -> None:
        """Join the internal reader thread for the device connection."""
        if self._reader_thread is not None:
            self._reader_thread.join(timeout)
            if self._reader_thread.is_alive():
                logger.warning("Reader thread did not terminate within timeout")
        self._reader_thread = None

    def update(self, **kwargs) -> None:  # noqa: B027
        """Update connection parameters based on provided keyword arguments."""

    def _clear_internal_resources(self) -> None:
        self._reader_thread = None
        self._device_read_queue = queue.Queue()

    def _handle_device_output(self, reader_started: threading.Event) -> None:
        """
        This method is dedicated to run it in separate thread to read output
        from device and put them into internal queue and save to log file.
        """
        with open(self.log_path, 'a+', encoding='utf-8', errors='replace') as log_file:
            while reader_started.is_set():
                if self.is_device_connected():
                    output = self._read_device_output().decode(errors='replace').rstrip("\r\n")
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
            raise TwisterHarnessTimeoutException(
                f'Read from device timeout occurred ({timeout}s)'
            ) from exc
        return data

    def readline(self, timeout: float | None = None, print_output: bool = True) -> str:
        """
        Read line from device output. If timeout is not provided, then use
        base_timeout.
        """
        timeout = timeout or self.timeout
        if self.is_device_connected() or not self._device_read_queue.empty():
            data = self._read_from_queue(timeout)
        else:
            msg = 'No connection to the device and no more data to read.'
            logger.error(msg)
            raise TwisterHarnessException('No connection to the device and no more data to read.')
        if print_output:
            logger.debug('%s#: %s', self.log_prefix, data)
        return data

    def readlines_until(
        self,
        regex: str | None = None,
        num_of_lines: int | None = None,
        timeout: float | None = None,
        print_output: bool = True,
    ) -> list[str]:
        """
        Read lines from device output until a specific condition is met.

        This method provides flexible ways to collect device output from your test device:
        - Wait for a specific pattern/message to appear in the output
        - Collect a fixed number of lines
        - Get all currently available lines immediately
        """
        __tracebackhide__ = True  # pylint: disable=unused-variable
        timeout = timeout or self.timeout
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
                if regex is not None:
                    msg = f'Did not find line "{regex}" within {timeout} seconds'
                else:
                    msg = f'Did not find expected number of lines within {timeout} seconds'
                logger.error(msg)
                raise AssertionError(msg)
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
        Remove all available output produced by device from internal buffer.
        """
        self.readlines(print_output=False)

    def connect(self) -> None:
        """Connect to device - allow for output gathering."""
        if self.is_device_connected():
            # Device already connected
            return
        self._connect_device()

    def disconnect(self) -> None:
        """Disconnect device - block output gathering."""
        if not self.is_device_connected():
            # Device already disconnected
            return
        self._disconnect_device()

    @abc.abstractmethod
    def _connect_device(self) -> None:
        """Connect with the device (e.g. via serial port)."""

    @abc.abstractmethod
    def _disconnect_device(self) -> None:
        """Disconnect from the device (e.g. from serial port)."""

    @abc.abstractmethod
    def _read_device_output(self) -> bytes:
        """
        Read device output directly through serial, subprocess, FIFO, etc.
        Even if device is not connected, this method has to return something
        (e.g. empty bytes string). This assumption is made to maintain
        compatibility between various adapters and their reading technique.
        """

    @abc.abstractmethod
    def is_device_connected(self) -> bool:
        """Return true if device is connected."""

    @abc.abstractmethod
    def _write_to_device(self, data: bytes) -> None:
        """Write to device directly through serial, subprocess, FIFO, etc."""

    @abc.abstractmethod
    def _flush_device_output(self) -> None:
        """Flush device connection (serial, subprocess output, FIFO, etc.)"""


class SerialConnection(DeviceConnection):
    """Serial/UART connection implementation for hardware devices"""

    def __init__(self, log_path: Path, timeout: float, serial_config: DeviceSerialConfig) -> None:
        """
        Initialize serial connection.
        """
        super().__init__(log_path, timeout)
        self.serial_config: DeviceSerialConfig = serial_config
        self._serial_connection: serial.Serial | None = None
        self._serial_pty_proc: subprocess.Popen | None = None
        self._serial_buffer: bytearray = bytearray()

    def _connect_device(self) -> None:
        if self.is_device_connected():
            # Device already connected
            return
        serial_name = self._open_serial_pty() or self.serial_config.port
        logger.debug('Opening serial connection for %s', serial_name)
        try:
            self._serial_connection = serial.Serial(
                serial_name,
                baudrate=self.serial_config.baud,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS,
                timeout=self.timeout,
            )
        except serial.SerialException as exc:
            logger.exception('Cannot open connection: %s', exc)
            self._close_serial_pty()
            raise

        self._serial_connection.flush()
        self._serial_connection.reset_input_buffer()
        self._serial_connection.reset_output_buffer()

    def is_device_connected(self) -> bool:
        return self._serial_connection and self._serial_connection.is_open

    def _open_serial_pty(self) -> str | None:
        """Open a pty pair, run process and return tty name"""
        if not self.serial_config.serial_pty:
            return None

        try:
            master, slave = pty.openpty()
        except NameError as exc:
            logger.exception('PTY module is not available.')
            raise exc

        try:
            self._serial_pty_proc = subprocess.Popen(
                re.split('[, ]', self.serial_config.serial_pty),
                stdout=master,
                stdin=master,
                stderr=master,
            )
        except subprocess.CalledProcessError as exc:
            logger.exception('Failed to run subprocess, error %s', str(exc))
            raise
        return os.ttyname(slave)

    def _disconnect_device(self) -> None:
        if self._serial_connection:
            serial_name = self._serial_connection.port
            self._serial_connection.close()
            self._serial_connection = None
            logger.debug('Closed serial connection for %s', serial_name)
        self._close_serial_pty()

    def _close_serial_pty(self) -> None:
        """Terminate the process opened for serial pty script"""
        if self._serial_pty_proc:
            self._serial_pty_proc.terminate()
            self._serial_pty_proc.communicate(timeout=self.timeout)
            logger.debug('Process %s terminated', self.serial_config.serial_pty)
            self._serial_pty_proc = None

    def _read_device_output(self) -> bytes:
        try:
            output = self._readline_serial()
        except (OSError, TypeError):
            # serial was probably disconnected
            output = b''
        return output

    def _readline_serial(self) -> bytes:
        """
        This method was created to avoid using PySerial built-in readline
        method which cause blocking reader thread even if there is no data to
        read. Instead for this, following implementation try to read data only
        if they are available. Inspiration for this code was taken from this
        comment:
        https://github.com/pyserial/pyserial/issues/216#issuecomment-369414522
        """
        line = self._readline_from_serial_buffer()
        if line is not None:
            return line
        while True:
            if self._serial_connection is None or not self._serial_connection.is_open:
                return b''
            elif self._serial_connection.in_waiting == 0:
                time.sleep(0.05)
            else:
                bytes_to_read = max(1, min(2048, self._serial_connection.in_waiting))
                output = self._serial_connection.read(bytes_to_read)
                self._serial_buffer.extend(output)
                line = self._readline_from_serial_buffer()
                if line is not None:
                    return line

    def _readline_from_serial_buffer(self) -> bytes | None:
        idx = self._serial_buffer.find(b"\n")
        if idx >= 0:
            line = self._serial_buffer[: idx + 1]
            self._serial_buffer = self._serial_buffer[idx + 1 :]
            return bytes(line)
        else:
            return None

    def _write_to_device(self, data: bytes) -> None:
        self._serial_connection.write(data)

    def _flush_device_output(self) -> None:
        if self.is_device_connected():
            self._serial_connection.flush()
            self._serial_connection.reset_input_buffer()

    def _clear_internal_resources(self) -> None:
        super()._clear_internal_resources()
        self._serial_connection = None
        self._serial_pty_proc = None
        self._serial_buffer.clear()


class ProcessConnection(DeviceConnection):
    """Process pipe connection implementation for native simulation"""

    def __init__(self, log_path: Path, timeout: float) -> None:
        """
        Initialize serial connection.
        """
        super().__init__(log_path, timeout)
        self._process: subprocess.Popen | None = None

    def update(self, **kwargs) -> None:
        """
        The process instance is set after connection initialization and must be
        provided by the binary adapter via this method.
        """
        if 'process' in kwargs:
            self._process = kwargs['process']

    def _read_device_output(self) -> bytes:
        return self._process.stdout.readline()

    def _connect_device(self) -> None:
        """Connect with the device. Left empty for native simulation."""

    def _disconnect_device(self) -> None:
        """Disconnect from the device. Left empty for native simulation."""

    def is_device_connected(self) -> bool:
        return self._is_binary_running()

    def _is_binary_running(self) -> bool:
        return self._process is not None and self._process.poll() is None

    def _write_to_device(self, data: bytes) -> None:
        if self.is_device_connected():
            self._process.stdin.write(data)
            self._process.stdin.flush()

    def _flush_device_output(self) -> None:
        if self.is_device_connected():
            self._process.stdout.flush()


class FifoConnection(ProcessConnection):
    """FIFO queue connection implementation for QEMU"""

    def __init__(self, log_path: Path, timeout: float, fifo_file: Path) -> None:
        """
        Initialize serial connection.
        """
        super().__init__(log_path, timeout)
        self._fifo_connection: FifoHandler = FifoHandler(fifo_file, timeout)

    def _connect_device(self) -> None:
        """Create fifo connection"""
        if self.is_device_connected():
            # Device already connected
            return
        if not self._is_binary_running():
            msg = 'Cannot connect to not working device.'
            logger.error(msg)
            raise TwisterHarnessException(msg)
        self._fifo_connection.initiate_connection()
        timeout_time: float = time.time() + self.timeout
        while time.time() < timeout_time and self._is_binary_running():
            if self._fifo_connection.is_open:
                # to flush after reconnection
                self._write_to_device(b'\n')
                return
            time.sleep(0.1)
        msg = 'Cannot establish communication with QEMU device.'
        logger.error(msg)
        raise TwisterHarnessException(msg)

    def _disconnect_device(self) -> None:
        """Disconnect fifo connection"""
        if self._fifo_connection.is_open:
            self._fifo_connection.disconnect()

    def _read_device_output(self) -> bytes:
        try:
            output = self._fifo_connection.readline()
        except (OSError, ValueError):
            # emulation was probably finished and thus fifo file was closed too
            output = b''
        return output

    def is_device_connected(self) -> bool:
        return super().is_device_connected() and self._fifo_connection.is_open

    def _write_to_device(self, data: bytes) -> None:
        if self.is_device_connected():
            self._fifo_connection.write(data)
            self._fifo_connection.flush_write()

    def _flush_device_output(self) -> None:
        if self.is_device_connected():
            self._fifo_connection.flush_read()

    def _clear_internal_resources(self) -> None:
        super()._clear_internal_resources()
        self._fifo_connection.cleanup()


def create_device_connections(device_config: DeviceConfig) -> list[DeviceConnection]:
    """Factory method to create device connections based on device configuration."""
    connections: list[DeviceConnection] = []

    log_path: Path = device_config.build_dir / 'handler.log'
    timeout = device_config.base_timeout

    if device_config.type == "hardware":
        for core, serial_config in enumerate(device_config.serial_configs):
            if core > 0:
                log_path = device_config.build_dir / f'handler_{core}.log'
            connection = SerialConnection(log_path, timeout, serial_config)
            connections.append(connection)
    elif device_config.type == "qemu":
        fifo_file = device_config.build_dir / 'qemu-fifo'
        connection = FifoConnection(log_path, timeout, fifo_file)
        connections.append(connection)
    else:  # native
        connection = ProcessConnection(log_path, timeout)
        connections.append(connection)

    # Update log prefixes only for extra connections (not for the first one)
    for index, _ in enumerate(connections[1:], start=1):
        connections[index].log_prefix = f"[{index}]"
    return connections
