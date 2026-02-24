# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import queue
import signal
import sys
import threading
import time
from contextlib import nullcontext, suppress
from datetime import datetime

import jsonschema
import serial
import yaml
from serial.tools import list_ports
from west import log
from west.commands import CommandError, WestCommand

from build_helpers import find_build_dir, is_zephyr_build
from zcmake import CMakeCache
from zephyr_ext_common import ZEPHYR_BASE

try:
    import termios
    import tty

    _HAS_TERMIOS = os.name == "posix"
except ImportError:
    _HAS_TERMIOS = False

try:
    import fcntl

    _HAS_FCNTL = True
except ImportError:
    _HAS_FCNTL = False

_ON_WINDOWS = os.name == "nt"
if _ON_WINDOWS:
    try:
        import msvcrt
    except ImportError:
        msvcrt = None
else:
    try:
        import select
    except ImportError:
        select = None


DEFAULT_RESET_DELAY_MS = 25
DEFAULT_RESET_HOLD_MS = 100
DEFAULT_LINE_MAP = "dtr-boot,rts-reset"
DEFAULT_INVERT_RESET = True
DEFAULT_INVERT_BOOT = True
DEFAULT_IDLE_LINES = "released"
DEFAULT_RESET_KEY = "r"

VALID_IDLE_MODES = ["released", "boot-hold", "reset-hold"]
VALID_RESET_MODES = ["none", "run", "boot", "dtr-pulse", "rts-pulse"]
VALID_READ_MODES = ["auto", "line", "raw"]

DEFAULT_BAUD_RATE = 115200
DEFAULT_RTSCTS = False
DEFAULT_READ_MODE = "auto"
DEFAULT_IDLE_FLUSH_MS = 60
DEFAULT_SERIAL_TIMEOUT = 0.05
DEFAULT_RECONNECT_INTERVAL = 0.5
DEFAULT_QUEUE_SIZE = 200

DEFAULT_EXIT_CHAR = "]"

DEFAULT_TIMESTAMP = False

DEFAULT_BUILD_DIR = "build"
MONITOR_YAML = "monitor.yaml"
MONITOR_YAML_HIDDEN = ".monitor.yaml"
MONITOR_SCHEMA_PATH = str(ZEPHYR_BASE / "scripts" / "schemas" / "monitor-schema.yaml")

KEYBOARD_COALESCE_WINDOW = 0.030
KEYBOARD_POLL_INTERVAL = 0.001
KEYBOARD_READ_BUFFER_SIZE = 4096
THREAD_JOIN_TIMEOUT = 1.0
OUTPUT_QUEUE_TIMEOUT = 0.1
IDLE_SLEEP_INTERVAL = 0.02
RECONNECT_SLEEP_INTERVAL = 0.2


def get_parser_defaults():
    '''Get default values for all parser arguments.'''
    return {
        "baud": DEFAULT_BAUD_RATE,
        "rtscts": DEFAULT_RTSCTS,
        "read_mode": DEFAULT_READ_MODE,
        "idle_flush_ms": DEFAULT_IDLE_FLUSH_MS,
        "timestamp": DEFAULT_TIMESTAMP,
        "reset": VALID_RESET_MODES[0],
        "reset_on_connect": False,
        "reset_on_reconnect": False,
        "reset_delay_ms": DEFAULT_RESET_DELAY_MS,
        "reset_hold_ms": DEFAULT_RESET_HOLD_MS,
        "line_map": DEFAULT_LINE_MAP,
        "invert_reset": DEFAULT_INVERT_RESET,
        "invert_boot": DEFAULT_INVERT_BOOT,
        "idle_lines": DEFAULT_IDLE_LINES,
    }


def _isatty(stream):
    try:
        return stream.isatty()
    except (AttributeError, OSError):
        return False


def _find_default_port():
    '''Return first available serial port, or None.'''
    ports = list(list_ports.comports())
    if not ports:
        return None
    preferred = [p for p in ports if any(x in p.device for x in ("USB", "ACM", "tty."))]
    return preferred[0].device if preferred else ports[0].device


def _stdin_read_chunk_nonblocking(maxlen=4096):
    '''Return bytes from stdin if available, else None.'''
    if not _isatty(sys.stdin):
        return None
    if _ON_WINDOWS and msvcrt is not None:
        if msvcrt.kbhit():
            ch = msvcrt.getwch()
            return ch.encode("utf-8", errors="ignore") if ch else None
        return None
    if not _HAS_TERMIOS or select is None:
        return None
    try:
        fd = sys.stdin.fileno()
        r, _, _ = select.select([fd], [], [], 0.0)
        if not r:
            return None
        return os.read(fd, maxlen)
    except OSError:
        return None


class _StdinCBreakNoEcho:
    '''POSIX: cbreak + no-echo + O_NONBLOCK on stdin; restores on exit.'''

    def __init__(self):
        self._fd = None
        self._saved_term = None
        self._saved_flags = None

    def __enter__(self):
        if _HAS_TERMIOS and _isatty(sys.stdin):
            try:
                self._fd = sys.stdin.fileno()
                self._saved_term = termios.tcgetattr(self._fd)
                tty.setcbreak(self._fd)
                attrs = termios.tcgetattr(self._fd)
                attrs[3] = attrs[3] & ~termios.ECHO
                termios.tcsetattr(self._fd, termios.TCSANOW, attrs)
                if _HAS_FCNTL:
                    self._saved_flags = fcntl.fcntl(self._fd, fcntl.F_GETFL)
                    new_flags = self._saved_flags | os.O_NONBLOCK
                    fcntl.fcntl(self._fd, fcntl.F_SETFL, new_flags)
            except (OSError, termios.error):
                self._fd = None
                self._saved_term = None
                self._saved_flags = None
        return self

    def __exit__(self, exc_type, exc, tb):
        if self._fd is not None and _HAS_FCNTL and self._saved_flags is not None:
            with suppress(OSError):
                fcntl.fcntl(self._fd, fcntl.F_SETFL, self._saved_flags)
        if self._fd is not None and self._saved_term is not None:
            with suppress(OSError, termios.error):
                termios.tcsetattr(self._fd, termios.TCSANOW, self._saved_term)


class ResetController:
    '''Generic reset sequences using RTS/DTR.'''

    def __init__(
        self,
        ser_ref,
        map_str=DEFAULT_LINE_MAP,
        invert_reset=DEFAULT_INVERT_RESET,
        invert_boot=DEFAULT_INVERT_BOOT,
        settle_ms=DEFAULT_RESET_DELAY_MS,
        hold_ms=DEFAULT_RESET_HOLD_MS,
    ):
        self._ser_ref = ser_ref
        self.settle = settle_ms / 1000.0
        self.hold = hold_ms / 1000.0

        self.map_reset = "rts"
        self.map_boot = "dtr"
        valid_tokens = {"dtr-boot", "dtr-reset", "rts-boot", "rts-reset"}
        for p in map_str.split(","):
            p = p.strip()
            if not p:
                continue
            if p not in valid_tokens:
                log.wrn(f"monitor: ignoring unknown line-map token: '{p}'")
                continue
            if p == "rts-boot":
                self.map_boot = "rts"
            elif p == "dtr-boot":
                self.map_boot = "dtr"
            elif p == "dtr-reset":
                self.map_reset = "dtr"
            elif p == "rts-reset":
                self.map_reset = "rts"
        self.invert_reset = invert_reset
        self.invert_boot = invert_boot

    def _set_physical(self, mapped: str, inverted: bool, asserted: bool):
        ser = self._ser_ref.get()
        if ser is None:
            return
        physical = (not asserted) if inverted else asserted
        if mapped == "rts":
            ser.rts = bool(physical)
        else:
            ser.dtr = bool(physical)

    def _set_reset(self, asserted: bool):
        self._set_physical(self.map_reset, self.invert_reset, asserted)

    def _set_boot(self, asserted: bool):
        self._set_physical(self.map_boot, self.invert_boot, asserted)

    def set_idle(self, mode: str):
        if mode == "released":
            self._set_boot(False)
            self._set_reset(False)
        elif mode == "boot-hold":
            self._set_boot(True)
            self._set_reset(False)
        elif mode == "reset-hold":
            self._set_reset(True)
            self._set_boot(False)

    def enter_bootloader(self):
        self._set_boot(True)
        time.sleep(self.settle)
        self._set_reset(True)
        time.sleep(self.hold)
        self._set_reset(False)
        time.sleep(self.settle)
        self._set_boot(False)

    def run_reset(self):
        self._set_boot(False)
        time.sleep(self.settle)
        self._set_reset(True)
        time.sleep(self.hold)
        self._set_reset(False)

    def pulse_dtr(self):
        ser = self._ser_ref.get()
        if ser:
            ser.dtr = True
            time.sleep(self.hold)
            ser.dtr = False

    def pulse_rts(self):
        ser = self._ser_ref.get()
        if ser:
            ser.rts = True
            time.sleep(self.hold)
            ser.rts = False


class SerRef:
    def __init__(self, ser=None):
        self._ser = ser
        self._lock = threading.RLock()

    def get(self):
        with self._lock:
            return self._ser

    def set(self, ser):
        with self._lock:
            self._ser = ser


class SerialWorker(threading.Thread):
    '''Reads from serial, splits by LF/CR, flushes partial lines on idle.'''

    def __init__(
        self,
        ser_ref,
        out_q,
        stop_evt,
        open_params,
        reconnect=True,
        reconnect_interval=DEFAULT_RECONNECT_INTERVAL,
        reset_callback=None,
    ):
        super().__init__(daemon=True)
        self.ser_ref = ser_ref
        self.out_q = out_q
        self.stop_evt = stop_evt
        self.open_params = open_params
        self.reconnect = reconnect
        self.reconnect_interval = reconnect_interval
        self.reset_callback = reset_callback
        self._last_err = None
        self._buf = bytearray()
        self._last_rx = 0.0

    def _emit(self, text: str, had_delim: bool):
        '''Emit a data frame to the output queue.'''
        self.out_q.put(f"[[data:{1 if had_delim else 0}]]{text}")

    def _emit_monitor_message(self, message: str):
        '''Emit a monitor control message.'''
        self.out_q.put(f"[[monitor: {message}]]")

    def _emit_reset_trigger(self):
        '''Emit a reset trigger frame.'''
        self.out_q.put("[[reset:reconnect]]")

    def run(self):
        '''Main worker loop: reconnect, read, and process serial data.'''
        while not self.stop_evt.is_set():
            ser = self.ser_ref.get()

            if ser is None:
                if not self._handle_disconnected_state():
                    break
                continue

            if not self._process_serial_data(ser):
                continue

    def _handle_disconnected_state(self):
        '''Handle disconnected serial state. Returns False if should stop.'''
        if not self.reconnect:
            self._emit_monitor_message("port lost and reconnect disabled")
            return False

        if self.stop_evt.wait(self.reconnect_interval):
            return False

        self._attempt_reconnect()
        return True

    def _attempt_reconnect(self):
        '''Attempt to reconnect to the serial port.'''
        port = self.open_params["port"]

        try:
            ser = self._open_serial_port(port)
            self._handle_successful_reconnect(ser, port)
        except (OSError, serial.SerialException) as e:
            self._handle_reconnect_failure(e)

    def _open_serial_port(self, port):
        '''Open serial port with controlled DTR/RTS state.'''
        ser = serial.Serial()
        ser.port = port
        ser.baudrate = self.open_params["baud"]
        ser.rtscts = self.open_params["rtscts"]
        ser.timeout = DEFAULT_SERIAL_TIMEOUT
        ser.dtr = False
        ser.rts = False
        ser.open()
        ser.rts = True
        ser.dtr = True
        return ser

    def _handle_successful_reconnect(self, ser, port):
        '''Handle successful reconnection.'''
        self.ser_ref.set(ser)
        self._emit_monitor_message(f"reconnected to {port}")

        if self.reset_callback and self.open_params.get("reset_on_reconnect"):
            self._emit_reset_trigger()

        self._buf.clear()
        self._last_rx = 0.0

    def _handle_reconnect_failure(self, error):
        '''Handle reconnection failure.'''
        msg = str(error)
        if msg != self._last_err:
            self._last_err = msg
            self._emit_monitor_message(f"waiting for port ({msg})")

    def _process_serial_data(self, ser):
        '''Process data from serial port. Returns False on error.'''
        try:
            b = ser.read(ser.in_waiting or 1)
            now = time.time()

            if b:
                self._handle_received_data(b, now)
            else:
                self._handle_idle_state(now)

            return True
        except (OSError, serial.SerialException) as e:
            self._handle_serial_error(ser, e)
            return False

    def _handle_received_data(self, data, timestamp):
        '''Process received serial data.'''
        self._buf.extend(data)
        self._last_rx = timestamp
        self._process_buffer_lines()

    def _handle_idle_state(self, timestamp):
        '''Handle idle state when no data received.'''
        if not self._buf:
            time.sleep(IDLE_SLEEP_INTERVAL)
            return

        mode = self.open_params.get("read_mode", DEFAULT_READ_MODE)

        if mode == "raw":
            self._flush_buffer_raw()
        elif mode == "auto":
            self._flush_buffer_auto(timestamp)
        time.sleep(IDLE_SLEEP_INTERVAL)

    def _flush_buffer_raw(self):
        '''Flush buffer in raw mode.'''
        txt = bytes(self._buf).decode(errors="replace")
        self._buf.clear()
        self._emit(txt, had_delim=False)

    def _flush_buffer_auto(self, timestamp):
        '''Flush buffer in auto mode if idle threshold exceeded.'''
        idle_ms = self.open_params.get("idle_flush_ms", DEFAULT_IDLE_FLUSH_MS)
        threshold = max(0.0, idle_ms / 1000.0)

        if self._last_rx and (timestamp - self._last_rx) >= threshold:
            txt = bytes(self._buf).decode(errors="replace")
            self._buf.clear()
            self._emit(txt, had_delim=False)

    def _process_buffer_lines(self):
        '''Process complete lines in the buffer.'''
        while True:
            split_info = self._find_next_delimiter()
            if split_info is None:
                break

            self._extract_and_emit_line(split_info)

    def _find_next_delimiter(self):
        '''Find the next line delimiter. Returns (position, delimiter) or None.'''
        i_nl = self._buf.find(b"\n")
        i_cr = self._buf.find(b"\r")

        if i_nl == -1 and i_cr == -1:
            return None

        if i_nl != -1 and i_cr != -1:
            if i_nl < i_cr:
                return (i_nl, b"\n")
            else:
                return (i_cr, b"\r")

        if i_nl != -1:
            return (i_nl, b"\n")

        return (i_cr, b"\r")

    def _extract_and_emit_line(self, split_info):
        '''Extract a line from buffer and emit it.'''
        split_at, delim = split_info
        line = bytes(self._buf[:split_at]).decode(errors="replace")

        paired = self._has_paired_delimiter(split_at)
        if paired:
            delimiter_tag = "LF"
            self._buf = self._buf[split_at + 2 :]
        else:
            delimiter_tag = "LF" if delim == b"\n" else "CR"
            self._buf = self._buf[split_at + 1 :]
        self._emit(f"{delimiter_tag}::{line}", had_delim=True)

    def _has_paired_delimiter(self, position):
        '''Check if delimiter at position is part of a CRLF/LFCR pair.'''
        if position + 1 >= len(self._buf):
            return False

        pair = self._buf[position : position + 2]
        return pair in (b"\r\n", b"\n\r")

    def _handle_serial_error(self, ser, error):
        '''Handle serial port error.'''
        with suppress(OSError):
            ser.close()

        self.ser_ref.set(None)
        self._emit_monitor_message(f"serial error: {error}")
        self.stop_evt.wait(self.reconnect_interval)


_YAML_KEY_MAP = {
    ("serial", "baud"): "baud",
    ("serial", "rtscts"): "rtscts",
    ("serial", "read_mode"): "read_mode",
    ("serial", "idle_flush_ms"): "idle_flush_ms",
    ("reset", "mode"): "reset",
    ("reset", "on_connect"): "reset_on_connect",
    ("reset", "on_reconnect"): "reset_on_reconnect",
    ("reset", "delay_ms"): "reset_delay_ms",
    ("reset", "hold_ms"): "reset_hold_ms",
    ("reset", "line_map"): "line_map",
    ("reset", "invert_reset"): "invert_reset",
    ("reset", "invert_boot"): "invert_boot",
    ("reset", "idle_lines"): "idle_lines",
    ("display", "timestamp"): "timestamp",
}


def _load_monitor_schema():
    '''Load the monitor YAML schema for validation.'''
    with open(MONITOR_SCHEMA_PATH) as f:
        return yaml.safe_load(f)


def _load_monitor_yaml(path):
    '''Load a monitor.yaml file and flatten to argparse keys.

    Raises CommandError on malformed YAML or I/O errors.
    '''
    with open(path) as f:
        data = yaml.safe_load(f)

    if not isinstance(data, dict):
        raise CommandError(f"{path}: expected a YAML mapping")

    schema = _load_monitor_schema()
    try:
        jsonschema.validate(data, schema)
    except jsonschema.ValidationError as e:
        raise CommandError(f"{path}: {e.message}") from e

    flat = {}
    for (section, key), dest in _YAML_KEY_MAP.items():
        sec = data.get(section)
        if isinstance(sec, dict) and key in sec:
            flat[dest] = sec[key]

    return flat


def _find_board_monitor_yaml(build_dir):
    '''Find monitor.yaml in the board directory, or None.'''
    try:
        cache = CMakeCache.from_build_dir(build_dir)
        board_dir = cache.get("BOARD_DIR")
        if not board_dir:
            return None
        candidate = os.path.join(board_dir, MONITOR_YAML)
        if os.path.isfile(candidate):
            return candidate
    except (KeyError, OSError):
        pass
    return None


def _find_project_monitor_yaml(build_dir):
    '''Find .monitor.yaml or monitor.yaml in project directory, or None.'''
    try:
        cache = CMakeCache.from_build_dir(build_dir)
        app_dir = cache.get("APPLICATION_SOURCE_DIR")
        if app_dir:
            for name in (MONITOR_YAML_HIDDEN, MONITOR_YAML):
                candidate = os.path.join(app_dir, name)
                if os.path.isfile(candidate):
                    return candidate
    except (KeyError, OSError):
        pass

    cwd = os.getcwd()
    for name in (".monitor.yaml", "monitor.yaml"):
        candidate = os.path.join(cwd, name)
        if os.path.isfile(candidate):
            return candidate

    return None


def _config_overrides(args, flat_config, parser_defaults):
    '''Return list of (key, value) pairs from config where args still has defaults.'''
    overrides = []
    for key, val in flat_config.items():
        if key not in parser_defaults:
            continue
        if getattr(args, key, None) == parser_defaults[key]:
            overrides.append((key, val))
    return overrides


class Monitor(WestCommand):
    '''Serial monitor with reset control and auto-reconnect.'''

    def __init__(self):
        super().__init__(
            "monitor",
            "serial monitor with reset control and auto-reconnect",
            "Zephyr-friendly serial monitor with immediate key send.",
        )
        self._stop_evt = None
        self._ser_ref = None
        self._reset_ctl = None
        self._prev_sigint = None

    def do_add_parser(self, add_parser):
        '''Configure command-line argument parser.'''
        p = add_parser.add_parser(self.name, help=self.help, description=self.description)

        self._add_serial_args(p)
        self._add_reset_args(p)
        self._add_output_args(p)
        self._add_config_args(p)
        self._add_exit_args(p)

        return p

    def _add_serial_args(self, parser):
        '''Add serial port and behavior arguments.'''
        parser.add_argument("-p", "--port", help="Serial port (auto-detect if missing)")
        parser.add_argument("-b", "--baud", type=int, default=DEFAULT_BAUD_RATE)
        parser.add_argument(
            "--rtscts", action=argparse.BooleanOptionalAction, default=False,
            help="Enable hardware flow control",
        )
        parser.add_argument(
            "--read-mode",
            choices=VALID_READ_MODES,
            default=DEFAULT_READ_MODE,
            help="auto: newline + idle flush; line: newline only; raw: unbuffered",
        )
        parser.add_argument(
            "--idle-flush-ms",
            type=int,
            default=DEFAULT_IDLE_FLUSH_MS,
            help="idle timeout to flush partial line (auto mode)",
        )

    def _add_reset_args(self, parser):
        '''Add reset and control line arguments.'''
        parser.add_argument(
            "--reset",
            choices=VALID_RESET_MODES,
            default=VALID_RESET_MODES[0],
            help="Reset sequence type",
        )
        parser.add_argument(
            "--reset-on-connect", action=argparse.BooleanOptionalAction, default=False,
            help="Perform reset after initial connection",
        )
        parser.add_argument(
            "--reset-on-reconnect", action=argparse.BooleanOptionalAction, default=False,
            help="Perform reset after reconnections",
        )
        parser.add_argument(
            "--reset-delay-ms",
            type=int,
            default=DEFAULT_RESET_DELAY_MS,
            help="Delay between line toggles (ms)",
        )
        parser.add_argument(
            "--reset-hold-ms",
            type=int,
            default=DEFAULT_RESET_HOLD_MS,
            help="Hold time for asserted lines (ms)",
        )
        parser.add_argument(
            "--line-map",
            default=DEFAULT_LINE_MAP,
            help="Line mapping: dtr-boot|rts-boot, dtr-reset|rts-reset",
        )

        parser.add_argument(
            "--invert-reset", action=argparse.BooleanOptionalAction,
            default=DEFAULT_INVERT_RESET, help="Invert reset line polarity",
        )
        parser.add_argument(
            "--invert-boot", action=argparse.BooleanOptionalAction,
            default=DEFAULT_INVERT_BOOT, help="Invert boot line polarity",
        )

        parser.add_argument(
            "--idle-lines",
            choices=VALID_IDLE_MODES,
            default=DEFAULT_IDLE_LINES,
            help="Line state when idle",
        )
        parser.add_argument(
            "--reset-key",
            default=DEFAULT_RESET_KEY,
            help="Ctrl-<key> to trigger reset (default: r)",
        )

    def _add_output_args(self, parser):
        '''Add output formatting arguments.'''
        parser.add_argument(
            "-t", "--timestamp", action=argparse.BooleanOptionalAction, default=False,
            help="Prepend timestamps",
        )
        parser.add_argument("-l", "--logfile", help="Log output to file")

    def _add_config_args(self, parser):
        '''Add config file arguments.'''
        parser.add_argument(
            "--config",
            default=None,
            help="Path to monitor.yaml config file",
        )

    def _add_exit_args(self, parser):
        '''Add exit key arguments.'''
        parser.add_argument(
            "--exit-char", default=DEFAULT_EXIT_CHAR, help="Ctrl-<char> to exit (default: ])"
        )

    def do_run(self, args, unknown):
        '''Main entry point - orchestrates the monitor workflow.'''
        build_dir = self._resolve_build_dir()
        self._apply_monitor_config(args, build_dir)

        port = self._resolve_port(args)

        self._ser_ref, self._reset_ctl = self._setup_serial_connection(args, port)
        self._stop_evt = threading.Event()
        out_q = queue.Queue(maxsize=DEFAULT_QUEUE_SIZE)

        rx = self._start_serial_worker(out_q, args, port)

        self._run_main_loop(args, out_q, rx)

    def _load_and_apply_yaml(self, path, label, args, defaults):
        '''Load a YAML config file and apply it to args.'''
        cfg = _load_monitor_yaml(path)
        if not cfg:
            return
        overrides = _config_overrides(args, cfg, defaults)
        if overrides:
            for k, v in overrides:
                setattr(args, k, v)
            self.dbg(f"monitor: loaded {label}: {path}")
            for k, v in overrides:
                self.dbg(f"  {k} = {v}")

    def _apply_monitor_config(self, args, build_dir):
        '''Apply layered monitor configuration.

        Priority (lowest to highest):
          1. Code defaults (already in argparse)
          2. Board monitor.yaml (from BOARD_DIR)
          3. Project .monitor.yaml / monitor.yaml
          4. Explicit --config file
          5. CLI arguments (already in argparse)
        '''
        defaults = get_parser_defaults()

        board_yaml = _find_board_monitor_yaml(build_dir)
        if board_yaml:
            self._load_and_apply_yaml(board_yaml, "board config", args, defaults)

        proj_yaml = _find_project_monitor_yaml(build_dir)
        if proj_yaml:
            self._load_and_apply_yaml(proj_yaml, "project config", args, defaults)

        if args.config:
            if not os.path.isfile(args.config):
                raise CommandError(f"config file not found: {args.config}")
            self._load_and_apply_yaml(args.config, "config", args, defaults)

    def _resolve_port(self, args):
        '''Resolve the serial port to use.'''
        port = args.port or _find_default_port()
        if not port:
            raise CommandError("No serial port found; use --port")
        return port

    def _resolve_build_dir(self):
        '''Resolve the build directory using Zephyr's finder if available.'''
        if find_build_dir is None:
            return DEFAULT_BUILD_DIR

        try:
            build_dir = find_build_dir(None)
            if build_dir and is_zephyr_build(build_dir):
                self.dbg(f"monitor: found build directory: {build_dir}")
                return build_dir
        except (ValueError, OSError) as e:
            self.dbg(f"monitor: build directory lookup failed: {e}")

        return DEFAULT_BUILD_DIR

    def _setup_serial_connection(self, args, port):
        '''Establish initial serial connection and configure reset controller.'''
        ser = self._open_serial_port(port, args)
        ser_ref = SerRef(ser)

        reset_ctl = ResetController(
            ser_ref,
            map_str=args.line_map,
            invert_reset=args.invert_reset,
            invert_boot=args.invert_boot,
            settle_ms=args.reset_delay_ms,
            hold_ms=args.reset_hold_ms,
        )

        if ser is not None:
            self._apply_initial_reset(ser, args, reset_ctl)
        else:
            self.inf(f"monitor: will retry connection to {port}...")

        return ser_ref, reset_ctl

    def _open_serial_port(self, port, args):
        '''Open serial port with controlled DTR/RTS state.'''
        try:
            ser = serial.Serial()
            ser.port = port
            ser.baudrate = args.baud
            ser.rtscts = args.rtscts
            ser.timeout = DEFAULT_SERIAL_TIMEOUT
            ser.dtr = False
            ser.rts = False
            ser.open()
            ser.rts = True
            ser.dtr = True
            self.inf(f"monitor: connected to {port} @ {args.baud}")
            return ser
        except (OSError, serial.SerialException):
            self.dbg(f"monitor: {port} not available, waiting for connection")
            return None

    def _apply_initial_reset(self, ser, args, reset_ctl):
        '''Apply initial reset and idle line policy.'''
        if args.reset_on_connect and args.reset != VALID_RESET_MODES[0]:
            self._do_reset(reset_ctl, args.reset)

        reset_ctl.set_idle(args.idle_lines)

    def _create_reset_callback(self, args):
        '''Create reset callback for serial worker.'''

        def reset_callback():
            if args.reset != VALID_RESET_MODES[0]:
                self._do_reset(self._reset_ctl, args.reset)
                self._reset_ctl.set_idle(args.idle_lines)

        return reset_callback

    def _start_serial_worker(self, out_q, args, port):
        '''Start the serial worker thread with reconnection support.'''
        reset_callback = self._create_reset_callback(args)

        rx = SerialWorker(
            self._ser_ref,
            out_q,
            self._stop_evt,
            {
                "port": port,
                "baud": args.baud,
                "rtscts": args.rtscts,
                "read_mode": args.read_mode,
                "idle_flush_ms": args.idle_flush_ms,
                "reset_on_reconnect": args.reset_on_reconnect,
            },
            reconnect=True,
            reconnect_interval=DEFAULT_RECONNECT_INTERVAL,
            reset_callback=reset_callback,
        )
        rx.start()
        return rx

    def _do_reset(self, reset_ctl, mode):
        '''Execute a reset sequence.'''
        self.inf(f"monitor: reset sequence -> {mode}")
        try:
            reset_actions = {
                "run": reset_ctl.run_reset,
                "boot": reset_ctl.enter_bootloader,
                "dtr-pulse": reset_ctl.pulse_dtr,
                "rts-pulse": reset_ctl.pulse_rts,
            }
            action = reset_actions.get(mode)
            if action:
                action()
        except (OSError, serial.SerialException) as e:
            self.wrn(f"monitor: reset failed: {e}")

    def _run_main_loop(self, args, out_q, rx):
        '''Run the main monitoring loop with keyboard input and output processing.'''
        exit_ord = ord(args.exit_char) & 0x1F
        reset_ord = ord(args.reset_key) & 0x1F

        log_path = args.logfile
        if log_path:
            self.inf(f"logging to: {log_path}")

        kb = None
        try:
            with open(log_path, "ab", buffering=0) if log_path else nullcontext() as log_fp:
                self._setup_signal_handlers()
                kb = self._start_keyboard_worker(args, exit_ord, reset_ord)
                self._process_output_loop(out_q, args, log_fp)
        except KeyboardInterrupt:
            self._stop_evt.set()
        finally:
            self._cleanup(rx, kb)

    def _setup_signal_handlers(self):
        '''Setup SIGINT handler for graceful shutdown.'''

        def _sigint(signum, frame):
            self._stop_evt.set()

        with suppress(OSError, ValueError):
            self._prev_sigint = signal.signal(signal.SIGINT, _sigint)

    def _start_keyboard_worker(self, args, exit_ord, reset_ord):
        '''Start keyboard input worker thread if stdin is a TTY.'''
        if not _isatty(sys.stdin):
            return None

        def stdin_worker():
            try:
                self._keyboard_input_loop(args, exit_ord, reset_ord)
            except OSError:
                self._stop_evt.set()

        kb = threading.Thread(target=stdin_worker, daemon=True)
        kb.start()
        return kb

    def _keyboard_input_loop(self, args, exit_ord, reset_ord):
        '''Process keyboard input with coalescing.'''
        with _StdinCBreakNoEcho():
            window_s = KEYBOARD_COALESCE_WINDOW

            while not self._stop_evt.is_set():
                chunk = self._read_keyboard_input(window_s)
                if chunk is None:
                    continue

                if self._handle_control_keys(chunk, exit_ord, reset_ord, args):
                    continue

                self._send_to_serial(chunk)

    def _read_keyboard_input(self, window_s):
        '''Read and coalesce keyboard input.'''
        first = _stdin_read_chunk_nonblocking()
        if first is None:
            time.sleep(KEYBOARD_POLL_INTERVAL)
            return None

        chunk = bytearray(first)

        if not _ON_WINDOWS and select is not None:
            chunk = self._coalesce_input(chunk, window_s)

        return chunk

    def _coalesce_input(self, chunk, window_s):
        '''Coalesce rapid keyboard input within time window.'''
        deadline = time.time() + window_s
        fd = sys.stdin.fileno()

        while time.time() < deadline:
            try:
                r, _, _ = select.select([fd], [], [], KEYBOARD_POLL_INTERVAL)
                if not r:
                    break
                more = os.read(fd, KEYBOARD_READ_BUFFER_SIZE)
                if not more:
                    break
                chunk.extend(more)
            except OSError:
                break

        return chunk

    def _handle_control_keys(self, chunk, exit_ord, reset_ord, args):
        '''Handle control key commands. Returns True if control key was handled.'''
        if len(chunk) != 1:
            return False

        c = chunk[0]

        if c in (exit_ord, 0x03):
            self._stop_evt.set()
            return True

        if c == reset_ord and args.reset != VALID_RESET_MODES[0]:
            self._do_reset(self._reset_ctl, args.reset)
            self._reset_ctl.set_idle(args.idle_lines)

            return True

        return False

    def _send_to_serial(self, chunk):
        '''Send data to serial port with safety check.'''
        s = self._ser_ref.get()
        if s is not None:
            with suppress(OSError, serial.SerialException):
                s.write(chunk)

    def _process_output_loop(self, out_q, args, log_fp):
        '''Main output processing loop.'''
        while not self._stop_evt.is_set():
            frame = self._get_output_frame(out_q)
            if frame is None:
                continue

            if self._handle_special_frames(frame, args):
                continue

            self._process_data_frame(frame, args, log_fp)

    def _get_output_frame(self, out_q):
        '''Get next output frame from queue.'''
        try:
            return out_q.get(timeout=OUTPUT_QUEUE_TIMEOUT)
        except queue.Empty:
            return None

    def _handle_special_frames(self, frame, args):
        '''Handle special control frames. Returns True if frame was handled.'''
        if frame.startswith("[[monitor:"):
            print(frame, flush=True)
            return True

        if frame.startswith("[[reset:"):
            if args.reset != VALID_RESET_MODES[0]:
                self._do_reset(self._reset_ctl, args.reset)
                self._reset_ctl.set_idle(args.idle_lines)

            return True

        return False

    def _process_data_frame(self, frame, args, log_fp):
        '''Process and output a data frame.'''
        nl_tag, payload, delim = self._parse_data_frame(frame)

        raw = self._add_timestamp(args, payload, delim)
        self._print_output(raw, delim, nl_tag, log_fp, payload)

    def _parse_data_frame(self, frame):
        '''Parse data frame into components.'''
        if frame.startswith("[[data:"):
            nl_tag = frame[7:8]
            payload = frame[10:]
        else:
            nl_tag = "1"
            payload = frame

        # Extract delimiter type
        delim = None
        if payload.startswith("LF::"):
            delim, payload = "LF", payload[4:]
        elif payload.startswith("CR::"):
            delim, payload = "CR", payload[4:]

        return nl_tag, payload, delim

    def _add_timestamp(self, args, raw, delim):
        '''Add timestamp to output if enabled.'''
        if args.timestamp and delim != "CR":
            ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            return f"[{ts}] {raw}"
        return raw

    def _print_output(self, raw, delim, nl_tag, log_fp, payload):
        '''Print output to console and log file.'''
        if delim == "CR":
            self._print_carriage_return_output(raw, log_fp)
        else:
            self._print_normal_output(raw, nl_tag, log_fp, payload)

    def _print_carriage_return_output(self, raw, log_fp):
        '''Print carriage return output.'''
        print("\r" + raw, end="", flush=True)
        if log_fp:
            with suppress(OSError):
                log_fp.write(raw.encode("utf-8", errors="replace"))

    def _print_normal_output(self, raw, nl_tag, log_fp, payload):
        '''Print normal output with optional newline.'''
        endch = "\n" if nl_tag == "1" else ""
        print(raw, end=endch, flush=True)
        if log_fp:
            with suppress(OSError):
                log_fp.write((payload + ("\n" if endch else "")).encode("utf-8", errors="replace"))

    def _cleanup(self, rx, kb):
        '''Cleanup resources on shutdown.'''
        self._stop_evt.set()

        if self._prev_sigint is not None:
            with suppress(OSError, ValueError):
                signal.signal(signal.SIGINT, self._prev_sigint)

        s = self._ser_ref.get()
        if s:
            with suppress(OSError):
                s.close()

        rx.join(timeout=THREAD_JOIN_TIMEOUT)

        if kb:
            kb.join(timeout=THREAD_JOIN_TIMEOUT)

        self.inf("monitor: disconnected")
