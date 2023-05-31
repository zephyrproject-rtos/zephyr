# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import abc
import asyncio
import asyncio.subprocess
import logging
import os
import shutil
import signal
import subprocess
import threading
import time
from asyncio.base_subprocess import BaseSubprocessTransport
from datetime import datetime
from functools import wraps
from pathlib import Path
from queue import Queue
from typing import Generator

import psutil

from twister_harness.constants import END_OF_DATA
from twister_harness.device.device_abstract import DeviceAbstract
from twister_harness.exceptions import TwisterHarnessException
from twister_harness.helper import log_command
from twister_harness.log_files.log_file import HandlerLogFile
from twister_harness.twister_harness_config import DeviceConfig


# Workaround for RuntimeError: Event loop is closed
# https://pythonalgos.com/runtimeerror-event-loop-is-closed-asyncio-fix/
def silence_event_loop_closed(func):
    @wraps(func)
    def wrapper(self, *args, **kwargs):
        try:
            return func(self, *args, **kwargs)
        except RuntimeError as e:
            if str(e) != 'Event loop is closed':
                raise

    return wrapper


BaseSubprocessTransport.__del__ = silence_event_loop_closed(BaseSubprocessTransport.__del__)  # type: ignore

logger = logging.getLogger(__name__)


class SimulatorAdapterBase(DeviceAbstract, abc.ABC):

    def __init__(self, device_config: DeviceConfig, **kwargs) -> None:
        """
        :param twister_config: twister configuration
        """
        super().__init__(device_config, **kwargs)
        self._process: asyncio.subprocess.Process | None = None
        self._process_ended_with_timeout: bool = False
        self.queue: Queue = Queue()
        self._stop_job: bool = False
        self._exc: Exception | None = None  #: store any exception which appeared running this thread
        self._thread: threading.Thread | None = None
        self.command: list[str] = []
        self.process_kwargs: dict = {
            'stdout': asyncio.subprocess.PIPE,
            'stderr': asyncio.subprocess.STDOUT,
            'stdin': asyncio.subprocess.PIPE,
            'env': self.env,
        }
        self._data_to_send: bytes | None = None

    def connect(self, timeout: float = 1) -> None:
        pass  # pragma: no cover

    def flash_and_run(self, timeout: float = 60.0) -> None:
        if not self.command:
            msg = 'Run simulation command is empty, please verify if it was generated properly.'
            logger.error(msg)
            raise TwisterHarnessException(msg)
        self._thread = threading.Thread(target=self._run_simulation, args=(timeout,), daemon=True)
        self._thread.start()
        # Give a time to start subprocess before test is executed
        time.sleep(0.1)
        # Check if subprocess (simulation) has started without errors
        if self._exc is not None:
            logger.error('Simulation failed due to an exception: %s', self._exc)
            raise self._exc

    def _run_simulation(self, timeout: float) -> None:
        log_command(logger, 'Running command', self.command, level=logging.INFO)
        try:
            return_code: int = asyncio.run(self._run_command(timeout=timeout))
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
                logger.info('Running simulation stopped interrupted by user')
            else:
                logger.warning('Running simulation finished with return code %s', return_code)
        finally:
            self.queue.put(END_OF_DATA)  # indicate to the other threads that there will be no more data in queue

    async def _run_command(self, timeout: float = 60.):
        assert isinstance(self.command, (list, tuple, set))
        # to avoid stupid and difficult to debug mistakes
        # we are using asyncio to run subprocess to be able to read from stdout
        # without blocking while loop (readline with timeout)
        self._process = await asyncio.create_subprocess_exec(
            *self.command,
            **self.process_kwargs
        )
        logger.debug('Started subprocess with PID %s', self._process.pid)
        end_time = time.time() + timeout
        while not self._stop_job and not self._process.stdout.at_eof():  # type: ignore[union-attr]
            if line := await self._read_line(timeout=0.1):
                self.queue.put(line.decode('utf-8').strip())
            if time.time() > end_time:
                self._process_ended_with_timeout = True
                logger.info(f'Finished process with PID {self._process.pid} after {timeout} seconds timeout')
                break
            if self._data_to_send:
                self._process.stdin.write(self._data_to_send)  # type: ignore[union-attr]
                await self._process.stdin.drain()  # type: ignore[union-attr]
                self._data_to_send = None

        self.queue.put(END_OF_DATA)  # indicate to the other threads that there will be no more data in queue
        return await self._process.wait()

    async def _read_line(self, timeout=0.1) -> bytes | None:
        try:
            return await asyncio.wait_for(self._process.stdout.readline(), timeout=timeout)  # type: ignore[union-attr]
        except asyncio.TimeoutError:
            return None

    def disconnect(self):
        pass  # pragma: no cover

    def stop(self) -> None:
        """Stop device."""
        self._stop_job = True
        time.sleep(0.1)  # give a time to end while loop in running simulation
        if self._process is not None and self._process.returncode is None:
            logger.debug('Stopping all running processes for PID %s', self._process.pid)
            # kill subprocess if it is still running
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

    def iter_stdout_lines(self) -> Generator[str, None, None]:
        """Return output from serial."""
        while True:
            stream = self.queue.get()
            if stream == END_OF_DATA:
                logger.debug('No more data from running process')
                break
            self.handler_log_file.handle(data=stream + '\n')
            yield stream
            self.queue.task_done()
        self.iter_object = None

    def write(self, data: bytes) -> None:
        """Write data to serial"""
        while self._data_to_send:
            # wait data will be write to self._process.stdin.write
            time.sleep(0.1)
        self._data_to_send = data

    def initialize_log_files(self):
        self.handler_log_file = HandlerLogFile.create(build_dir=self.device_config.build_dir)
        start_msg = f'\n==== Logging started at {datetime.now()} ====\n'
        self.handler_log_file.handle(start_msg)


class NativeSimulatorAdapter(SimulatorAdapterBase):
    """Simulator adapter to run `zephyr.exe` simulation"""

    def generate_command(self) -> None:
        """Return command to run."""
        self.command = [
            str((Path(self.device_config.build_dir) / 'zephyr' / 'zephyr.exe').resolve())
        ]


class UnitSimulatorAdapter(SimulatorAdapterBase):
    """Simulator adapter to run unit tests"""

    def generate_command(self) -> None:
        """Return command to run."""
        self.command = [str((Path(self.device_config.build_dir) / 'testbinary').resolve())]


class CustomSimulatorAdapter(SimulatorAdapterBase):

    def generate_command(self) -> None:
        """Return command to run."""
        if (west := shutil.which('west')) is None:
            logger.error('west not found')
            self.command = []
        else:
            self.command = [west, 'build', '-d', str(self.device_config.build_dir), '-t', 'run']
