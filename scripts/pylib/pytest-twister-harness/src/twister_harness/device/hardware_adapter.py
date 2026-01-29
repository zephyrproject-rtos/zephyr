# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import subprocess
import time
from pathlib import Path

from serial import SerialException

from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.device.utils import log_command, terminate_process
from twister_harness.exceptions import (
    TwisterHarnessException,
    TwisterHarnessTimeoutException,
)
from twister_harness.twister_harness_config import DeviceConfig

logger = logging.getLogger(__name__)


class HardwareAdapter(DeviceAdapter):
    """Adapter class for real device."""

    def __init__(self, device_config: DeviceConfig) -> None:
        super().__init__(device_config)
        self._flashing_timeout: float = device_config.flash_timeout

        self.device_log_path: Path = device_config.build_dir / 'device.log'
        self._log_files.append(self.device_log_path)

    def _generate_flash_command(self) -> None:
        command = [self.device_config.flash_command[0]]
        command.extend(['--build-dir', str(self.device_config.build_dir)])

        if self.device_config.id:
            command.extend(['--board-id', self.device_config.id])

        command.extend(self.device_config.flash_command[1:])

        self.command = command

    def generate_command(self) -> None:
        """Return command to flash."""
        if self.device_config.flash_command:
            self._generate_flash_command()
            return

        command = [
            self.west,
            'flash',
            '--no-rebuild',
            '--build-dir',
            str(self.device_config.build_dir),
        ]

        command_extra_args = []
        if self.device_config.west_flash_extra_args:
            command_extra_args.extend(self.device_config.west_flash_extra_args)

        if self.device_config.runner:
            runner_base_args, runner_extra_args = self._prepare_runner_args()
            command.extend(runner_base_args)
            command_extra_args.extend(runner_extra_args)

        if command_extra_args:
            command.append('--')
            command.extend(command_extra_args)
        self.command = command

    def _prepare_runner_args(self) -> tuple[list[str], list[str]]:
        base_args: list[str] = []
        extra_args: list[str] = []
        runner = self.device_config.runner
        base_args.extend(['--runner', runner])
        if self.device_config.runner_params:
            for param in self.device_config.runner_params:
                extra_args.append(param)
        if board_id := self.device_config.id:
            if runner == 'pyocd':
                extra_args.append('--board-id')
                extra_args.append(board_id)
            elif runner == "esp32":
                extra_args.append("--esp-device")
                extra_args.append(board_id)
            elif runner in ('nrfjprog', 'nrfutil', 'nrfutil_next'):
                extra_args.append('--dev-id')
                extra_args.append(board_id)
            elif runner == 'openocd' and self.device_config.product in [
                'STM32 STLink',
                'STLINK-V3',
            ]:
                extra_args.append('--cmd-pre-init')
                extra_args.append(f'hla_serial {board_id}')
            elif runner == 'openocd' and self.device_config.product == 'EDBG CMSIS-DAP':
                extra_args.append('--cmd-pre-init')
                extra_args.append(f'cmsis_dap_serial {board_id}')
            elif runner == "openocd" and self.device_config.product == "LPC-LINK2 CMSIS-DAP":
                extra_args.append("--cmd-pre-init")
                extra_args.append(f'adapter serial {board_id}')
            elif runner == 'jlink' or (
                runner == 'stm32cubeprogrammer' and self.device_config.product != "BOOT-SERIAL"
            ):
                base_args.append('--dev-id')
                base_args.append(board_id)
            elif runner == 'linkserver':
                base_args.append(f'--probe={board_id}')
        return base_args, extra_args

    def _device_launch(self) -> None:
        """Flash and run application on a device and connect with serial port."""
        if self.device_config.flash_before:
            # For hardware devices with shared USB or software USB, connect after flashing.
            # Retry for up to 10 seconds for USB-CDC based devices to enumerate.
            self._flash_and_run()
            attempt = 0
            while True:
                try:
                    self.connect()
                    break
                except SerialException:
                    if attempt < 100:
                        attempt += 1
                        time.sleep(0.1)
                    else:
                        raise
        else:
            # On hardware, flash after connecting to COM port, otherwise some messages
            # from target can be lost.
            self.connect()
            self._flash_and_run()

    def _flash_and_run(self) -> None:
        """Flash application on a device."""
        if not self.command:
            msg = 'Flash command is empty, please verify if it was generated properly.'
            logger.error(msg)
            raise TwisterHarnessException(msg)

        if self.device_config.pre_script:
            self._run_custom_script(self.device_config.pre_script, self.base_timeout)

        if self.device_config.id:
            logger.debug('Flashing device %s', self.device_config.id)
        log_command(logger, 'Flashing command', self.command, level=logging.DEBUG)

        process = stdout = None
        try:
            process = subprocess.Popen(
                self.command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=self.env
            )
            stdout, _ = process.communicate(timeout=self._flashing_timeout)
        except subprocess.TimeoutExpired as exc:
            process.kill()
            msg = f'Timeout occurred ({self._flashing_timeout}s) during flashing.'
            logger.error(msg)
            raise TwisterHarnessTimeoutException(msg) from exc
        except subprocess.SubprocessError as exc:
            msg = f'Flashing subprocess failed due to SubprocessError {exc}'
            logger.error(msg)
            raise TwisterHarnessTimeoutException(msg) from exc
        finally:
            if stdout is not None:
                stdout_decoded = stdout.decode(errors='ignore')
                with open(self.device_log_path, 'a+') as log_file:
                    log_file.write(stdout_decoded)
            if self.device_config.post_flash_script:
                self._run_custom_script(self.device_config.post_flash_script, self.base_timeout)
            if process is not None and process.returncode == 0:
                logger.debug('Flashing finished')
            else:
                msg = f'Could not flash device {self.device_config.id}'
                logger.error(msg)
                raise TwisterHarnessException(msg)

    def _close_device(self) -> None:
        # Run post script only if the reader thread is started to avoid running it
        # multiple times and when in initialization phase
        if self.is_reader_started() and self.device_config.post_script:
            self._run_custom_script(self.device_config.post_script, self.base_timeout)

    @staticmethod
    def _run_custom_script(script_path: str | Path, timeout: float) -> None:
        with subprocess.Popen(
            str(script_path), stderr=subprocess.PIPE, stdout=subprocess.PIPE
        ) as proc:
            try:
                stdout, stderr = proc.communicate(timeout=timeout)
                logger.debug(stdout.decode())
                if proc.returncode != 0:
                    msg = f'Custom script failure: \n{stderr.decode(errors="ignore")}'
                    logger.error(msg)
                    raise TwisterHarnessException(msg)

            except subprocess.TimeoutExpired as exc:
                terminate_process(proc)
                proc.communicate(timeout=timeout)
                msg = f'Timeout occurred ({timeout}s) during execution custom script: {script_path}'
                logger.error(msg)
                raise TwisterHarnessTimeoutException(msg) from exc
