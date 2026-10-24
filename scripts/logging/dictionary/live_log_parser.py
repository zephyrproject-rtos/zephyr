#!/usr/bin/env python3
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
Log Parser for Dictionary-based Logging

Decode binary dictionary log streams from serial (UART), a file, or J-Link RTT
using one or more JSON log database files (``log_dictionary.json``).
"""

import argparse
import contextlib
import logging
import os
import select
import sys
import time

import parserlib
import serial

try:
    # Pylink is an optional dependency for RTT reading, which requires it's own installation.
    # Don't fail, unless the user tries to use RTT reading.
    import pylink
except ImportError:
    pylink = None

LOGGER_FORMAT = "%(message)s"
logger = logging.getLogger("parser")


class SerialReader:
    """Class to read data from serial port and parse it"""

    def __init__(self, serial_port, baudrate):
        self.serial_port = serial_port
        self.baudrate = baudrate
        self.serial = None

    @contextlib.contextmanager
    def open(self):
        try:
            self.serial = serial.Serial(self.serial_port, self.baudrate)
            yield
        finally:
            self.serial.close()

    def fileno(self):
        return self.serial.fileno()

    def read_non_blocking(self):
        size = self.serial.in_waiting
        return self.serial.read(size)


class FileReader:
    """Class to read data from serial port and parse it"""

    def __init__(self, filepath):
        self.filepath = filepath
        self.file = None

    @contextlib.contextmanager
    def open(self):
        if self.filepath is not None:
            with open(self.filepath, 'rb') as f:
                self.file = f
                yield
        else:
            sys.stdin = os.fdopen(sys.stdin.fileno(), 'rb', 0)
            self.file = sys.stdin
            yield

    def fileno(self):
        return self.file.fileno()

    def read_non_blocking(self):
        # Read available data using a reasonable buffer size (without buffer size, this blocks
        # forever, but with buffer size it returns even when less data than the buffer read was
        # available).
        return self.file.read(1024)


class JLinkRTTReader:
    """Class to read data from JLink's RTT"""

    @staticmethod
    def _create_jlink_connection(lib_path):
        if pylink is None:
            raise ImportError(
                "pylink module is required for RTT reading. "
                "Please install it using 'pip install pylink-square'."
            )

        if lib_path is not None:
            lib = pylink.Library(lib_path, True)
            jlink = pylink.JLink(lib)
        else:
            jlink = pylink.JLink()

        return jlink

    @contextlib.contextmanager
    def open(self):
        try:
            self.jlink.open()
            self.jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
            if self.speed != 0:
                self.jlink.connect(self.target_device, self.speed)
            else:
                self.jlink.connect(self.target_device)

            self.jlink.rtt_start(self.block_address)

            # Wait for the JLINK RTT buffers to be initialized.
            up_down_initialized = False
            while not up_down_initialized:
                try:
                    _ = self.jlink.rtt_get_num_up_buffers()
                    _ = self.jlink.rtt_get_num_down_buffers()
                    up_down_initialized = True
                except pylink.errors.JLinkRTTException:
                    time.sleep(0.1)

            yield

        finally:
            self.close()

    def __init__(self, target_device, block_address, channel, speed, lib_path):
        self.target_device = target_device
        self.block_address = block_address
        self.speed = speed
        self.channel = channel

        self.jlink = self._create_jlink_connection(lib_path)

    def close(self):
        # JLink closes the connection through the __del__ method.
        del self.jlink

    def read_non_blocking(self):
        return bytes(self.jlink.rtt_read(self.channel, 1024))


INPUT_MODES = ("serial", "file", "jlink-rtt")

_USAGE = (
    "%(prog)s [options] DBFILE [DBFILE ...] MODE [mode arguments]\n"
    "\n"
    "  DBFILE       Path to log_dictionary.json from the Zephyr build. Pass one file per "
    "domain, or several paths separated by ';' in a single argument.\n"
    "  MODE         One of: serial, file, jlink-rtt (see examples below)."
)

_EPILOG = """\
input modes:

  serial PORT BAUDRATE
      Read binary dictionary logs from a UART (for example /dev/ttyACM0 115200).

  file [FILEPATH]
      Read from FILEPATH. If FILEPATH is omitted, read binary data from stdin.

  jlink-rtt DEVICE
      Read from SEGGER J-Link RTT (requires the pylink-square package).
      Optional: --block-address ADDR (hex), --channel N (default 0),
      --speed HZ (default 0, use J-Link default), --lib-path PATH.

examples:

  %(prog)s build/zephyr/log_dictionary.json serial /dev/ttyACM0 115200

  %(prog)s app/log_dictionary.json remote/log_dictionary.json \\
      --latency 20 --ts_freq 32768 serial /dev/ttyACM0 115200

  %(prog)s build/zephyr/log_dictionary.json file capture.bin

  %(prog)s build/zephyr/log_dictionary.json jlink-rtt nrf5340_xxaa_app \\
      --channel 0
"""


def _mode_argument_parser():
    """Parser for arguments following the input mode subcommand."""
    parser = argparse.ArgumentParser(add_help=False)
    subparsers = parser.add_subparsers(dest="mode", required=True)

    serial_parser = subparsers.add_parser(
        "serial",
        help="read dictionary logs from a UART",
        description="Read binary dictionary log data from serial PORT at BAUDRATE.",
    )
    serial_parser.add_argument("port", help="Serial port device (e.g. /dev/ttyACM0)")
    serial_parser.add_argument("baudrate", type=int, help="UART baud rate (e.g. 115200)")

    file_parser = subparsers.add_parser(
        "file",
        help="read dictionary logs from a file or stdin",
        description="Read binary dictionary log data from FILEPATH, or from stdin if omitted.",
    )
    file_parser.add_argument(
        "filepath", nargs="?", default=None, help="Input file path, leave empty for stdin"
    )

    jlink_rtt_parser = subparsers.add_parser(
        "jlink-rtt",
        help="read dictionary logs from J-Link RTT",
        description=(
            "Read binary dictionary log data over SEGGER J-Link RTT. "
            "Requires pylink-square (pip install pylink-square)."
        ),
    )
    jlink_rtt_parser.add_argument(
        "target_device", help="Device Name (see https://www.segger.com/supported-devices/jlink/)"
    )
    jlink_rtt_parser.add_argument(
        "--block-address", help="RTT block address in hex", type=lambda x: int(x, 16)
    )
    jlink_rtt_parser.add_argument("--channel", type=int, help="RTT channel number", default=0)
    jlink_rtt_parser.add_argument("--speed", type=int, help="Reading speed", default="0")
    jlink_rtt_parser.add_argument("--lib-path", help="Path to libjlinkarm.so library")

    return parser


def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(
        allow_abbrev=False,
        usage=_USAGE,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=(
            "Decode dictionary logs in real time from serial, a file, or J-Link RTT."
        ),
        epilog=_EPILOG,
    )
    parser.add_argument("--debug", action="store_true", help="Print extra debugging information")
    parser.add_argument(
        "--polling-interval",
        type=float,
        default=0.1,
        help="Interval for polling input source, if it does not support 'select'",
    )

    parserlib.add_sorting_arguments(parser)

    args, remainder = parser.parse_known_args()
    parserlib.check_sorting_arguments(parser, args)

    if not remainder:
        parser.error(
            "at least one dbfile and input mode are required "
            "(e.g. app/log_dictionary.json remote/log_dictionary.json serial /dev/ttyACM0 115200)"
        )

    try:
        mode_idx = next(i for i, tok in enumerate(remainder) if tok in INPUT_MODES)
    except StopIteration:
        parser.error("the following arguments are required: mode")

    args.dbfile = parserlib.normalize_dbfiles(remainder[:mode_idx])
    if not args.dbfile:
        parser.error("at least one dbfile is required")

    mode_argv = remainder[mode_idx:]
    mode_ns = _mode_argument_parser().parse_args(mode_argv)

    args.mode = mode_ns.mode
    args.port = getattr(mode_ns, "port", None)
    args.baudrate = getattr(mode_ns, "baudrate", None)
    args.filepath = getattr(mode_ns, "filepath", None)
    args.target_device = getattr(mode_ns, "target_device", None)
    args.block_address = getattr(mode_ns, "block_address", None)
    args.channel = getattr(mode_ns, "channel", 0)
    args.speed = getattr(mode_ns, "speed", 0)
    args.lib_path = getattr(mode_ns, "lib_path", None)

    return args


def main():
    """function of serial parser"""
    args = parse_args()

    for one_db in args.dbfile:
        if ".json" not in one_db:
            logger.error("ERROR: invalid log database path: %s, exiting...", one_db)
            sys.exit(1)

    logging.basicConfig(format=LOGGER_FORMAT)

    if args.debug:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    def load_log_parser():
        """Read the database files and create the log parser. Returns None on failure."""
        try:
            return parserlib.get_log_parser(
                args.dbfile, logger, latency_ms=args.latency, ts_freq=args.ts_freq
            )
        except Exception as e:  # noqa: BLE001
            logger.error("ERROR: Failed to load the log database: %s", e)
            return None

    # Stdin in file mode is fed with binary log data converted from a hexadecimal UART stream
    # (after the ##ZLOGV1## marker). That stream does not include the binary synchronization
    # sequence, so there is nothing to wait for and no way to resynchronize.
    stdin_mode = args.mode == "file" and args.filepath is None

    data = b''
    log_parser = None
    # Input data is skipped until the synchronization sequence is found. The database files are
    # read only after that, so if the target is reflashed (and the databases changed), the new
    # databases are picked up when the synchronization sequence is received again.
    synced = False

    if stdin_mode:
        log_parser = load_log_parser()
        if log_parser is None:
            sys.exit(1)
        synced = True

    if args.mode == "serial":
        reader = SerialReader(args.port, args.baudrate)
    elif args.mode == "file":
        reader = FileReader(args.filepath)
    elif args.mode == "jlink-rtt":
        reader = JLinkRTTReader(
            args.target_device, args.block_address, args.channel, args.speed, args.lib_path
        )
    else:
        raise ValueError(f"Invalid mode selected: {args.mode!r}.")

    # Set when buffered data has to be searched for the synchronization sequence before waiting
    # for any new data.
    rescan = False

    with reader.open():
        while True:
            if rescan:
                rescan = False
            else:
                # When messages are cached for sorting, do not wait for new data longer than
                # it is needed to print them.
                idle_timeout = log_parser.idle_timeout() if log_parser is not None else None
                if hasattr(reader, 'fileno') and sys.platform != "win32":
                    _, _, _ = select.select([reader], [], [], idle_timeout)
                else:
                    if idle_timeout is None:
                        time.sleep(args.polling_interval)
                    else:
                        time.sleep(min(args.polling_interval, idle_timeout))
                data += reader.read_non_blocking()

            if not synced:
                synced, remaining = parserlib.skip_to_sync(data)
                if synced:
                    logger.debug("Synchronization sequence found")
                elif len(remaining) != len(data):
                    logger.debug(
                        "Skipped %d bytes while waiting for sync", len(data) - len(remaining)
                    )
                data = remaining
                if not synced:
                    continue

                # Database files are read every time the synchronization is (re)established,
                # because they may have changed since the previous one.
                log_parser = load_log_parser()
                if log_parser is None:
                    logger.error("Waiting for the next synchronization sequence...")
                    synced = False
                    continue

            parsed_data_offset, error = parserlib.parser(data, log_parser, logger)
            if error is not None:
                # The data does not match the database, most likely the target was reflashed
                # and the databases changed. Print what was already decoded and look for the
                # synchronization sequence in the data (and in the data which will follow).
                log_parser.flush()
                if stdin_mode:
                    logger.error("ERROR: Failed to process log data: %s", error)
                    sys.exit(1)
                logger.error(
                    "ERROR: Failed to process log data: %s. Waiting for synchronization...", error
                )
                log_parser = None
                synced = False
                rescan = True
                continue

            data = data[parsed_data_offset:]

            # If no new messages arrived for the latency time, print what is still cached.
            log_parser.flush_if_idle()


if __name__ == "__main__":
    main()
