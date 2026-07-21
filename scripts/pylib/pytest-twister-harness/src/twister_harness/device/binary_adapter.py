# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import abc
import logging
import subprocess

from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.device.utils import log_command, terminate_process
from twister_harness.exceptions import TwisterHarnessException
from twister_harness.twister_harness_config import DeviceConfig

logger = logging.getLogger(__name__)


class BinaryAdapterBase(DeviceAdapter, abc.ABC):
    def __init__(self, device_config: DeviceConfig) -> None:
        """
        :param twister_config: twister configuration
        """
        super().__init__(device_config)
        self._process: subprocess.Popen | None = None
        self.process_kwargs: dict = {
            'stdout': subprocess.PIPE,
            'stderr': subprocess.STDOUT,
            'stdin': subprocess.PIPE,
            'env': self.env,
            'cwd': device_config.app_build_dir,
        }

    @abc.abstractmethod
    def generate_command(self) -> None:
        """Generate and set command which will be used during running device."""

    def _device_launch(self) -> None:
        self._run_subprocess()
        self.connect()

    def _run_subprocess(self) -> None:
        if not self.command:
            msg = 'Run command is empty, please verify if it was generated properly.'
            logger.error(msg)
            raise TwisterHarnessException(msg)
        log_command(logger, 'Running command', self.command, level=logging.DEBUG)
        try:
            self._process = subprocess.Popen(self.command, **self.process_kwargs)
            # Update all ProcessConnection instances with the new process
            for conn in self.connections:
                conn.update(process=self._process)
        except subprocess.SubprocessError as exc:
            msg = f'Running subprocess failed due to SubprocessError {exc}'
            logger.error(msg)
            raise TwisterHarnessException(msg) from exc
        except FileNotFoundError as exc:
            msg = f'Running subprocess failed due to file not found: {exc.filename}'
            logger.error(msg)
            raise TwisterHarnessException(msg) from exc
        except Exception as exc:
            msg = f'Running subprocess failed {exc}'
            logger.error(msg)
            raise TwisterHarnessException(msg) from exc

    def _stop_subprocess(self) -> None:
        if self._process is None:
            # subprocess already stopped
            return
        return_code: int | None = self._process.poll()
        if return_code is None:
            terminate_process(self._process, self.base_timeout)
            return_code = self._process.wait(self.base_timeout)
        self._process = None
        for conn in self.connections:
            if hasattr(conn, '_process'):
                conn._process = None
        logger.debug('Running subprocess finished with return code %s', return_code)

    def _close_device(self) -> None:
        """Terminate subprocess"""
        self._stop_subprocess()

    def _clear_internal_resources(self) -> None:
        super()._clear_internal_resources()
        self._process = None


class NativeSimulatorAdapter(BinaryAdapterBase):
    """Simulator adapter to run `zephyr.exe` simulation"""

    def generate_command(self) -> None:
        """Set command to run."""
        self.command = [str(self.device_config.app_build_dir / 'zephyr' / 'zephyr.exe')]


class UnitSimulatorAdapter(BinaryAdapterBase):
    """Simulator adapter to run unit tests"""

    def generate_command(self) -> None:
        """Set command to run."""
        self.command = [str(self.device_config.app_build_dir / 'testbinary')]


class CustomSimulatorAdapter(BinaryAdapterBase):
    """Simulator adapter to run custom simulator"""

    def generate_command(self) -> None:
        """Set command to run."""
        self.command = [
            self.west,
            'build',
            '-d',
            str(self.device_config.app_build_dir),
            '-t',
            'run',
        ]


class QemuAdapter(BinaryAdapterBase):
    """Simulator adapter to run QEMU"""

    def generate_command(self) -> None:
        """Set command to run."""
        self.command = [
            self.west,
            'build',
            '-d',
            str(self.device_config.app_build_dir),
            '-t',
            'run',
        ]
        if 'stdin' in self.process_kwargs:
            self.process_kwargs.pop('stdin')
