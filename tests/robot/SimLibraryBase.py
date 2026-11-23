# Copyright (c) 2026 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
"""Base class shared by NativeSimLibrary and QemuSimLibrary."""

import re
import threading
import time


class SimLibraryBase:
    """Common Robot Framework keyword base for Zephyr simulator libraries.

    Provides shared output-buffering state and the ``wait_for_output`` /
    ``wait_for_literal_output`` keywords used by both the native_sim and
    QEMU back-ends.
    """

    ROBOT_LIBRARY_SCOPE = 'SUITE'

    def __init__(self):
        self._buffer = ''
        self._lock = threading.Lock()
        self._reader = None

    @staticmethod
    def _strip_ansi(data):
        """Decode *data* bytes and strip ANSI escape sequences."""
        return re.sub(
            r'\x1b\[[0-9;]*[mABCDEFGHJKSTfhilmnsu]',
            '',
            data.decode('utf-8', errors='replace'),
        )

    def wait_for_output(self, pattern, timeout=15):
        """Wait until *pattern* (regex) appears in the simulator's output."""
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
        """Wait until the literal *text* string appears in the simulator's output."""
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
