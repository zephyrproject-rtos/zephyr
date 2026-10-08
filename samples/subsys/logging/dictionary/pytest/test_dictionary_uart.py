#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

"""
Pytest harness for the dictionary logging sample with output on the console UART.

The log messages are sent either in the binary format (UART frontend) or as a hexadecimal
string (UART backend). The test starts ``live_log_parser.py`` which decodes messages coming from
the UART of the device, starts the device and then verifies that the decoded messages match the
logs produced by the sample.

On hardware, the device is flashed (and thereby reset) and the UART is the serial port of the
board. On QEMU, the UART is connected to the FIFO created by the twister harness.
"""

import binascii
import logging
import os
import re
import string
import subprocess
import sys
import threading
import time
from pathlib import Path

import serial
from twister_harness import DeviceAdapter
from twister_harness.device.fifo_handler import FifoHandler
from twister_harness.helpers.utils import find_in_config

logger = logging.getLogger(__name__)

ZEPHYR_BASE = Path(os.environ.get("ZEPHYR_BASE", Path(__file__).resolve().parents[5]))
LIVE_LOG_PARSER = ZEPHYR_BASE / "scripts" / "logging" / "dictionary" / "live_log_parser.py"

# Maximum time to wait for all expected messages after the device was flashed.
CAPTURE_TIMEOUT = 30.0

# Marker which precedes the hexadecimal log data.
HEX_LOG_MARKER = "##ZLOGV1##"

# Synchronization sequence sent by the UART frontend at the beginning of the binary stream (see
# SYNC_SEQUENCE in scripts/logging/dictionary/parserlib.py). Anything which precedes it is not
# a log data, e.g. the firmware (SeaBIOS) of the x86 QEMU targets prints a banner on the UART
# before the application is started.
BINARY_SYNC_SEQUENCE = bytes([0x44, 0x9C, 0x64, 0xCF, 0x84, 0x1C, 0xFB, 0x40, 0x73, 0x2F])

ANSI_ESCAPE = re.compile(r'\x1b\[[0-9;]*[A-Za-z]')

# Prefix of a log message: "[00:00:00.123,456,789] "
LOG_PREFIX = r'[\s]*\[[0-9,:\. ]+\] '


def log_regex(level, body, module='hello_world'):
    return re.compile(LOG_PREFIX + rf'<{level}> {module}: (?:\w+: )?{body}')


# Messages produced by the sample (see src/main.c), in the order they are emitted. Each entry is
# a tuple: log level of the message (None for messages which are not filtered by level) and the
# regular expression matching the decoded line.
EXPECTED = [
    # *** Booting Zephyr OS build <version> ***
    (None, re.compile(r'[*]{3} Booting Zephyr OS build [0-9a-z.-]+')),
    # Hello World! <board name>
    (None, re.compile(r'^[\s]*Hello World! [\w-]+')),
    ('err', log_regex('err', 'error string')),
    ('dbg', log_regex('dbg', 'debug string')),
    ('inf', log_regex('inf', 'info string')),
    ('dbg', log_regex('dbg', 'int8_t 1, uint8_t 2')),
    ('dbg', log_regex('dbg', 'int16_t 16, uint16_t 17')),
    ('dbg', log_regex('dbg', 'int32_t 32, uint32_t 33')),
    ('dbg', log_regex('dbg', 'int64_t 64, uint64_t 65')),
    ('dbg', log_regex('dbg', 'char !')),
    ('dbg', log_regex('dbg', 's str static str c str')),
    ('dbg', log_regex('dbg', 'd str dynamic str')),
    (
        'dbg',
        log_regex(
            'dbg',
            'mixed str dynamic str --- dynamic str --- another dynamic str --- another dynamic str',
        ),
    ),
    ('dbg', log_regex('dbg', 'mixed c/s ! static str dynamic str static str !')),
    ('dbg', log_regex('dbg', 'pointer 0x[0-9a-f]+')),
    ('dbg', log_regex('dbg', 'For HeXdUmP!')),
    # 48 45 58 44 55 4d 50 21  20 48 45 58 44 55 4d 50 |HEXDUMP!  HEXDUMP
    ('dbg', re.compile(r'^[\s]*[0-9a-f ]{48,52}[|]HEXDUMP!  HEXDUMP')),
    # 40 20 48 45 58 44 55 4d  50 23                   |@ HEXDUM P#
    ('dbg', re.compile(r'^[\s]*[0-9a-f ]{48,52}[|]@ HEXDUM P#')),
]

# Messages produced only when CONFIG_FPU is enabled.
EXPECTED_FPU = [
    # float 66.669998, double 68.690000
    ('dbg', log_regex('dbg', r'float 66\.[0-9]+, double 68\.[0-9]+')),
]

# Messages produced only when CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE is enabled.
EXPECTED_LONG_DOUBLE = [
    # long double 70.710000
    ('dbg', log_regex('dbg', r'long double 70\.[0-9]+')),
]


def expected_messages(config_file: Path) -> tuple[list[re.Pattern], list[re.Pattern]]:
    """Return messages expected and messages not expected for the sample configuration."""
    messages = list(EXPECTED)
    if find_in_config(config_file, 'CONFIG_FPU') == 'y':
        messages += EXPECTED_FPU
        if find_in_config(config_file, 'CONFIG_CBPRINTF_PACKAGE_LONGDOUBLE') == 'y':
            messages += EXPECTED_LONG_DOUBLE

    if find_in_config(config_file, 'CONFIG_LOG_RUNTIME_FILTERING') == 'y':
        # With the runtime filtering, the initial level of a module is the default level (info),
        # so debug messages are filtered out even though they are enabled at compile time.
        expected = [regex for level, regex in messages if level != 'dbg']
        unexpected = [regex for level, regex in messages if level == 'dbg']
    else:
        expected = [regex for _, regex in messages]
        unexpected = []

    return expected, unexpected


class SerialStream:
    """Raw byte stream coming from a serial port."""

    def __init__(self, port: str, baud: int):
        self._serial = serial.Serial(port, baud, timeout=0.1)

    def read(self, size: int) -> bytes:
        """Return available data. An empty result means that there is no data yet."""
        return self._serial.read(size)

    def close(self):
        self._serial.close()


class FifoStream:
    """Raw byte stream coming from the FIFO to which QEMU redirects the UART."""

    def __init__(self, fifo: FifoHandler):
        self._fifo = fifo

    def read(self, size: int) -> bytes:
        """Block until some data is available. Raise EOFError when QEMU is gone."""
        try:
            data = self._fifo.read(size)
        except (OSError, ValueError) as exc:
            raise EOFError from exc
        if not data:
            raise EOFError
        return data

    def close(self):
        # The FIFO is owned (and closed) by the device adapter.
        pass


class StreamFeeder(threading.Thread):
    """Read log data from a byte stream and feed the parser with binary data.

    If ``hex_data`` is set, the stream carries log data encoded as hexadecimal string, which
    follows the HEX_LOG_MARKER. Otherwise the stream is passed to the parser as it is, except
    for the data which precedes the ``preamble_end`` (if not empty). The parser started in the
    ``file`` mode expects the data which is already synchronized.
    """

    def __init__(self, stream, sink, hex_data: bool, preamble_end: bytes = b''):
        super().__init__(daemon=True)
        self._stream = stream
        self._sink = sink
        self._hex_data = hex_data
        self._preamble_end = preamble_end
        self._stop_event = threading.Event()

    def run(self):
        hex_digits = set(string.hexdigits)
        text = ''
        started = False
        leftover = ''
        preamble = b''
        in_preamble = bool(self._preamble_end) and not self._hex_data
        while not self._stop_event.is_set():
            try:
                raw = self._stream.read(4096)
            except EOFError:
                break
            if not raw:
                continue

            if in_preamble:
                preamble += raw
                idx = preamble.find(self._preamble_end)
                if idx < 0:
                    continue
                in_preamble = False
                raw = preamble[idx + len(self._preamble_end) :]
                if not raw:
                    continue

            if not self._hex_data:
                if not self._write(raw):
                    break
                continue

            chunk = raw.decode('latin-1')
            if not started:
                # Everything before the marker is not a log data.
                text += chunk
                idx = text.find(HEX_LOG_MARKER)
                if idx < 0:
                    text = text[-(len(HEX_LOG_MARKER) - 1) :]
                    continue
                started = True
                chunk = text[idx + len(HEX_LOG_MARKER) :]

            digits = leftover + ''.join(c for c in chunk if c in hex_digits)
            even = len(digits) // 2 * 2
            leftover = digits[even:]
            if even and not self._write(binascii.unhexlify(digits[:even])):
                break

    def _write(self, data: bytes) -> bool:
        try:
            self._sink.write(data)
            self._sink.flush()
        except (BrokenPipeError, ValueError):
            # Parser is gone.
            return False
        return True

    def stop(self):
        self._stop_event.set()
        self.join(timeout=2)
        self._stream.close()


class LiveLogParser:
    """Run live_log_parser.py and collect the decoded lines.

    If a ``stream`` is not given, binary log data is decoded straight from the serial port.
    Otherwise the data is read from the stream by this class (and converted to the binary form
    if it is hexadecimal) and passed to the parser.
    """

    def __init__(
        self,
        dictionary_json: Path,
        output_file: Path,
        port: str = '',
        baud: int = 115200,
        stream=None,
        hex_data: bool = False,
        preamble_end: bytes = b'',
    ):
        self.lines: list[str] = []
        self._lock = threading.Lock()
        self._output_file = output_file
        self._feeder = None
        cmd = [sys.executable, str(LIVE_LOG_PARSER), str(dictionary_json)]
        cmd += ['file'] if stream else ['serial', port, str(baud)]
        logger.info('Starting live log parser: %s', ' '.join(cmd))
        env = os.environ.copy()
        env['PYTHONUNBUFFERED'] = '1'
        self._proc = subprocess.Popen(
            cmd,
            stdin=subprocess.PIPE if stream else None,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            env=env,
        )
        self._thread = threading.Thread(target=self._read, daemon=True)
        self._thread.start()
        if stream:
            self._feeder = StreamFeeder(stream, self._proc.stdin, hex_data, preamble_end)
            self._feeder.start()

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
        if self._feeder is not None:
            self._feeder.stop()
        if self._proc.poll() is None:
            self._proc.terminate()
            try:
                self._proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self._proc.kill()
                self._proc.wait()
        if self._proc.stdin is not None:
            self._proc.stdin.close()
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


def start_qemu(dut: DeviceAdapter) -> FifoStream:
    """Start QEMU and return the stream of the data sent by the application over the UART.

    The device adapter is not launched with ``launch()`` because it would start a thread reading
    the FIFO line by line as a text, which is not suitable for the binary log data.
    """
    dut.generate_command()
    logger.info('Running: %s', ' '.join(dut.command))
    dut._run_subprocess()
    connection = dut.connections[0]
    connection.connect()
    # pylint: disable-next=protected-access
    return FifoStream(connection._fifo_connection)


def test_dictionary_uart(unlaunched_dut: DeviceAdapter):
    """Verify that dictionary logs sent over the UART are decoded correctly."""
    assert LIVE_LOG_PARSER.is_file(), f'{LIVE_LOG_PARSER} not found'

    build_dir = Path(unlaunched_dut.device_config.app_build_dir)
    dictionary_json = build_dir / 'zephyr' / 'log_dictionary.json'
    assert dictionary_json.is_file(), f'{dictionary_json} not found'

    config_file = build_dir / 'zephyr' / '.config'
    expected, unexpected = expected_messages(config_file)
    hex_data = find_in_config(config_file, 'CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY_HEX') == 'y'
    logger.info(
        'Hexadecimal output: %s, expected messages: %d, filtered out: %d',
        hex_data,
        len(expected),
        len(unexpected),
    )

    # The device adapter is intentionally not launched. It would open the serial port which
    # would then be unavailable for the log parser.
    output_file = build_dir / 'decoded.log'
    if unlaunched_dut.device_config.type == 'qemu':
        # QEMU is started before the parser. The data written by the application is kept in the
        # FIFO until it is read, so no message is lost.
        parser = LiveLogParser(
            dictionary_json,
            output_file,
            stream=start_qemu(unlaunched_dut),
            hex_data=hex_data,
            preamble_end=BINARY_SYNC_SEQUENCE,
        )
        start_device = None
    else:
        serial_config = unlaunched_dut.device_config.serial_configs[0]
        # Start capturing before the device is flashed and reset to not miss any message.
        parser = LiveLogParser(
            dictionary_json,
            output_file,
            port=serial_config.port,
            baud=serial_config.baud,
            stream=SerialStream(serial_config.port, serial_config.baud) if hex_data else None,
            hex_data=hex_data,
        )
        start_device = flash_device

    try:
        time.sleep(1.0)
        assert parser.is_running(), 'Live log parser exited prematurely:\n' + '\n'.join(
            parser.snapshot()
        )

        if start_device:
            start_device(unlaunched_dut)

        deadline = time.monotonic() + CAPTURE_TIMEOUT
        while time.monotonic() < deadline:
            if not match_in_order(parser.snapshot(), expected) or not parser.is_running():
                break
            time.sleep(0.5)
    finally:
        parser.stop()

    decoded = parser.snapshot()
    logger.info('Decoded logs:\n%s', '\n'.join(decoded))
    missing = match_in_order(decoded, expected, verbose=True)
    found_unexpected = [
        regex.pattern for regex in unexpected if any(regex.search(line) for line in decoded)
    ]
    assert not found_unexpected, 'Filtered out messages found: ' + ', '.join(found_unexpected)
    assert not missing, (
        'Expected log messages not found: '
        + ', '.join(p.pattern for p in missing)
        + '\nParser output:\n'
        + '\n'.join(decoded)
    )
