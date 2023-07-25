# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import io
import logging
import os
import threading
from pathlib import Path

logger = logging.getLogger(__name__)


class FifoHandler:
    """Creates FIFO file for reading and writing."""

    def __init__(self, fifo: str | Path):
        """
        :param fifo: path to fifo file
        """
        self._fifo_in = str(fifo) + '.in'
        self._fifo_out = str(fifo) + '.out'
        self.file_in: io.BytesIO | None = None
        self.file_out: io.BytesIO | None = None
        self._threads: list[threading.Thread] = []

    @staticmethod
    def _make_fifo_file(filename: str) -> None:
        if os.path.exists(filename):
            os.unlink(filename)
        os.mkfifo(filename)
        logger.debug('Created new fifo file: %s', filename)

    @property
    def is_open(self) -> bool:
        try:
            return bool(
                self.file_in is not None and self.file_out is not None
                and self.file_in.fileno() and self.file_out.fileno()
            )
        except ValueError:
            return False

    def connect(self):
        self._make_fifo_file(self._fifo_in)
        self._make_fifo_file(self._fifo_out)
        self._threads = [
            threading.Thread(target=self._open_fifo_in, daemon=True),
            threading.Thread(target=self._open_fifo_out, daemon=True)
        ]
        for t in self._threads:
            t.start()

    def _open_fifo_in(self):
        self.file_in = open(self._fifo_in, 'wb', buffering=0)

    def _open_fifo_out(self):
        self.file_out = open(self._fifo_out, 'rb', buffering=0)

    def disconnect(self):
        if self.file_in is not None:
            self.file_in.close()
        if self.file_out is not None:
            self.file_out.close()
        for t in self._threads:
            t.join(timeout=1)
        logger.debug(f'Unlink {self._fifo_in}')
        os.unlink(self._fifo_in)
        logger.debug(f'Unlink {self._fifo_out}')
        os.unlink(self._fifo_out)

    def read(self, __size: int | None = None) -> bytes:
        return self.file_out.read(__size)  # type: ignore[union-attr]

    def readline(self, __size: int | None = None) -> bytes:
        line = self.file_out.readline(__size)  # type: ignore[union-attr]
        return line

    def write(self, __buffer: bytes) -> int:
        return self.file_in.write(__buffer)  # type: ignore[union-attr]

    def flush(self):
        if self.file_in:
            self.file_in.flush()
        if self.file_out:
            self.file_out.flush()

    def fileno(self) -> int:
        return self.file_out.fileno()  # type: ignore[union-attr]
