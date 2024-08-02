# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import io
import logging
import os
import threading
import time
from pathlib import Path

logger = logging.getLogger(__name__)


class FifoHandler:
    """
    Class dedicated for handling communication over POSIX system FIFO (named
    pipes).
    """

    def __init__(self, fifo_path: str | Path, timeout: float):
        """
        :param fifo_path: path to basic fifo name
        :param timeout: timeout for establishing connection over FIFO
        """
        self._fifo_out_path = str(fifo_path) + '.out'
        self._fifo_in_path = str(fifo_path) + '.in'
        self._fifo_out_file: io.FileIO | None = None
        self._fifo_in_file: io.FileIO | None = None
        self._open_fifo_thread: threading.Thread | None = None
        self._opening_monitor_thread: threading.Thread | None = None
        self._fifo_opened: threading.Event = threading.Event()
        self._stop_waiting_for_opening: threading.Event = threading.Event()
        self._timeout = timeout

    def initiate_connection(self) -> None:
        """
        Opening FIFO could be a blocking operation (it requires also opening
        FIFO on the other side - by separate program/process). So, to avoid
        blockage, execute opening FIFO in separate thread and additionally run
        in second thread opening time monitor to alternatively unblock first
        thread when timeout will expire.
        """
        self._stop_waiting_for_opening.clear()
        self._make_fifo_file(self._fifo_out_path)
        self._make_fifo_file(self._fifo_in_path)
        if self._open_fifo_thread is None:
            self._open_fifo_thread = threading.Thread(target=self._open_fifo, daemon=True)
            self._open_fifo_thread.start()
        if self._opening_monitor_thread is None:
            self._opening_monitor_thread = threading.Thread(target=self._opening_monitor, daemon=True)
            self._opening_monitor_thread.start()

    @staticmethod
    def _make_fifo_file(filename: str) -> None:
        if not os.path.exists(filename):
            os.mkfifo(filename)

    def _open_fifo(self) -> None:
        self._fifo_out_file = open(self._fifo_out_path, 'rb', buffering=0)
        self._fifo_in_file = open(self._fifo_in_path, 'wb', buffering=0)
        if not self._stop_waiting_for_opening.is_set():
            self._fifo_opened.set()

    def _opening_monitor(self) -> None:
        """
        Monitor opening FIFO operation - if timeout was expired (or disconnect
        was called in the meantime), then interrupt opening FIFO in other
        thread.
        """
        timeout_time: float = time.time() + self._timeout
        while time.time() < timeout_time and not self._stop_waiting_for_opening.is_set():
            if self._fifo_opened.is_set():
                return
            time.sleep(0.1)
        self._stop_waiting_for_opening.set()
        self._unblock_open_fifo_operation()

    def _unblock_open_fifo_operation(self) -> None:
        """
        This is workaround for unblocking opening FIFO operation - imitate
        opening FIFO "on the other side".
        """
        if os.path.exists(self._fifo_out_path):
            open(self._fifo_out_path, 'wb', buffering=0)
        if os.path.exists(self._fifo_in_path):
            open(self._fifo_in_path, 'rb', buffering=0)

    def disconnect(self) -> None:
        self._stop_waiting_for_opening.set()
        if self._open_fifo_thread and self._open_fifo_thread.is_alive():
            self._open_fifo_thread.join(timeout=1)
        self._open_fifo_thread = None
        if self._opening_monitor_thread and self._opening_monitor_thread.is_alive():
            self._opening_monitor_thread.join(timeout=1)
        self._opening_monitor_thread = None
        self._fifo_opened.clear()

        if self._fifo_out_file:
            self._fifo_out_file.close()
        if self._fifo_in_file:
            self._fifo_in_file.close()

        if os.path.exists(self._fifo_out_path):
            os.unlink(self._fifo_out_path)
        if os.path.exists(self._fifo_in_path):
            os.unlink(self._fifo_in_path)

    @property
    def is_open(self) -> bool:
        try:
            return bool(
                self._fifo_opened.is_set()
                and self._fifo_in_file is not None and self._fifo_out_file is not None
                and self._fifo_in_file.fileno() and self._fifo_out_file.fileno()
            )
        except ValueError:
            return False

    def read(self, __size: int = -1) -> bytes:
        return self._fifo_out_file.read(__size)  # type: ignore[union-attr]

    def readline(self, __size: int | None = None) -> bytes:
        return self._fifo_out_file.readline(__size)  # type: ignore[union-attr]

    def write(self, __buffer: bytes) -> int:
        return self._fifo_in_file.write(__buffer)  # type: ignore[union-attr]

    def flush_write(self) -> None:
        if self._fifo_in_file:
            self._fifo_in_file.flush()

    def flush_read(self) -> None:
        if self._fifo_out_file:
            self._fifo_out_file.flush()
