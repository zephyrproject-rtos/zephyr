# Copyright (c) 2026 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
"""Robot Framework library for interacting with Zephyr native_sim binaries via stdin/stdout."""

import atexit
import subprocess
import threading

from SimLibraryBase import SimLibraryBase


class NativeSimLibrary(SimLibraryBase):
    """Keyword library that starts a native_sim binary and provides UART-like interaction
    over its stdin/stdout (requires CONFIG_UART_NATIVE_PTY_0_ON_STDINOUT=y)."""

    def __init__(self):
        super().__init__()
        self._process = None

    def start_native_binary(self, binary):
        """Start the native_sim binary and begin capturing its output."""
        self._process = subprocess.Popen(
            [binary],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            bufsize=0,
        )
        self._reader = threading.Thread(target=self._read_stdout, daemon=True)
        self._reader.start()
        atexit.register(self.stop_native_binary)

    def _read_stdout(self):
        while self._process.poll() is None:
            data = self._process.stdout.read(256)
            if not data:
                break
            with self._lock:
                self._buffer += self._strip_ansi(data)

    def write_to_uart(self, text):
        """Write a line to the binary's stdin, simulating UART input."""
        if self._process is None or self._process.poll() is not None:
            raise RuntimeError('Native binary is not running')
        self._process.stdin.write((text + '\n').encode())
        self._process.stdin.flush()

    def stop_native_binary(self):
        """Terminate the native binary (called on suite teardown)."""
        if self._process and self._process.poll() is None:
            self._process.terminate()
            try:
                self._process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self._process.kill()
