# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import abc
import logging
import os
from typing import Generator

from twister_harness.log_files.log_file import LogFile, NullLogFile
from twister_harness.twister_harness_config import DeviceConfig

logger = logging.getLogger(__name__)


class DeviceAbstract(abc.ABC):
    """Class defines an interface for all devices."""

    def __init__(self, device_config: DeviceConfig, **kwargs) -> None:
        """
        :param device_config: device configuration
        """
        self.device_config: DeviceConfig = device_config
        self.handler_log_file: LogFile = NullLogFile.create()
        self.device_log_file: LogFile = NullLogFile.create()
        self.iter_object: Generator[str, None, None] | None = None

    def __repr__(self) -> str:
        return f'{self.__class__.__name__}()'

    @property
    def env(self) -> dict[str, str]:
        env = os.environ.copy()
        return env

    @abc.abstractmethod
    def connect(self, timeout: float = 1) -> None:
        """Connect with the device (e.g. via UART)"""

    @abc.abstractmethod
    def disconnect(self) -> None:
        """Close a connection with the device"""

    @abc.abstractmethod
    def generate_command(self) -> None:
        """
        Generate command which will be used during flashing or running device.
        """

    def flash_and_run(self, timeout: float = 60.0) -> None:
        """
        Flash and run application on a device.

        :param timeout: time out in seconds
        """

    @abc.abstractmethod
    def write(self, data: bytes) -> None:
        """Write data bytes to device"""

    @abc.abstractmethod
    def initialize_log_files(self):
        """
        Initialize file to store logs.
        """

    def stop(self) -> None:
        """Stop device."""

    @abc.abstractmethod
    def iter_stdout_lines(self) -> Generator[str, None, None]:
        """A generator that yields lines read from device"""

    @property
    def iter_stdout(self) -> Generator[str, None, None]:
        """
        Get generator object to iterate stdout from a device.
        This wrapper method is added to avoid problems, when
        user creates an instance of generator multiple times.
        """
        if not self.iter_object:
            self.iter_object = self.iter_stdout_lines()
        return self.iter_object
