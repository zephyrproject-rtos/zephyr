# Copyright (c) 2026 Anthropic
#
# SPDX-License-Identifier: Apache-2.0

"""
DUT (Device Under Test) communication channels for twister.

Two implementations:

* :class:`SerialDUTChannel` — talks to a physical serial port via pyserial.
* :class:`ProcessDUTChannel` — talks to a user-supplied subprocess over
  stdin/stdout pipes (no pty wrapping; the subprocess is the DUT).

Both implementations expose the same surface so that
``DeviceHandler.monitor_serial`` can stay backend-agnostic.
"""

from __future__ import annotations

import contextlib
import logging
import os
import re
import select
import subprocess
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None  # type: ignore[assignment]
    list_ports = None  # type: ignore[assignment]

import psutil
import signal

logger = logging.getLogger('twister')


def _terminate(proc: subprocess.Popen) -> None:
    """Terminate a subprocess and its descendants, then force-kill after a grace
    period. Mirrors ``handlers.terminate_process`` but is duplicated here to
    avoid a circular import."""
    with contextlib.suppress(ProcessLookupError, psutil.NoSuchProcess):
        for child in psutil.Process(proc.pid).children(recursive=True):
            with contextlib.suppress(ProcessLookupError, psutil.NoSuchProcess):
                os.kill(child.pid, signal.SIGTERM)
    proc.terminate()
    time.sleep(0.5)
    proc.kill()


class DUTChannel:
    """Abstract DUT communication channel.

    Lifecycle:

    1. Construct the channel (no I/O yet).
    2. Call :meth:`open_before_flash` *or* :meth:`open_after_flash` depending
       on whether the test should attach before or after the flash step.
    3. Read with :attr:`in_waiting` / :meth:`readline` until done.
    4. Call :meth:`close` (idempotent).
    """

    @property
    def is_open(self) -> bool:
        raise NotImplementedError

    def open_before_flash(self) -> None:
        """Open the channel prior to flashing the device."""
        raise NotImplementedError

    def open_after_flash(self, flash_timeout: float) -> None:
        """Open the channel after flashing the device.

        For serial channels this is where USB re-enumeration is waited on
        and any runner-specific reset sequence (e.g. ESP32 DTR/RTS) is
        applied. For subprocess channels this just starts the process.
        """
        raise NotImplementedError

    def reset_input_buffer(self) -> None:
        raise NotImplementedError

    @property
    def in_waiting(self) -> bool:
        raise NotImplementedError

    def readline(self) -> bytes:
        raise NotImplementedError

    def close(self) -> None:
        raise NotImplementedError

    def __enter__(self) -> "DUTChannel":
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()


class SerialDUTChannel(DUTChannel):
    """A :class:`DUTChannel` backed by a physical serial port via pyserial."""

    def __init__(
        self,
        port: str,
        baud: int,
        read_timeout: float,
        runner: str | None = None,
    ):
        if serial is None:
            raise RuntimeError(
                "pyserial is not installed; cannot use SerialDUTChannel"
            )
        self._port = port
        self._baud = baud
        self._read_timeout = read_timeout
        self._runner = runner
        self._ser = serial.Serial(
            None,
            baudrate=baud,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=read_timeout,
        )
        self._ser.port = port

    def __repr__(self) -> str:
        return f"SerialDUTChannel(port={self._port!r}, baud={self._baud})"

    @property
    def is_open(self) -> bool:
        return self._ser.isOpen()

    def open_before_flash(self) -> None:
        self._ser.open()

    def open_after_flash(self, flash_timeout: float) -> None:
        logger.debug(
            f"Attach serial device {self._port} @ {self._baud} baud"
        )
        if self._runner == "esp32":
            logger.debug("Applying ESP32 RTS/DTR reset sequence")
            self._ser.dtr = True
            self._ser.rts = False
            self._ser.open()
            self._ser.dtr = False
            self._ser.rts = True
            time.sleep(0.1)
            self._ser.rts = False
        else:
            # Wait for the port to come back after USB re-enumeration. Cap the
            # wait at 20% of the flash timeout, with a 10s floor.
            wait_timeout = max(10, int(flash_timeout * 0.2))
            t0 = time.time()
            while self._ser.port not in (p.name for p in list_ports.comports()):
                time.sleep(0.1)
                if time.time() - t0 > wait_timeout:
                    break
            self._ser.open()

    def reset_input_buffer(self) -> None:
        self._ser.reset_input_buffer()

    @property
    def in_waiting(self) -> bool:
        return bool(self._ser.in_waiting)

    def readline(self) -> bytes:
        return self._ser.readline()

    def close(self) -> None:
        if self._ser.isOpen():
            self._ser.close()


class ProcessDUTChannel(DUTChannel):
    """A :class:`DUTChannel` backed by a user-supplied subprocess.

    The subprocess's stdout is read directly via :func:`os.read` with
    :func:`select.select` for non-blocking polling. No pty is involved — line
    buffering on the subprocess side is the user's responsibility (e.g. by
    running with ``python -u`` or calling ``sys.stdout.reconfigure(line_buffering=True)``).
    """

    def __init__(
        self,
        command: str,
        env: dict[str, str] | None = None,
        read_timeout: float | None = None,
        kill_timeout: float = 30.0,
    ):
        self._command_str = command
        self._command = re.split('[, ]', command)
        self._env = env if env is not None else os.environ.copy()
        self._read_timeout = read_timeout
        self._kill_timeout = kill_timeout
        self._proc: subprocess.Popen | None = None
        self._read_buf: bytes = b''

    def __repr__(self) -> str:
        return f"ProcessDUTChannel(command={self._command_str!r})"

    def _start(self) -> None:
        try:
            self._proc = subprocess.Popen(
                self._command,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=0,
                env=self._env,
            )
        except (OSError, subprocess.CalledProcessError) as error:
            logger.error(
                f"Failed to run subprocess {self._command_str}, error {error}"
            )
            raise

    @property
    def is_open(self) -> bool:
        return self._proc is not None and self._proc.poll() is None

    def open_before_flash(self) -> None:
        self._start()

    def open_after_flash(self, flash_timeout: float) -> None:
        self._start()

    def _fill_buf(self, timeout: float | None) -> None:
        """Try to read more bytes from the subprocess stdout into the internal
        buffer. Returns once *some* data has been read, or once ``timeout``
        elapses with nothing available."""
        if self._proc is None or self._proc.stdout is None:
            return
        fd = self._proc.stdout.fileno()
        try:
            ready, _, _ = select.select([fd], [], [], timeout)
        except (OSError, ValueError):
            return
        if not ready:
            return
        try:
            chunk = os.read(fd, 4096)
        except OSError:
            return
        if chunk:
            self._read_buf += chunk

    def reset_input_buffer(self) -> None:
        self._read_buf = b''
        if self._proc is None or self._proc.stdout is None:
            return
        fd = self._proc.stdout.fileno()
        # Drain whatever has accumulated without blocking.
        while True:
            try:
                ready, _, _ = select.select([fd], [], [], 0)
            except (OSError, ValueError):
                return
            if not ready:
                return
            try:
                chunk = os.read(fd, 4096)
            except OSError:
                return
            if not chunk:
                return

    @property
    def in_waiting(self) -> bool:
        if self._read_buf:
            return True
        if self._proc is None or self._proc.stdout is None:
            return False
        fd = self._proc.stdout.fileno()
        try:
            ready, _, _ = select.select([fd], [], [], 0)
        except (OSError, ValueError):
            return False
        return bool(ready)

    def readline(self) -> bytes:
        """Return one line of bytes (terminated by ``\\n`` if available), up to
        the configured ``read_timeout``. May return a partial line on timeout
        or EOF — matches pyserial's behaviour."""
        if self._proc is None:
            return b''
        deadline = (
            time.time() + self._read_timeout if self._read_timeout else None
        )
        while b'\n' not in self._read_buf:
            if deadline is None:
                remaining: float | None = None
            else:
                remaining = max(0.0, deadline - time.time())
                if remaining == 0.0:
                    break
            before = len(self._read_buf)
            self._fill_buf(remaining)
            if len(self._read_buf) == before:
                break
        if b'\n' in self._read_buf:
            line, sep, self._read_buf = self._read_buf.partition(b'\n')
            return line + sep
        line, self._read_buf = self._read_buf, b''
        return line

    def close(self) -> None:
        if self._proc is None:
            return
        proc = self._proc
        self._proc = None
        logger.debug(f"Terminating serial-pty:'{self._command_str}'")
        _terminate(proc)
        try:
            (stdout, stderr) = proc.communicate(timeout=self._kill_timeout)
            logger.debug(
                f"Terminated serial-pty:'{self._command_str}',"
                f" stdout:'{stdout}', stderr:'{stderr}'"
            )
        except subprocess.TimeoutExpired:
            logger.debug(f"Terminated serial-pty:'{self._command_str}'")
