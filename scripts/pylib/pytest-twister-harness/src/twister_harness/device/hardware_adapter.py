# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import os
import pty
import re
import shutil
import subprocess
from datetime import datetime
from typing import Generator

import serial

from twister_harness.device.device_abstract import DeviceAbstract
from twister_harness.exceptions import TwisterHarnessException
from twister_harness.helper import log_command
from twister_harness.log_files.log_file import DeviceLogFile, HandlerLogFile
from twister_harness.twister_harness_config import DeviceConfig

logger = logging.getLogger(__name__)


class HardwareAdapter(DeviceAbstract):
    """Adapter class for real device."""

    def __init__(self, device_config: DeviceConfig, **kwargs) -> None:
        super().__init__(device_config, **kwargs)
        self.connection: serial.Serial | None = None
        self.command: list[str] = []
        self.process_kwargs: dict = {
            'stdout': subprocess.PIPE,
            'stderr': subprocess.STDOUT,
            'env': self.env,
        }
        self.serial_pty_proc: subprocess.Popen | None = None

    def connect(self, timeout: float = 1) -> None:
        """
        Open serial connection.

        :param timeout: Read timeout value in seconds
        """
        if self.connection:
            # already opened
            return

        if self.device_config.pre_script:
            self.run_custom_script(self.device_config.pre_script, 30)

        serial_name = self._open_serial_pty() or self.device_config.serial
        logger.info('Opening serial connection for %s', serial_name)
        try:
            self.connection = serial.Serial(
                serial_name,
                baudrate=self.device_config.baud,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS,
                timeout=timeout
            )
        except serial.SerialException as e:
            logger.exception('Cannot open connection: %s', e)
            self._close_serial_pty()
            raise

        self.connection.flush()

    def disconnect(self) -> None:
        """Close serial connection."""
        if self.connection:
            serial_name = self.connection.port
            self.connection.close()
            self.connection = None
            logger.info('Closed serial connection for %s', serial_name)
        self._close_serial_pty()

    def stop(self) -> None:
        if self.device_config.post_script:
            self.run_custom_script(self.device_config.post_script, 30)

    def _open_serial_pty(self) -> str | None:
        """Open a pty pair, run process and return tty name"""
        if not self.device_config.serial_pty:
            return None
        master, slave = pty.openpty()
        try:
            self.serial_pty_proc = subprocess.Popen(
                re.split(',| ', self.device_config.serial_pty),
                stdout=master,
                stdin=master,
                stderr=master
            )
        except subprocess.CalledProcessError as e:
            logger.exception('Failed to run subprocess %s, error %s',
                             self.device_config.serial_pty, str(e))
            raise
        return os.ttyname(slave)

    def _close_serial_pty(self) -> None:
        """Terminate the process opened for serial pty script"""
        if self.serial_pty_proc:
            self.serial_pty_proc.terminate()
            self.serial_pty_proc.communicate()
            logger.info('Process %s terminated', self.device_config.serial_pty)
            self.serial_pty_proc = None

    def generate_command(self) -> None:
        """Return command to flash."""
        west = shutil.which('west')
        if west is None:
            raise TwisterHarnessException('west not found')

        command = [
            west,
            'flash',
            '--skip-rebuild',
            '--build-dir', str(self.device_config.build_dir),
        ]

        command_extra_args = []
        if self.device_config.west_flash_extra_args:
            command_extra_args.extend(self.device_config.west_flash_extra_args)

        if runner := self.device_config.runner:
            command.extend(['--runner', runner])

            if board_id := self.device_config.id:
                if runner == 'pyocd':
                    command_extra_args.append('--board-id')
                    command_extra_args.append(board_id)
                elif runner == 'nrfjprog':
                    command_extra_args.append('--dev-id')
                    command_extra_args.append(board_id)
                elif runner == 'openocd' and self.device_config.product in ['STM32 STLink', 'STLINK-V3']:
                    command_extra_args.append('--cmd-pre-init')
                    command_extra_args.append(f'hla_serial {board_id}')
                elif runner == 'openocd' and self.device_config.product == 'EDBG CMSIS-DAP':
                    command_extra_args.append('--cmd-pre-init')
                    command_extra_args.append(f'cmsis_dap_serial {board_id}')
                elif runner == 'jlink':
                    command.append(f'--tool-opt=-SelectEmuBySN {board_id}')
                elif runner == 'stm32cubeprogrammer':
                    command.append(f'--tool-opt=sn={board_id}')

        if command_extra_args:
            command.append('--')
            command.extend(command_extra_args)
        self.command = command

    @staticmethod
    def run_custom_script(script, timeout):
        with subprocess.Popen(script, stderr=subprocess.PIPE, stdout=subprocess.PIPE) as proc:
            try:
                stdout, stderr = proc.communicate(timeout=timeout)
                logger.debug(stdout.decode())
                if proc.returncode != 0:
                    logger.error(f"Custom script failure: {stderr.decode(errors='ignore')}")

            except subprocess.TimeoutExpired:
                proc.kill()
                proc.communicate()
                logger.error("{} timed out".format(script))

    def flash_and_run(self, timeout: float = 60.0) -> None:
        if not self.command:
            msg = 'Flash command is empty, please verify if it was generated properly.'
            logger.error(msg)
            raise TwisterHarnessException(msg)
        if self.device_config.id:
            logger.info('Flashing device %s', self.device_config.id)
        log_command(logger, 'Flashing command', self.command, level=logging.INFO)
        try:
            process = subprocess.Popen(
                self.command,
                **self.process_kwargs
            )
        except subprocess.CalledProcessError:
            logger.error('Error while flashing device')
            raise TwisterHarnessException('Could not flash device')
        else:
            stdout = stderr = None
            try:
                stdout, stderr = process.communicate(timeout=self.device_config.flashing_timeout)
            except subprocess.TimeoutExpired:
                process.kill()
            finally:
                if stdout:
                    self.device_log_file.handle(data=stdout)
                    logger.debug(stdout.decode(errors='ignore'))
                if stderr:
                    self.device_log_file.handle(data=stderr)
            if process.returncode == 0:
                logger.info('Flashing finished')
            else:
                raise TwisterHarnessException(f'Could not flash device {self.device_config.id}')
        finally:
            if self.device_config.post_flash_script:
                self.run_custom_script(self.device_config.post_flash_script, 30)

    def iter_stdout_lines(self) -> Generator[str, None, None]:
        """Return output from serial."""
        if not self.connection:
            return
        self.connection.flush()
        self.connection.reset_input_buffer()
        while self.connection and self.connection.is_open:
            stream = self.connection.readline()
            self.handler_log_file.handle(data=stream)
            yield stream.decode(errors='ignore').strip()
        self.iter_object = None

    def write(self, data: bytes) -> None:
        """Write data to serial"""
        if self.connection:
            self.connection.write(data)

    def initialize_log_files(self) -> None:
        self.handler_log_file = HandlerLogFile.create(build_dir=self.device_config.build_dir)
        self.device_log_file = DeviceLogFile.create(build_dir=self.device_config.build_dir)
        start_msg = f'\n==== Logging started at {datetime.now()} ====\n'
        self.handler_log_file.handle(start_msg)
        self.device_log_file.handle(start_msg)
