#!/usr/bin/env python3
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
Sorting of parsed log messages for Dictionary-based Logging

In multi-domain systems messages from a remote core are delivered through IPC, which adds
a delay. As a result, messages may appear in the log stream out of order. The sorter caches
parsed messages for a configurable time and prints them ordered by their timestamps.
"""

import heapq
import sys
import time


class LogSorter:
    """Cache parsed log messages and output them sorted by timestamp.

    A message is kept in the cache until it is "old enough", i.e. until the newest timestamp
    seen so far is at least ``latency_ms`` ahead of the message timestamp. Messages which become
    old enough are printed (in timestamp order) whenever new messages are added. If no new
    message was added for ``latency_ms`` (wall clock time) then all cached messages are printed.

    :param latency_ms: Time (in milliseconds) for which messages are cached.
    :param ts_freq: Frequency of the log timestamp in Hz.
    :param ts_bits: Width of the log timestamp in bits (used to handle wrapping).
    :param output: Callable which gets text of the message to be printed.
    :param clock: Function returning the current time in seconds (monotonic).
    """

    def __init__(self, latency_ms, ts_freq, ts_bits=32, output=None, clock=time.monotonic):
        if latency_ms < 0:
            raise ValueError("Latency must not be negative.")
        if ts_freq <= 0:
            raise ValueError("Timestamp frequency must be positive.")

        self.latency_s = latency_ms / 1000.0
        self.latency_ticks = int(round(self.latency_s * ts_freq))
        self.ts_mod = 1 << ts_bits
        self.output = output if output is not None else self._default_output
        self.clock = clock

        self._heap = []
        self._seq = 0
        # Reference timestamp (raw and extended - not wrapping) of the newest message.
        self._ref_raw = None
        self._ref_ext = 0
        self._last_arrival = None

    @staticmethod
    def _default_output(text):
        sys.stdout.write(text)
        sys.stdout.flush()

    def _extend_timestamp(self, timestamp):
        """Convert wrapping timestamp to a monotonic one relative to the newest message."""
        if self._ref_raw is None:
            self._ref_raw = timestamp
            self._ref_ext = timestamp
            return timestamp

        delta = (timestamp - self._ref_raw) % self.ts_mod
        if delta >= self.ts_mod // 2:
            delta -= self.ts_mod

        ext = self._ref_ext + delta
        if ext > self._ref_ext:
            self._ref_raw = timestamp
            self._ref_ext = ext

        return ext

    def add(self, timestamp, text):
        """Add a message to the cache and print the messages which are old enough.

        :param timestamp: Raw timestamp of the message, or None if the message has no timestamp
                          (it is then placed together with the newest message).
        :param text: Text to be printed for this message.
        """
        if timestamp is None:
            ext = self._ref_ext
        else:
            ext = self._extend_timestamp(timestamp)

        heapq.heappush(self._heap, (ext, self._seq, text))
        self._seq += 1
        self._last_arrival = self.clock()

        limit = self._ref_ext - self.latency_ticks
        while self._heap and self._heap[0][0] <= limit:
            self.output(heapq.heappop(self._heap)[2])

    def flush(self):
        """Print all cached messages."""
        while self._heap:
            self.output(heapq.heappop(self._heap)[2])

    def flush_if_idle(self):
        """Print all cached messages if no message was added for the latency time.

        :return: True if the messages were printed.
        """
        if self._heap and self.clock() - self._last_arrival >= self.latency_s:
            self.flush()
            return True

        return False

    def timeout(self):
        """Time (in seconds) after which :meth:`flush_if_idle` should be called.

        :return: Remaining time or None if there are no cached messages.
        """
        if not self._heap:
            return None

        return max(0.0, self._last_arrival + self.latency_s - self.clock())
