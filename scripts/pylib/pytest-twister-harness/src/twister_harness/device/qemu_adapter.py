# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import time

from twister_harness.device.fifo_handler import FifoHandler
from twister_harness.device.binary_adapter import BinaryAdapterBase
from twister_harness.exceptions import TwisterHarnessException
from twister_harness.twister_harness_config import DeviceConfig

logger = logging.getLogger(__name__)


class QemuAdapter(BinaryAdapterBase):
    def __init__(self, device_config: DeviceConfig) -> None:
        super().__init__(device_config)
        qemu_fifo_file_path = self.device_config.build_dir / 'qemu-fifo'
        self._fifo_connection: FifoHandler = FifoHandler(qemu_fifo_file_path, self.base_timeout)

    def generate_command(self) -> None:
        """Set command to run."""
        self.command = [self.west, 'build', '-d', str(self.device_config.app_build_dir), '-t', 'run']
        if 'stdin' in self.process_kwargs:
            self.process_kwargs.pop('stdin')

    def _flash_and_run(self) -> None:
        super()._flash_and_run()
        self._create_fifo_connection()

    def _create_fifo_connection(self) -> None:
        self._fifo_connection.initiate_connection()
        timeout_time: float = time.time() + self.base_timeout
        while time.time() < timeout_time and self._is_binary_running():
            if self._fifo_connection.is_open:
                return
            time.sleep(0.1)
        msg = 'Cannot establish communication with QEMU device.'
        logger.error(msg)
        raise TwisterHarnessException(msg)

    def _stop_subprocess(self) -> None:
        super()._stop_subprocess()
        self._fifo_connection.disconnect()

    def _read_device_output(self) -> bytes:
        try:
            output = self._fifo_connection.readline()
        except (OSError, ValueError):
            # emulation was probably finished and thus fifo file was closed too
            output = b''
        return output

    def _write_to_device(self, data: bytes) -> None:
        self._fifo_connection.write(data)
        self._fifo_connection.flush_write()

    def _flush_device_output(self) -> None:
        if self.is_device_running():
            self._fifo_connection.flush_read()

    def is_device_connected(self) -> bool:
        """Return true if device is connected."""
        return bool(super().is_device_connected() and self._fifo_connection.is_open)
