# Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0

import os
import queue
import re
import shutil
import signal
import subprocess
import sys
import threading
import time
from contextlib import nullcontext, suppress
from datetime import datetime
from pathlib import Path

from build_helpers import find_build_dir, is_zephyr_build
from west import log
from west.commands import CommandError, WestCommand
from zcmake import CMakeCache

try:
    import serial
    from serial.tools import list_ports
except Exception:
    serial = None
    list_ports = None

try:
    import colorama
except Exception:
    colorama = None

# POSIX terminal + nonblocking stdin support
try:
    import termios
    import tty

    _HAS_TERMIOS = os.name == "posix"
except Exception:
    _HAS_TERMIOS = False

try:
    import fcntl

    _HAS_FCNTL = True
except Exception:
    _HAS_FCNTL = False

# Vendor profiles (pure data; safe if missing)
VENDOR_PROFILES = {}
VENDOR_PROFILES_ERR = None

try:
    from profiles import PROFILES as VENDOR_PROFILES
except Exception as e1:
    try:
        import importlib.util

        _profiles_path = Path(__file__).with_name("profiles.py")
        if _profiles_path.exists():
            spec = importlib.util.spec_from_file_location(
                "west_monitor_profiles", str(_profiles_path)
            )
            mod = importlib.util.module_from_spec(spec)
            assert spec is not None and spec.loader is not None
            spec.loader.exec_module(mod)
            obj = getattr(mod, "PROFILES", {})
            if isinstance(obj, dict):
                VENDOR_PROFILES = obj
            else:
                VENDOR_PROFILES_ERR = "profiles.py: PROFILES is not a dict"
        else:
            VENDOR_PROFILES_ERR = f"profiles.py not found at {_profiles_path}"
    except Exception as e2:
        err = f"relative import failed ({e1}); file load failed ({e2})"
        VENDOR_PROFILES_ERR = err

_ON_WINDOWS = os.name == "nt"
if _ON_WINDOWS:
    try:
        import msvcrt
    except Exception:
        msvcrt = None
else:
    try:
        import select
    except Exception:
        select = None


# ============================================================================
# CONSTANTS AND DEFAULTS
# ============================================================================

# Reset and Control Line Constants
DEFAULT_RESET_DELAY_MS = 50
DEFAULT_RESET_HOLD_MS = 120
DEFAULT_LINE_MAP = "dtr-boot,rts-reset"
DEFAULT_INVERT_RESET = True
DEFAULT_INVERT_BOOT = True
DEFAULT_IDLE_LINES = "released"
DEFAULT_RESET_KEY = "r"

# Valid modes
VALID_IDLE_MODES = ["released", "boot-hold", "reset-hold"]
VALID_RESET_MODES = ["none", "run", "boot", "dtr-pulse", "rts-pulse"]
VALID_READ_MODES = ["auto", "line", "raw"]

# Serial Port Constants
DEFAULT_BAUD_RATE = 115200
DEFAULT_RTSCTS = False
DEFAULT_READ_MODE = "auto"
DEFAULT_IDLE_FLUSH_MS = 60
DEFAULT_SERIAL_TIMEOUT = 0.05
DEFAULT_RECONNECT_INTERVAL = 0.5
DEFAULT_QUEUE_SIZE = 200

# Control Keys
DEFAULT_EXIT_CHAR = "]"

# Output Constants
DEFAULT_TIMESTAMP = False
DEFAULT_NO_COLOR = False

# Addr2line Decoder Constants
DEFAULT_ADDR2LINE_CACHE_LIMIT = 1000
DEFAULT_ADDR2LINE_TIMEOUT = 2.0
DEFAULT_DEMANGLE = True
DEFAULT_INLINE = False

# Vendor and Build
DEFAULT_VENDOR = "none"
DEFAULT_BUILD_DIR = "build"

# Timing Constants (seconds)
KEYBOARD_COALESCE_WINDOW = 0.030
KEYBOARD_POLL_INTERVAL = 0.001
KEYBOARD_READ_BUFFER_SIZE = 4096
THREAD_JOIN_TIMEOUT = 1.0
OUTPUT_QUEUE_TIMEOUT = 0.1
IDLE_SLEEP_INTERVAL = 0.02
RECONNECT_SLEEP_INTERVAL = 0.2


def get_parser_defaults():
    """Get default values for all parser arguments."""
    return {
        "baud": DEFAULT_BAUD_RATE,
        "rtscts": DEFAULT_RTSCTS,
        "read_mode": DEFAULT_READ_MODE,
        "idle_flush_ms": DEFAULT_IDLE_FLUSH_MS,
        "timestamp": DEFAULT_TIMESTAMP,
        "no_color": DEFAULT_NO_COLOR,
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


# ============================================================================

ADDR_RE = re.compile(r"0x[0-9a-fA-F]{8,16}")


# ----------------------------- helpers -----------------------------
def _isatty(stream):
    try:
        return stream.isatty()
    except Exception:
        return False


def _get_build_info_from_zephyr(build_dir):
    """
    Use Zephyr's CMakeCache to get ELF and addr2line.
    This is the preferred method as it properly handles all build configurations.
    """
    addr2line_cmd = None
    elf_path = None

    try:
        # Use CMakeCache for robust parsing
        cache = CMakeCache.from_build_dir(build_dir)

        # Get addr2line from CMAKE_ADDR2LINE or derive from objdump
        addr2line_cmd = cache.get('CMAKE_ADDR2LINE')
        if not addr2line_cmd:
            objdump = cache.get('CMAKE_OBJDUMP')
            if objdump:
                addr2line_cmd = objdump.replace('objdump', 'addr2line')
                if not os.path.exists(addr2line_cmd):
                    addr2line_cmd = None

        # Get ELF file - try multiple approaches
        elf_name = cache.get('BYPRODUCT_KERNEL_ELF_NAME', 'zephyr.elf')

        # Try common locations
        for subdir in ['zephyr', '']:
            candidate = (
                os.path.join(build_dir, subdir, elf_name)
                if subdir
                else os.path.join(build_dir, elf_name)
            )
            if os.path.exists(candidate):
                elf_path = candidate
                break

    except Exception as e:
        log.dbg(f"Failed to use Zephyr build helpers: {e}")

    return addr2line_cmd, elf_path


def _default_addr2line_cmd():
    for var in ("ADDR2LINE", "ZEPHYR_ADDR2LINE"):
        v = os.environ.get(var)
        if v:
            return v
    for cand in (
        "arm-none-eabi-addr2line",
        "riscv64-unknown-elf-addr2line",
        "xtensa-elf-addr2line",
        "llvm-addr2line",
        "addr2line",
    ):
        if shutil.which(cand):
            return cand
    return None


def _find_default_port():
    if list_ports is None:
        return None
    ports = list(list_ports.comports())
    if not ports:
        return None
    preferred = [p for p in ports if any(x in p.device for x in ("USB", "ACM", "tty."))]
    return preferred[0].device if preferred else ports[0].device


def _stdin_read_chunk_nonblocking(maxlen=4096):
    """Return bytes from stdin if available, else None. Non-blocking."""
    if not _isatty(sys.stdin):
        return None
    if _ON_WINDOWS and msvcrt is not None:
        if msvcrt.kbhit():
            ch = msvcrt.getwch()
            return ch.encode("utf-8", errors="ignore")[:1] if ch else None
        return None
    if not _HAS_TERMIOS or select is None:
        return None
    try:
        fd = sys.stdin.fileno()
        r, _, _ = select.select([fd], [], [], 0.0)
        if not r:
            return None
        return os.read(fd, maxlen)
    except Exception:
        return None


class _StdinCBreakNoEcho:
    """POSIX: cbreak + no-echo + O_NONBLOCK on stdin; restores on exit."""

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
            except Exception:
                self._fd = None
                self._saved_term = None
                self._saved_flags = None
        return self

    def __exit__(self, exc_type, exc, tb):
        if self._fd is not None and _HAS_FCNTL and self._saved_flags is not None:
            with suppress(Exception):
                fcntl.fcntl(self._fd, fcntl.F_SETFL, self._saved_flags)
        if self._fd is not None and self._saved_term is not None:
            with suppress(Exception):
                termios.tcsetattr(self._fd, termios.TCSADRAIN, self._saved_term)


# ----------------------------- reset controller -----------------------------
class ResetController:
    """Generic reset sequences using RTS/DTR."""

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
        for p in map_str.split(","):
            p = p.strip()
            if p == "rts-boot":
                self.map_boot = "rts"
            elif p == "dtr-reset":
                self.map_reset = "dtr"
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
        time.sleep(self.settle)

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
        self._set_reset(True)
        time.sleep(self.hold)
        self._set_reset(False)
        time.sleep(self.hold / 2)
        self._set_boot(False)

    def run_reset(self):
        self._set_boot(False)
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


# ------------------- addr2line -----------------------------
class Addr2LineDecoder:
    """Decodes memory addresses to source locations using addr2line."""

    def __init__(
        self,
        elf,
        cmd=None,
        inline=DEFAULT_INLINE,
        demangle=DEFAULT_DEMANGLE,
        cache_limit=DEFAULT_ADDR2LINE_CACHE_LIMIT,
    ):
        self.elf = elf
        self.cmd = cmd or _default_addr2line_cmd()
        self.inline = inline
        self.demangle = demangle
        self.cache = {}
        self.cache_limit = cache_limit
        self.enabled = self._check_enabled()

    def _check_enabled(self):
        """Check if decoder is properly configured and available."""
        if not self.cmd or not self.elf:
            return False
        return os.path.exists(self.elf)

    def decode_addrs(self, addrs):
        """Decode a list of addresses to source locations.

        Args:
            addrs: List of address strings to decode

        Returns:
            Dictionary mapping addresses to decoded strings
        """
        if not self.enabled or not addrs:
            return {}

        self._manage_cache_size()
        return self._decode_address_list(addrs)

    def _manage_cache_size(self):
        """Clear cache if it exceeds the size limit."""
        if len(self.cache) > self.cache_limit:
            self.cache.clear()

    def _decode_address_list(self, addrs):
        """Decode all addresses, using cache when available."""
        results = {}

        for addr in addrs:
            if addr in self.cache:
                results[addr] = self.cache[addr]
            else:
                decoded = self._decode_single_address(addr)
                self.cache[addr] = decoded
                results[addr] = decoded

        return results

    def _decode_single_address(self, addr):
        """Decode a single address using addr2line.

        Args:
            addr: Address string to decode

        Returns:
            Decoded string or error message
        """
        try:
            return self._run_addr2line(addr)
        except subprocess.TimeoutExpired:
            return "addr2line timeout"
        except Exception as e:
            return f"addr2line error: {e}"

    def _run_addr2line(self, addr):
        """Execute addr2line command for an address.

        Args:
            addr: Address string to decode

        Returns:
            Decoded string or "??" if decoding failed
        """
        args = self._build_command_args(addr)

        result = subprocess.run(
            args,
            check=False,
            capture_output=True,
            text=True,
            timeout=DEFAULT_ADDR2LINE_TIMEOUT,
        )

        return self._parse_addr2line_output(result)

    def _build_command_args(self, addr):
        """Build command-line arguments for addr2line.

        Args:
            addr: Address string to decode

        Returns:
            List of command arguments
        """
        args = [self.cmd, "-e", self.elf, "-f"]
        demangle_flag = "-C" if self.demangle else "-s"
        args.extend([demangle_flag, addr])
        return args

    def _parse_addr2line_output(self, result):
        """Parse addr2line output into a readable string.

        Args:
            result: CompletedProcess object from subprocess.run

        Returns:
            Formatted decoded string or "??" if parsing failed
        """
        if result.returncode != 0 or not result.stdout.strip():
            return "??"

        lines = [ln.strip() for ln in result.stdout.splitlines() if ln.strip()]

        if not lines:
            return "??"

        # Take first two lines (function name and file:line)
        return " ".join(lines[:2])


# ----------------------------- serial glue -----------------------------
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
    """Reads from serial, splits by LF/CR, flushes partial lines on idle."""

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
        super().__init__(daemon=False)
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
        """Emit a data frame to the output queue."""
        self.out_q.put(f"[[data:{1 if had_delim else 0}]]{text}")

    def _emit_monitor_message(self, message: str):
        """Emit a monitor control message."""
        self.out_q.put(f"[[monitor: {message}]]")

    def _emit_reset_trigger(self):
        """Emit a reset trigger frame."""
        self.out_q.put("[[reset:reconnect]]")

    def run(self):
        """Main worker loop: reconnect, read, and process serial data."""
        while not self.stop_evt.is_set():
            ser = self.ser_ref.get()

            if ser is None:
                if not self._handle_disconnected_state():
                    break
                continue

            if not self._process_serial_data(ser):
                continue

    def _handle_disconnected_state(self):
        """Handle disconnected serial state. Returns False if should stop."""
        if not self.reconnect:
            time.sleep(RECONNECT_SLEEP_INTERVAL)
            return True

        if self.stop_evt.wait(self.reconnect_interval):
            return False

        self._attempt_reconnect()
        return True

    def _attempt_reconnect(self):
        """Attempt to reconnect to the serial port."""
        port = self.open_params["port"]

        try:
            ser = self._open_serial_port(port)
            self._handle_successful_reconnect(ser, port)
        except Exception as e:
            self._handle_reconnect_failure(e)

    def _open_serial_port(self, port):
        """Open and configure a serial port."""
        ser = serial.Serial(
            port=port,
            baudrate=self.open_params["baud"],
            rtscts=self.open_params["rtscts"],
            timeout=DEFAULT_SERIAL_TIMEOUT,
        )
        ser.dtr = False
        ser.rts = False
        return ser

    def _handle_successful_reconnect(self, ser, port):
        """Handle successful reconnection."""
        self.ser_ref.set(ser)
        self._emit_monitor_message(f"reconnected to {port}")

        if self.reset_callback and self.open_params.get("reset_on_reconnect"):
            self._emit_reset_trigger()

        self._buf.clear()
        self._last_rx = 0.0

    def _handle_reconnect_failure(self, error):
        """Handle reconnection failure."""
        msg = str(error)
        if msg != self._last_err:
            self._last_err = msg
            self._emit_monitor_message(f"waiting for port ({msg})")

    def _process_serial_data(self, ser):
        """Process data from serial port. Returns False on error."""
        try:
            b = ser.read(ser.in_waiting or 1)
            now = time.time()

            if b:
                self._handle_received_data(b, now)
            else:
                self._handle_idle_state(now)

            return True
        except Exception as e:
            self._handle_serial_error(ser, e)
            return False

    def _handle_received_data(self, data, timestamp):
        """Process received serial data."""
        self._buf.extend(data)
        self._last_rx = timestamp
        self._process_buffer_lines()

    def _handle_idle_state(self, timestamp):
        """Handle idle state when no data received."""
        if not self._buf:
            time.sleep(IDLE_SLEEP_INTERVAL)
            return

        mode = self.open_params.get("read_mode", DEFAULT_READ_MODE)

        if mode == "raw":
            self._flush_buffer_raw()
        elif mode == "auto":
            self._flush_buffer_auto(timestamp)
        # 'line' mode waits for delimiter

        time.sleep(IDLE_SLEEP_INTERVAL)

    def _flush_buffer_raw(self):
        """Flush buffer in raw mode."""
        txt = bytes(self._buf).decode(errors="replace")
        self._buf.clear()
        self._emit(txt, had_delim=False)

    def _flush_buffer_auto(self, timestamp):
        """Flush buffer in auto mode if idle threshold exceeded."""
        idle_ms = self.open_params.get("idle_flush_ms", DEFAULT_IDLE_FLUSH_MS)
        threshold = max(0.0, idle_ms / 1000.0)

        if self._last_rx and (timestamp - self._last_rx) >= threshold:
            txt = bytes(self._buf).decode(errors="replace")
            self._buf.clear()
            self._emit(txt, had_delim=False)

    def _process_buffer_lines(self):
        """Process complete lines in the buffer."""
        while True:
            split_info = self._find_next_delimiter()
            if split_info is None:
                break

            self._extract_and_emit_line(split_info)

    def _find_next_delimiter(self):
        """Find the next line delimiter. Returns (position, delimiter) or None."""
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
        """Extract a line from buffer and emit it."""
        split_at, delim = split_info
        line = bytes(self._buf[:split_at]).decode(errors="replace")

        # Check for paired CRLF/LFCR
        if self._has_paired_delimiter(split_at):
            self._buf = self._buf[split_at + 2 :]
            self._emit(f"LF::{line}", had_delim=True)
        else:
            self._buf = self._buf[split_at + 1 :]
            delimiter_tag = "LF" if delim == b"\n" else "CR"
            self._emit(f"{delimiter_tag}::{line}", had_delim=True)

    def _has_paired_delimiter(self, position):
        """Check if delimiter at position is part of a CRLF/LFCR pair."""
        if position + 1 >= len(self._buf):
            return False

        pair = self._buf[position : position + 2]
        return pair in (b"\r\n", b"\n\r")

    def _handle_serial_error(self, ser, error):
        """Handle serial port error."""
        with suppress(Exception):
            ser.close()

        self.ser_ref.set(None)
        self._emit_monitor_message(f"serial error: {error}")
        time.sleep(self.reconnect_interval)


# ----------------------------- vendor defaults -----------------------------
def _apply_vendor_defaults(args, vendor_name: str, profiles: dict, parser_defaults: dict):
    """Apply vendor profile where user left option at parser default."""
    profile = profiles.get(vendor_name)
    if not profile:
        return [], f"no profile named '{vendor_name}'"
    applied = []
    for key, val in profile.items():
        if key not in parser_defaults:
            continue
        if getattr(args, key, None) == parser_defaults[key]:
            setattr(args, key, val)
            applied.append((key, val))
    return applied, None


# ----------------------------- West command -----------------------------
class Monitor(WestCommand):
    """Serial monitor with addr2line decoding, reset control, and auto-reconnect."""

    def __init__(self):
        super().__init__(
            "monitor",
            "serial monitor with addr2line, resets, and auto-reconnect",
            "Zephyr-friendly serial monitor with immediate key send and addr2line decoding.",
        )
        self._stop_evt = None
        self._ser_ref = None
        self._reset_ctl = None

    def do_add_parser(self, add_parser):
        """Configure command-line argument parser."""
        p = add_parser.add_parser(self.name, help=self.help, description=self.description)

        self._add_serial_args(p)
        self._add_reset_args(p)
        self._add_output_args(p)
        self._add_decode_args(p)
        self._add_vendor_args(p)
        self._add_exit_args(p)

        return p

    def _add_serial_args(self, parser):
        """Add serial port and behavior arguments."""
        parser.add_argument("--port", help="Serial port (auto-detect if missing)")
        parser.add_argument("--baud", type=int, default=DEFAULT_BAUD_RATE)
        parser.add_argument("--rtscts", action="store_true", help="Enable hardware flow control")
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
        """Add reset and control line arguments."""
        parser.add_argument(
            "--reset",
            choices=VALID_RESET_MODES,
            default=VALID_RESET_MODES[0],
            help="Reset sequence type",
        )
        parser.add_argument(
            "--reset-on-connect", action="store_true", help="Perform reset after initial connection"
        )
        parser.add_argument(
            "--reset-on-reconnect", action="store_true", help="Perform reset after reconnections"
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

        inv = parser.add_mutually_exclusive_group()
        inv.add_argument("--invert-reset", dest="invert_reset", action="store_true")
        inv.add_argument("--no-invert-reset", dest="invert_reset", action="store_false")
        parser.set_defaults(invert_reset=DEFAULT_INVERT_RESET)

        invb = parser.add_mutually_exclusive_group()
        invb.add_argument("--invert-boot", dest="invert_boot", action="store_true")
        invb.add_argument("--no-invert-boot", dest="invert_boot", action="store_false")
        parser.set_defaults(invert_boot=DEFAULT_INVERT_BOOT)

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
        """Add output formatting arguments."""
        parser.add_argument("--timestamp", action="store_true", help="Prepend timestamps")
        parser.add_argument("--no-color", action="store_true", help="Disable colored output")
        parser.add_argument("--logfile", help="Log output to file")

    def _add_decode_args(self, parser):
        """Add addr2line decoding arguments."""
        parser.add_argument(
            "--decode",
            action="store_true",
            help="Enable address decoding using ELF and addr2line (auto-detected from build/)",
        )
        parser.add_argument("--elf", help="ELF file path (overrides auto-detection)")
        parser.add_argument("--addr2line", help="addr2line tool path (overrides auto-detection)")
        parser.add_argument(
            "--build-dir",
            help="Build directory for auto-detection (uses find_build_dir if not specified)",
        )
        parser.add_argument(
            "--no-demangle",
            dest="demangle",
            action="store_false",
            default=True,
            help="Disable C++ symbol demangling",
        )

    def _add_vendor_args(self, parser):
        """Add vendor profile arguments."""
        parser.add_argument(
            "--vendor",
            default=DEFAULT_VENDOR,
            help="Vendor profile name (e.g. 'espressif'), or 'none'",
        )

    def _add_exit_args(self, parser):
        """Add exit key arguments."""
        parser.add_argument(
            "--exit-char", default=DEFAULT_EXIT_CHAR, help="Ctrl-<char> to exit (default: ])"
        )

    def do_run(self, args, unknown):
        """Main entry point - orchestrates the monitor workflow."""
        self._validate_dependencies()

        use_color = self._setup_color_output(args)
        self._apply_vendor_profile(args)

        port = self._resolve_port(args)
        build_dir = self._resolve_build_dir(args)

        self._ser_ref, self._reset_ctl = self._setup_serial_connection(args, port)
        self._stop_evt = threading.Event()
        out_q = queue.Queue(maxsize=DEFAULT_QUEUE_SIZE)

        rx = self._start_serial_worker(out_q, args, port)
        decoder = self._setup_decoder(args, build_dir)

        self._run_main_loop(args, out_q, decoder, use_color, rx)

    def _validate_dependencies(self):
        """Validate required dependencies are available."""
        if serial is None:
            raise CommandError("pyserial required: pip install pyserial")

    def _setup_color_output(self, args):
        """Initialize color output if available and enabled."""
        use_color = (not args.no_color) and _isatty(sys.stdout) and colorama is not None
        if use_color:
            colorama.init()
        return use_color

    def _get_parser_defaults(self):
        """Get default values for parser arguments."""
        return get_parser_defaults()

    def _apply_vendor_profile(self, args):
        """Apply vendor-specific defaults if configured."""
        if args.vendor == DEFAULT_VENDOR:
            return

        if VENDOR_PROFILES_ERR:
            log.wrn("monitor: vendor profiles unavailable: " + VENDOR_PROFILES_ERR)
            return

        parser_defaults = self._get_parser_defaults()
        applied, reason = _apply_vendor_defaults(
            args, args.vendor, VENDOR_PROFILES, parser_defaults
        )

        if reason:
            log.wrn(f"monitor: vendor profile issue: {reason}")
        elif applied:
            log.inf(f"monitor: applied '{args.vendor}' vendor defaults:")
            for key, val in applied:
                log.inf(f"  {key} = {val}")

    def _resolve_port(self, args):
        """Resolve the serial port to use."""
        port = args.port or _find_default_port()
        if not port:
            raise CommandError("No serial port found; use --port")
        return port

    def _resolve_build_dir(self, args):
        """Resolve the build directory using Zephyr's finder if available."""
        if args.build_dir:
            return args.build_dir

        if find_build_dir is None:
            return DEFAULT_BUILD_DIR

        try:
            build_dir = find_build_dir(None)
            if build_dir and is_zephyr_build and is_zephyr_build(build_dir):
                log.dbg(f"monitor: found build directory: {build_dir}")
                return build_dir
        except Exception:
            pass

        return DEFAULT_BUILD_DIR

    def _setup_serial_connection(self, args, port):
        """Establish initial serial connection and configure reset controller."""
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

        self._apply_initial_reset(ser, args, reset_ctl)
        return ser_ref, reset_ctl

    def _open_serial_port(self, port, args):
        """Attempt to open the serial port."""
        try:
            ser = serial.Serial(
                port=port, baudrate=args.baud, rtscts=args.rtscts, timeout=DEFAULT_SERIAL_TIMEOUT
            )
            ser.dtr = False
            ser.rts = False
            log.inf(f"monitor: connected to {port} @ {args.baud}")
            return ser
        except Exception as e:
            log.wrn(f"monitor: failed to open {port}: {e}")
            log.inf(f"monitor: will retry connection to {port}...")
            return None

    def _apply_initial_reset(self, ser, args, reset_ctl):
        """Apply initial reset and idle line policy."""
        if not ser:
            return

        if args.reset_on_connect and args.reset != VALID_RESET_MODES[0]:
            self._do_reset(reset_ctl, args.reset)

        reset_ctl.set_idle(args.idle_lines)

    def _create_reset_callback(self, args):
        """Create reset callback for serial worker."""

        def reset_callback():
            if args.reset != VALID_RESET_MODES[0]:
                self._do_reset(self._reset_ctl, args.reset)
                self._reset_ctl.set_idle(args.idle_lines)

        return reset_callback

    def _start_serial_worker(self, out_q, args, port):
        """Start the serial worker thread with reconnection support."""
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

    def _setup_decoder(self, args, build_dir):
        """Setup addr2line decoder with auto-detection."""
        if not args.decode:
            return self._create_disabled_decoder()

        elf_path, addr2line_cmd = self._detect_build_info(args, build_dir)
        decoder = self._create_decoder(elf_path, addr2line_cmd, args)
        self._log_decoder_status(decoder, elf_path, addr2line_cmd)

        return decoder

    def _create_disabled_decoder(self):
        """Create a disabled decoder."""
        decoder = Addr2LineDecoder(None, None, inline=DEFAULT_INLINE)
        decoder.enabled = False
        return decoder

    def _create_decoder(self, elf_path, addr2line_cmd, args):
        """Create an addr2line decoder."""
        return Addr2LineDecoder(
            elf_path,
            addr2line_cmd,
            inline=DEFAULT_INLINE,
            demangle=getattr(args, "demangle", True),
        )

    def _detect_build_info(self, args, build_dir):
        """Auto-detect ELF and addr2line paths."""
        elf_path = args.elf
        addr2line_cmd = args.addr2line

        if elf_path and addr2line_cmd:
            return elf_path, addr2line_cmd

        try:
            detected_addr2line, detected_elf = _get_build_info_from_zephyr(build_dir)

            if not elf_path and detected_elf:
                elf_path = detected_elf
                log.inf(f"monitor: auto-detected ELF: {elf_path}")

            if not addr2line_cmd and detected_addr2line:
                addr2line_cmd = detected_addr2line
                log.inf(f"monitor: auto-detected addr2line: {addr2line_cmd}")
        except Exception as e:
            log.wrn(f"monitor: failed to auto-detect build info: {e}")

        return elf_path, addr2line_cmd

    def _log_decoder_status(self, decoder, elf_path, addr2line_cmd):
        """Log the decoder initialization status."""
        if decoder.enabled:
            log.inf("monitor: addr2line decoding enabled")
        elif elf_path or addr2line_cmd:
            log.wrn(
                f"monitor: addr2line unavailable (elf={bool(elf_path)}, cmd={bool(addr2line_cmd)})"
            )

    def _do_reset(self, reset_ctl, mode):
        """Execute a reset sequence."""
        log.inf(f"monitor: reset sequence -> {mode}")
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
        except Exception as e:
            log.wrn(f"monitor: reset failed: {e}")

    def _run_main_loop(self, args, out_q, decoder, use_color, rx):
        """Run the main monitoring loop with keyboard input and output processing."""
        exit_ord = ord(args.exit_char) & 0x1F
        reset_ord = ord(args.reset_key) & 0x1F

        log_path = args.logfile
        if log_path:
            log.inf(f"logging to: {log_path}")

        kb = None
        try:
            with open(log_path, "ab", buffering=0) if log_path else nullcontext() as log_fp:
                self._setup_signal_handlers()
                kb = self._start_keyboard_worker(args, exit_ord, reset_ord)
                self._process_output_loop(out_q, args, decoder, use_color, log_fp)
        except KeyboardInterrupt:
            self._stop_evt.set()
        finally:
            self._cleanup(rx, kb)

    def _setup_signal_handlers(self):
        """Setup SIGINT handler for graceful shutdown."""

        def _sigint(signum, frame):
            self._stop_evt.set()

        with suppress(Exception):
            signal.signal(signal.SIGINT, _sigint)

    def _start_keyboard_worker(self, args, exit_ord, reset_ord):
        """Start keyboard input worker thread if stdin is a TTY."""
        if not _isatty(sys.stdin):
            return None

        def stdin_worker():
            try:
                self._keyboard_input_loop(args, exit_ord, reset_ord)
            except Exception:
                self._stop_evt.set()

        kb = threading.Thread(target=stdin_worker, daemon=False)
        kb.start()
        return kb

    def _keyboard_input_loop(self, args, exit_ord, reset_ord):
        """Process keyboard input with coalescing."""
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
        """Read and coalesce keyboard input."""
        first = _stdin_read_chunk_nonblocking()
        if first is None:
            time.sleep(KEYBOARD_POLL_INTERVAL)
            return None

        chunk = bytearray(first)

        # Coalesce rapid input (POSIX only)
        if not _ON_WINDOWS and select is not None:
            chunk = self._coalesce_input(chunk, window_s)

        return chunk

    def _coalesce_input(self, chunk, window_s):
        """Coalesce rapid keyboard input within time window."""
        deadline = time.time() + window_s
        fd = sys.stdin.fileno()

        while time.time() < deadline:
            r, _, _ = select.select([fd], [], [], KEYBOARD_POLL_INTERVAL)
            if not r:
                break

            try:
                more = os.read(fd, KEYBOARD_READ_BUFFER_SIZE)
                if not more:
                    break
                chunk.extend(more)
            except Exception:
                break

        return chunk

    def _handle_control_keys(self, chunk, exit_ord, reset_ord, args):
        """Handle control key commands. Returns True if control key was handled."""
        if len(chunk) != 1:
            return False

        c = chunk[0]

        if c == exit_ord:
            self._stop_evt.set()
            return True

        if c == reset_ord and args.reset != VALID_RESET_MODES[0]:
            self._do_reset(self._reset_ctl, args.reset)
            self._reset_ctl.set_idle(args.idle_lines)

            return True

        return False

    def _send_to_serial(self, chunk):
        """Send data to serial port with safety check."""
        s = self._ser_ref.get()
        if s is not None:
            with suppress(Exception):
                s.write(chunk)

    def _process_output_loop(self, out_q, args, decoder, use_color, log_fp):
        """Main output processing loop."""
        while not self._stop_evt.is_set():
            frame = self._get_output_frame(out_q)
            if frame is None:
                continue

            if self._handle_special_frames(frame, args):
                continue

            self._process_data_frame(frame, args, decoder, use_color, log_fp)

    def _get_output_frame(self, out_q):
        """Get next output frame from queue."""
        try:
            return out_q.get(timeout=OUTPUT_QUEUE_TIMEOUT)
        except queue.Empty:
            return None

    def _handle_special_frames(self, frame, args):
        """Handle special control frames. Returns True if frame was handled."""
        if frame.startswith("[[monitor:"):
            print(frame, flush=True)
            return True

        if frame.startswith("[[reset:"):
            if args.reset != VALID_RESET_MODES[0]:
                self._do_reset(self._reset_ctl, args.reset)
                self._reset_ctl.set_idle(args.idle_lines)

            return True

        return False

    def _process_data_frame(self, frame, args, decoder, use_color, log_fp):
        """Process and output a data frame."""
        nl_tag, payload, delim = self._parse_data_frame(frame)

        # Try addr2line decoding
        if self._try_decode_output(args, decoder, payload, delim, use_color, log_fp):
            return

        # Normal output with timestamp
        raw = self._add_timestamp(args, payload, delim)
        self._print_output(raw, delim, nl_tag, log_fp, payload)

    def _parse_data_frame(self, frame):
        """Parse data frame into components."""
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

    def _try_decode_output(self, args, decoder, payload, delim, use_color, log_fp):
        """Try to decode addresses in output. Returns True if decoded."""
        if args.read_mode == "raw" or not decoder.enabled:
            return False

        addrs = list(dict.fromkeys(ADDR_RE.findall(payload)))
        if not addrs:
            return False

        mapping = decoder.decode_addrs(addrs)
        if not mapping:
            return False

        self._print_decoded_output(args, payload, delim, addrs, mapping, use_color, log_fp)
        return True

    def _print_decoded_output(self, args, payload, delim, addrs, mapping, use_color, log_fp):
        """Print output with decoded addresses."""
        ts_prefix = self._get_timestamp_prefix(args, delim)
        print(ts_prefix + payload, flush=True)

        for addr in addrs:
            dec = mapping.get(addr)
            if dec:
                self._print_decoded_address(addr, dec, use_color)

        if log_fp:
            with suppress(Exception):
                log_fp.write((payload + "\n").encode("utf-8", errors="replace"))

    def _get_timestamp_prefix(self, args, delim):
        """Get timestamp prefix if enabled."""
        if args.timestamp and delim != "CR":
            ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            return f"[{ts}] "
        return ""

    def _print_decoded_address(self, addr, decoded, use_color):
        """Print a decoded address with optional color."""
        if use_color:
            ca = f"{colorama.Fore.YELLOW}{addr}{colorama.Style.RESET_ALL}"
            cd = f"{colorama.Fore.YELLOW}{decoded}{colorama.Style.RESET_ALL}"
            print(f"  -> {ca}: {cd}", flush=True)
        else:
            print(f"  -> {addr}: {decoded}", flush=True)

    def _add_timestamp(self, args, raw, delim):
        """Add timestamp to output if enabled."""
        if args.timestamp and delim != "CR":
            ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            return f"[{ts}] {raw}"
        return raw

    def _print_output(self, raw, delim, nl_tag, log_fp, payload):
        """Print output to console and log file."""
        if delim == "CR":
            self._print_carriage_return_output(raw, log_fp)
        else:
            self._print_normal_output(raw, nl_tag, log_fp, payload)

    def _print_carriage_return_output(self, raw, log_fp):
        """Print carriage return output."""
        print("\r" + raw, end="", flush=True)
        if log_fp:
            with suppress(Exception):
                log_fp.write(raw.encode("utf-8", errors="replace"))

    def _print_normal_output(self, raw, nl_tag, log_fp, payload):
        """Print normal output with optional newline."""
        endch = "\n" if nl_tag == "1" else ""
        print(raw, end=endch, flush=True)
        if log_fp:
            with suppress(Exception):
                log_fp.write((payload + ("\n" if endch else "")).encode("utf-8", errors="replace"))

    def _cleanup(self, rx, kb):
        """Cleanup resources on shutdown."""
        self._stop_evt.set()

        s = self._ser_ref.get()
        if s:
            with suppress(Exception):
                s.close()

        rx.join(timeout=THREAD_JOIN_TIMEOUT)

        if kb:
            kb.join(timeout=THREAD_JOIN_TIMEOUT)

        log.inf("monitor: disconnected")
