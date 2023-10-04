# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import os
import shutil
import signal
import subprocess
import threading
import time
from datetime import datetime
from pathlib import Path
from queue import Empty, Queue
from typing import Generator

import psutil

from twister_harness.constants import QEMU_FIFO_FILE_NAME
from twister_harness.device.device_abstract import DeviceAbstract
from twister_harness.device.fifo_handler import FifoHandler
from twister_harness.exceptions import TwisterHarnessException
from twister_harness.helper import log_command
from twister_harness.log_files.log_file import HandlerLogFile
from twister_harness.twister_harness_config import DeviceConfig

logger = logging.getLogger(__name__)


class QemuAdapter(DeviceAbstract):
    """Adapter for Qemu simulator"""

    def __init__(self, device_config: DeviceConfig, **kwargs) -> None:
        super().__init__(device_config, **kwargs)
        self._process: subprocess.Popen | None = None
        self._process_ended_with_timeout: bool = False
        self._exc: Exception | None = None  #: store any exception which appeared running this thread
        self._thread: threading.Thread | None = None
        self._emulation_was_finished: bool = False
        self.connection = FifoHandler(Path(self.device_config.build_dir).joinpath(QEMU_FIFO_FILE_NAME))
        self.command: list[str] = []
        self.timeout: float = 60  # running timeout in seconds
        self.booting_timeout_in_ms: int = 10_000  #: wait time for booting Qemu in milliseconds

    def generate_command(self) -> None:
        """Return command to flash."""
        if (west := shutil.which('west')) is None:
            logger.error('west not found')
            self.command = []
        else:
            self.command = [west, 'build', '-d', str(self.device_config.build_dir), '-t', 'run']

    def connect(self, timeout: float = 1) -> None:
        logger.debug('Opening connection')
        self.connection.connect()

    def flash_and_run(self, timeout: float = 60.0) -> None:
        self.timeout = timeout
        if not self.command:
            msg = 'Run simulation command is empty, please verify if it was generated properly.'
            logger.error(msg)
            raise TwisterHarnessException(msg)

        self._thread = threading.Thread(target=self._run_command, args=(self.timeout,), daemon=True)
        self._thread.start()
        # Give a time to start subprocess before test is executed
        time.sleep(0.1)
        # Check if subprocess (simulation) has started without errors
        if self._exc is not None:
            logger.error('Simulation failed due to an exception: %s', self._exc)
            raise self._exc

    def _run_command(self, timeout: float) -> None:
        log_command(logger, 'Running command', self.command, level=logging.INFO)
        try:
            self._process = subprocess.Popen(
                self.command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                env=self.env
            )
            stdout, _ = self._process.communicate(timeout=timeout)
            return_code: int = self._process.returncode
        except subprocess.TimeoutExpired:
            logger.error('Running simulation finished after timeout: %s seconds', timeout)
            self._process_ended_with_timeout = True
            # we don't want to raise Timeout exception, but allowed a test to parse the output
            # and set proper status
        except subprocess.SubprocessError as e:
            logger.error('Running simulation failed due to subprocess error %s', e)
            self._exc = TwisterHarnessException(e.args)
        except FileNotFoundError as e:
            logger.error(f'Running simulation failed due to file not found: {e.filename}')
            self._exc = TwisterHarnessException(f'File not found: {e.filename}')
        except Exception as e:
            logger.error('Running simulation failed: %s', e)
            self._exc = TwisterHarnessException(e.args)
        else:
            if return_code == 0:
                logger.info('Running simulation finished with return code %s', return_code)
            elif return_code == -15:
                logger.info('Running simulation terminated')
            else:
                logger.warning('Running simulation finished with return code %s', return_code)
                for line in stdout.decode('utf-8').split('\n'):
                    logger.info(line)
        finally:
            self._emulation_was_finished = True

    def disconnect(self):
        logger.debug('Closing connection')
        self.connection.disconnect()

    def stop(self) -> None:
        """Stop device."""
        time.sleep(0.1)  # give a time to end while loop in running simulation
        if self._process is not None and self._process.returncode is None:
            logger.debug('Stopping all running processes for PID %s', self._process.pid)
            # kill all child subprocesses
            for child in psutil.Process(self._process.pid).children(recursive=True):
                try:
                    os.kill(child.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
            # kill subprocess if it is still running
            os.kill(self._process.pid, signal.SIGTERM)
        if self._thread is not None:
            self._thread.join(timeout=1)  # Should end immediately, but just in case we set timeout for 1 sec
        if self._exc:
            raise self._exc

    def _wait_for_fifo(self):
        for _ in range(int(self.booting_timeout_in_ms / 10) or 1):
            if self.connection.is_open:
                break
            elif self._emulation_was_finished:
                msg = 'Problem with starting QEMU'
                logger.error(msg)
                raise TwisterHarnessException(msg)
            time.sleep(0.1)
        else:
            msg = 'Problem with starting QEMU - fifo file was not created yet'
            logger.error(msg)
            raise TwisterHarnessException(msg)

    def iter_stdout_lines(self) -> Generator[str, None, None]:
        if not self.connection:
            return
        if not self.connection.is_open:
            self._wait_for_fifo()

        # create unblocking reading from fifo file
        q: Queue = Queue()

        def read_lines():
            while self.connection and self.connection.is_open:
                try:
                    line = self.connection.readline().decode('UTF-8').strip()
                except (OSError, ValueError):
                    # file could be closed already so we should stop reading
                    break
                if len(line) != 0:
                    q.put(line)

        t = threading.Thread(target=read_lines, daemon=True)
        t.start()

        end_time = time.time() + self.timeout
        try:
            while True:
                try:
                    stream = q.get(timeout=0.1)
                    self.handler_log_file.handle(data=stream + '\n')
                    yield stream
                except Empty:  # timeout appeared
                    pass
                if time.time() > end_time:
                    break
        except KeyboardInterrupt:
            # let thread to finish smoothly
            pass
        finally:
            t.join(1)
            self.iter_object = None

    def write(self, data: bytes) -> None:
        """Write data to serial"""
        if not self.connection:
            return
        if not self.connection.is_open:
            self._wait_for_fifo()
        self.connection.write(data)

    def initialize_log_files(self):
        self.handler_log_file = HandlerLogFile.create(build_dir=self.device_config.build_dir)
        start_msg = f'\n==== Logging started at {datetime.now()} ====\n'
        self.handler_log_file.handle(start_msg)
