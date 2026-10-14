#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

"""
Pytest harness for the multidomain logging sample using the UART dictionary frontend.

The host domain (cpu0) outputs the binary dictionary log data on the UART. Messages from the
remote domain (cpu1) are forwarded to the host domain over IPC and are sent over the same UART.
The test starts ``live_log_parser.py`` with the log databases of both domains, which decodes the
messages coming from the UART of the device, flashes (and thereby resets) the device and then
verifies that the decoded messages match the logs produced by the sample.

Messages of the remote domain are delayed by the IPC transport, so the parser is asked to sort
the messages by the timestamp.
"""

import logging
import os
import re
import subprocess
import sys
import threading
import time
from pathlib import Path

import pytest
from twister_harness import DeviceAdapter
from twister_harness.helpers.utils import find_in_config

logger = logging.getLogger(__name__)

ZEPHYR_BASE = Path(os.environ.get("ZEPHYR_BASE", Path(__file__).resolve().parents[5]))
LIVE_LOG_PARSER = ZEPHYR_BASE / "scripts" / "logging" / "dictionary" / "live_log_parser.py"

# Maximum time to wait for all expected messages after the device was flashed.
CAPTURE_TIMEOUT = 30.0

# Time in milliseconds for which the parser caches messages to sort them by the timestamp.
SORTING_LATENCY_MS = 20

ANSI_ESCAPE = re.compile(r'\x1b\[[0-9;]*[A-Za-z]')

# Prefix of a log message: "[    123456] <inf> "
LOG_PREFIX = r'[\s]*\[[0-9,:\. ]+\] <inf> '


def log_regex(domain, body):
    return re.compile(LOG_PREFIX + rf'cpu{domain}\.app: {body}')


# Messages printed by the host domain (see src/main.c), in the order they are emitted.
HOST_EXPECTED = [
    # *** Booting Zephyr OS build <version> ***
    re.compile(r'[*]{3} Booting Zephyr OS build [0-9a-z.-]+'),
    # Hello World! <board name> (printk redirected to the logging)
    re.compile(r'^[\s]*Hello World! [\w-]+'),
    log_regex(0, 'Multidomain logging HOST demo started'),
    *[log_regex(0, rf'loop: {i}') for i in range(10)],
    *[log_regex(0, rf'HOST \[1\]: {i}') for i in range(1, 10, 2)],
    log_regex(0, 'Multidomain logging HOST demo ended.'),
    log_regex(0, r'CPU load: [0-9]+\.[0-9]+%'),
]

# Messages printed by the remote domain (see remote/src/main.c), in the order they are emitted.
REMOTE_EXPECTED = [
    log_regex(1, r'IPC-service REMOTE \[INST 1\] demo started'),
    log_regex(1, 'wait for bound'),
    *[log_regex(1, rf'loop: {i}') for i in range(10)],
    log_regex(1, 'bounded'),
    *[log_regex(1, rf'REMOTE \[1\]: {i}') for i in range(0, 9, 2)],
    # The last two messages of the remote domain ("demo ended" and "CPU load") are not expected.
    # In the configurations used with this test the multithreading is disabled in the remote
    # domain and these messages, emitted right before the remote application becomes idle, are
    # not delivered to the host domain (the same is observed without the dictionary mode).
]


class LiveLogParser:
    """Run live_log_parser.py on a serial port and collect the decoded lines."""

    def __init__(
        self,
        dictionary_jsons: list[Path],
        output_file: Path,
        port: str,
        baud: int,
        ts_freq: int,
    ):
        self.lines: list[str] = []
        self._lock = threading.Lock()
        self._output_file = output_file
        cmd = [
            sys.executable,
            str(LIVE_LOG_PARSER),
            '--latency',
            str(SORTING_LATENCY_MS),
            '--ts_freq',
            str(ts_freq),
            *[str(db) for db in dictionary_jsons],
            'serial',
            port,
            str(baud),
        ]
        logger.info('Starting live log parser: %s', ' '.join(cmd))
        env = os.environ.copy()
        env['PYTHONUNBUFFERED'] = '1'
        self._proc = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env
        )
        self._thread = threading.Thread(target=self._read, daemon=True)
        self._thread.start()

    def _read(self):
        with open(self._output_file, 'w', encoding='utf-8') as fp:
            for raw_line in self._proc.stdout:
                line = raw_line.decode(errors='replace')
                line = ANSI_ESCAPE.sub('', line).rstrip('\r\n')
                fp.write(line + '\n')
                fp.flush()
                with self._lock:
                    self.lines.append(line)

    def is_running(self) -> bool:
        return self._proc.poll() is None

    def snapshot(self) -> list[str]:
        with self._lock:
            return list(self.lines)

    def stop(self):
        if self._proc.poll() is None:
            self._proc.terminate()
            try:
                self._proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self._proc.kill()
                self._proc.wait()
        self._thread.join(timeout=5)


def match_in_order(
    lines: list[str], expected: list[re.Pattern], verbose: bool = False
) -> list[re.Pattern]:
    """Return expected patterns which were not found, matching them in order.

    Each pattern must match a line placed after the line matched by the previous pattern.
    """
    missing = []
    start = 0
    for pattern in expected:
        for idx in range(start, len(lines)):
            if pattern.search(lines[idx]):
                if verbose:
                    logger.info('Found: %s', lines[idx].strip())
                start = idx + 1
                break
        else:
            if verbose:
                logger.error('NOT FOUND: %s', pattern.pattern)
            missing.append(pattern)
    return missing


def all_missing(lines: list[str], verbose: bool = False) -> list[re.Pattern]:
    """Return messages which were not found.

    Messages of each domain are expected in the order of emission. Ordering between the domains
    is not verified, except for the order resulting from sorting by the timestamp.
    """
    return match_in_order(lines, HOST_EXPECTED, verbose) + match_in_order(
        lines, REMOTE_EXPECTED, verbose
    )


def flash_device(dut: DeviceAdapter):
    """Flash the application and reset the device without opening the serial port."""
    dut.generate_command()
    logger.info('Flashing: %s', ' '.join(dut.command))
    result = subprocess.run(
        dut.command,
        env=dut.env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors='replace',
        timeout=dut.device_config.flash_timeout,
    )
    logger.debug(result.stdout)
    assert result.returncode == 0, f'Flashing failed:\n{result.stdout}'


def test_multidomain_dictionary(unlaunched_dut: DeviceAdapter):
    """Verify that dictionary logs of both domains sent over the UART are decoded correctly."""
    if unlaunched_dut.device_config.type != 'hardware':
        pytest.skip('Test requires a device with the UART connected to the host')

    assert LIVE_LOG_PARSER.is_file(), f'{LIVE_LOG_PARSER} not found'

    # With sysbuild the application build directory is the one of the main image (host domain).
    # The build directory of the remote image is its sibling.
    host_build_dir = Path(unlaunched_dut.device_config.app_build_dir)
    remote_build_dir = host_build_dir.parent / 'remote'
    # The first database is the one of the domain which outputs logs (host).
    dictionary_jsons = [
        host_build_dir / 'zephyr' / 'log_dictionary.json',
        remote_build_dir / 'zephyr' / 'log_dictionary.json',
    ]
    for dictionary_json in dictionary_jsons:
        assert dictionary_json.is_file(), f'{dictionary_json} not found'

    config_file = host_build_dir / 'zephyr' / '.config'
    ts_freq = find_in_config(config_file, 'CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC')
    assert ts_freq, f'CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC not found in {config_file}'

    # The device adapter is intentionally not launched. It would open the serial port which
    # would then be unavailable for the log parser.
    serial_config = unlaunched_dut.device_config.serial_configs[0]
    output_file = host_build_dir / 'decoded.log'
    # Start capturing before the device is flashed and reset to not miss any message.
    parser = LiveLogParser(
        dictionary_jsons,
        output_file,
        port=serial_config.port,
        baud=serial_config.baud,
        ts_freq=int(ts_freq),
    )

    try:
        time.sleep(1.0)
        assert parser.is_running(), 'Live log parser exited prematurely:\n' + '\n'.join(
            parser.snapshot()
        )

        flash_device(unlaunched_dut)

        deadline = time.monotonic() + CAPTURE_TIMEOUT
        while time.monotonic() < deadline:
            if not all_missing(parser.snapshot()) or not parser.is_running():
                break
            time.sleep(0.5)
    finally:
        parser.stop()

    decoded = parser.snapshot()
    logger.info('Decoded logs:\n%s', '\n'.join(decoded))
    missing = all_missing(decoded, verbose=True)
    assert not missing, (
        'Expected log messages not found: '
        + ', '.join(p.pattern for p in missing)
        + '\nParser output:\n'
        + '\n'.join(decoded)
    )
