# Copyright (c) 2026 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
"""Robot Framework library for interacting with Zephyr native_sim binaries via stdin/stdout."""

import atexit
import re
import subprocess
import threading
import time


class NativeSimLibrary:
    """Keyword library that starts a native_sim binary and provides UART-like interaction
    over its stdin/stdout (requires CONFIG_UART_NATIVE_PTY_0_ON_STDINOUT=y)."""

    ROBOT_LIBRARY_SCOPE = 'SUITE'

    def __init__(self):
        self._process = None
        self._buffer = ''
        self._lock = threading.Lock()
        self._reader = None

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
            # Strip ANSI escape sequences before buffering
            text = re.sub(
                r'\x1b\[[0-9;]*[mABCDEFGHJKSTfhilmnsu]', '', data.decode('utf-8', errors='replace')
            )
            with self._lock:
                self._buffer += text

    def write_to_uart(self, text):
        """Write a line to the binary's stdin, simulating UART input."""
        if self._process is None or self._process.poll() is not None:
            raise RuntimeError('Native binary is not running')
        self._process.stdin.write((text + '\n').encode())
        self._process.stdin.flush()

    def wait_for_output(self, pattern, timeout=15):
        """Wait until *pattern* (regex) appears in the binary's output."""
        end_time = time.time() + float(timeout)
        regex = re.compile(pattern, re.MULTILINE)
        while time.time() < end_time:
            with self._lock:
                m = regex.search(self._buffer)
                if m:
                    return m.group(0)
            time.sleep(0.05)
        with self._lock:
            captured = self._buffer
        raise AssertionError(
            f"Pattern '{pattern}' not found within {timeout}s.\nCaptured output:\n{captured}"
        )

    def wait_for_literal_output(self, text, timeout=15):
        """Wait until the literal *text* string appears in the binary's output."""
        end_time = time.time() + float(timeout)
        while time.time() < end_time:
            with self._lock:
                if text in self._buffer:
                    return text
            time.sleep(0.05)
        with self._lock:
            captured = self._buffer
        raise AssertionError(
            f"Text '{text}' not found within {timeout}s.\nCaptured output:\n{captured}"
        )

    def stop_native_binary(self):
        """Terminate the native binary (called on suite teardown)."""
        if self._process and self._process.poll() is None:
            self._process.terminate()
            try:
                self._process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self._process.kill()
