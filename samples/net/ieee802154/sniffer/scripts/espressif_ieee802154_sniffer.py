#!/usr/bin/env python3
# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

"""Wireshark extcap plugin for the Espressif IEEE 802.15.4 sniffer sample.

The firmware prints one line per captured frame on its console UART:

    psdu: <hex> power: <rssi-dbm> lqi: <lqi> time: <us>

The PSDU carries no FCS: the radio drops frames with a bad checksum and
overwrites the checksum bytes with RSSI and LQI, so the sample strips them.
The checksum is recomputed here, because tools that read the capture expect a
complete frame. Lines that do not match are ignored, which is what lets the
capture share the UART with the shell and the log backend.

Run with --help for standalone use, or install it into Wireshark's extcap
directory to capture from the Wireshark interface list.
"""

import argparse
import re
import signal
import struct
import sys
import time

try:
    from serial import Serial, SerialException
    from serial.tools.list_ports import comports
except ImportError:
    sys.exit("pyserial is required: pip install pyserial")

EXTCAP_VERSION = "1.0"
DOC_URL = "https://docs.zephyrproject.org/latest/samples/net/ieee802154/sniffer/README.html"

# http://www.tcpdump.org/linktypes.html
DLT_IEEE802_15_4_WITHFCS = 195
DLT_IEEE802_15_4_TAP = 283

# TLV types from the IEEE 802.15.4 TAP specification,
# https://github.com/jkcko/ieee802.15.4-tap
TAP_TLV_FCS_TYPE = 0
TAP_TLV_RSS = 1
TAP_TLV_CHANNEL = 3
TAP_TLV_LQI = 10

# FCS Type TLV value for the 16-bit CRC that IEEE 802.15.4 uses.
TAP_FCS_TYPE_CRC16 = 1

# IEEE 802.15.4 FCS: CRC-16 with the reflected polynomial and a zero seed.
FCS_POLYNOMIAL = 0x8408

# The firmware reports an unavailable RSSI as INT16_MIN
# (IEEE802154_MAC_RSSI_DBM_UNDEFINED).
RSSI_UNDEFINED = -32768

# The device timestamp is a 32-bit microsecond counter.
TIMESTAMP_MODULO = 2**32

FRAME_RE = re.compile(
    r"psdu:\s+([0-9a-fA-F]+)\s+"
    r"power:\s+(-?\d+)\s+"
    r"lqi:\s+(\d+)\s+"
    r"time:\s+(-?\d+)"
)

DEFAULT_CHANNEL = 20
DEFAULT_BAUDRATE = 115200

# The console is raised to this for the duration of a capture and put back
# afterwards, because a maximum-length frame takes 26 ms to print at 115200 baud,
# slower than a busy channel delivers frames.
DEFAULT_CAPTURE_BAUDRATE = 921600

# Tried in turn when the board does not answer at the rate that was asked for,
# which covers a board left at the capture rate by a capture that was killed.
FALLBACK_BAUDRATES = (115200, 921600)

# The shell prompt, which a bare newline brings out even on a quiet channel.
PROMPT = "uart:~$"


class DeviceClock:
    """Maps the device's wrapping microsecond counter onto UNIX time.

    The device counts from boot, so the first frame anchors the two clocks and
    every later frame is placed relative to that anchor.
    """

    def __init__(self):
        self.offset_us = None
        self.previous_us = 0
        self.wraps = 0

    def unix_us(self, device_us):
        if self.offset_us is None:
            self.offset_us = int(time.time() * 1_000_000) - device_us
        elif device_us < self.previous_us:
            self.wraps += 1

        self.previous_us = device_us

        return self.offset_us + self.wraps * TIMESTAMP_MODULO + device_us


def fcs(psdu):
    """The two IEEE 802.15.4 FCS bytes for a PSDU, little endian.

    The radio verified the checksum of every frame it reports and then
    overwrote those two bytes with RSSI and LQI, so the value is recomputed
    from the PSDU rather than carried over the serial line.
    """
    crc = 0

    for byte in psdu:
        crc ^= byte

        for _ in range(8):
            crc = (crc >> 1) ^ FCS_POLYNOMIAL if crc & 1 else crc >> 1

    return struct.pack("<H", crc)


def pcap_file_header(dlt):
    """Classic libpcap file header, microsecond resolution."""
    return struct.pack("<IHHiIII", 0xA1B2C3D4, 2, 4, 0, 0, 65535, dlt)


def tap_header(channel, rssi, lqi):
    """IEEE 802.15.4 TAP header carrying the out-of-band metadata.

    The FCS Type TLV has to be present: without it the dissector assumes the
    frame carries no checksum and shows the two FCS bytes as payload.
    """
    tlvs = struct.pack("<HHI", TAP_TLV_FCS_TYPE, 1, TAP_FCS_TYPE_CRC16)

    if rssi != RSSI_UNDEFINED:
        tlvs += struct.pack("<HHf", TAP_TLV_RSS, 4, float(rssi))

    tlvs += struct.pack("<HHHBB", TAP_TLV_CHANNEL, 3, channel, 0, 0)
    tlvs += struct.pack("<HHI", TAP_TLV_LQI, 1, lqi)

    # Version 0, one reserved byte, then the total length including this header.
    return struct.pack("<BBH", 0, 0, 4 + len(tlvs)) + tlvs


def pcap_record(timestamp_us, payload):
    header = struct.pack(
        "<IIII", timestamp_us // 1_000_000, timestamp_us % 1_000_000, len(payload), len(payload)
    )
    return header + payload


def extcap_interfaces():
    """Offer the USB serial ports.

    Ports without a USB vendor id are skipped, which keeps the machine's
    legacy /dev/ttyS* devices out of Wireshark's interface list. The capture
    runs on the console UART, which the devkits expose through their USB-UART
    bridge; the port description tells several boards apart.
    """
    lines = [
        "extcap {version=" + EXTCAP_VERSION + "}"
        "{display=Espressif IEEE 802.15.4 sniffer}"
        "{help=" + DOC_URL + "}"
    ]

    for port in sorted(comports(), key=lambda p: p.device):
        if port.vid is None:
            continue

        lines.append(
            "interface {value=" + port.device + "}"
            "{display=Espressif IEEE 802.15.4 sniffer (" + port.description + ")}"
        )

    return lines


def extcap_dlts():
    return [
        "dlt {number=" + str(DLT_IEEE802_15_4_TAP) + "}"
        "{name=IEEE802_15_4_TAP}{display=IEEE 802.15.4 TAP}"
    ]


def extcap_config():
    """Capture options offered in Wireshark's interface settings dialog.

    The link type is fixed to TAP here: extcap advertises one DLT per
    interface, so --metadata stays a command-line-only debugging switch.
    """
    return [
        "arg {number=0}{call=--channel}{display=Channel}"
        "{tooltip=IEEE 802.15.4 channel to tune the radio to}"
        "{type=integer}{range=11,26}{default=" + str(DEFAULT_CHANNEL) + "}{group=Capture}",
        "arg {number=1}{call=--baudrate}{display=Baud rate}"
        "{tooltip=Console speed of the firmware; the other common rate is tried "
        "if the board does not answer}"
        "{type=integer}{default=" + str(DEFAULT_BAUDRATE) + "}{group=Capture}",
        "arg {number=2}{call=--capture-baudrate}{display=Capture baud rate}"
        "{tooltip=Speed the console is raised to while capturing, and put back "
        "afterwards; 0 leaves it alone}"
        "{type=integer}{default=" + str(DEFAULT_CAPTURE_BAUDRATE) + "}{group=Capture}",
        "arg {number=3}{call=--log-file}{display=Diagnostic log}"
        "{tooltip=File to write the device handshake and the lines that did not parse to}"
        "{type=string}{group=Capture}",
    ]


def discard(_record):
    """Sink for the teardown, where a frame is no longer wanted."""


def looks_like_text(line):
    """Tell console output apart from what a wrong baud rate turns it into.

    Bytes that do not survive the ASCII decode come back as replacement
    characters, so a line that is almost all printable is a line read at the
    rate the board is actually sending at.
    """
    if not line:
        return False

    printable = sum(1 for char in line if " " <= char <= "~")

    return printable >= len(line) * 0.9


class Log:
    """Optional record of what the device sent.

    Wireshark discards whatever an extcap plugin writes to stderr, so a capture
    that stays empty leaves nothing to look at. This keeps the handshake and the
    lines that did not parse, which is what tells a quiet channel apart from a
    port whose bytes a second reader is taking away.
    """

    UNMATCHED_LOGGED = 20

    def __init__(self, path):
        self.path = path
        self.file = None
        self.matched = 0
        self.unmatched = 0

    def __enter__(self):
        if self.path:
            self.file = open(self.path, "w", encoding="ascii", errors="replace")

        return self

    def __exit__(self, *_exc_info):
        self.write(f"{self.matched} frame lines, {self.unmatched} other lines")

        if self.file is not None:
            self.file.close()
            self.file = None

        return False

    def write(self, text):
        if self.file is None:
            return

        self.file.write(time.strftime("%H:%M:%S ") + text + "\n")
        self.file.flush()

    def count(self, text, matched, label="ignored"):
        if matched:
            self.matched += 1
            return

        self.unmatched += 1

        if self.unmatched <= self.UNMATCHED_LOGGED:
            self.write(f"{label}: {text!r}")


class LineReader:
    """Splits the serial stream into complete lines.

    Serial.readline() reads one byte per system call and, when a read times out
    in the middle of a line, hands back the fragment it has. The rest then
    arrives as a line of its own, and both halves fail to parse.
    """

    # A capture line tops out near 300 characters, anything longer is noise.
    MAX_PENDING = 8192

    def __init__(self, port):
        self.port = port
        self.pending = bytearray()

    def flush(self):
        """Drop everything buffered, on the port and in the partial line."""
        self.port.reset_input_buffer()
        self.pending = bytearray()

    def read(self):
        self.pending += self.port.read(max(1, self.port.in_waiting))
        lines = []

        while b"\n" in self.pending:
            raw, _, rest = self.pending.partition(b"\n")
            self.pending = bytearray(rest)
            lines.append(raw.decode("ascii", errors="replace").strip())

        if len(self.pending) > self.MAX_PENDING:
            self.pending = bytearray()

        return lines


class Capture:
    """Drives one capture session: serial in, pcap out."""

    # How long one command is given to be answered.
    COMMAND_TIMEOUT = 1.0
    COMMAND_TRIES = 2
    # How long to listen before deciding baud rate is wrong.
    PROBE_SECONDS = 1.0
    # Long enough for the replies that are thrown away before the handshake.
    SETTLE_SECONDS = 0.2
    # The firmware holds its reply back this long before it changes the rate.
    SPEED_DRAIN_SECONDS = 0.1

    def __init__(self, device, baudrate, capture_baudrate, channel, use_tap, log):
        self.device = device
        self.baudrate = baudrate
        self.capture_baudrate = capture_baudrate
        self.channel = channel
        self.use_tap = use_tap
        self.log = log
        self.clock = DeviceClock()
        self.running = True
        # The rate to put the console back to.
        self.restore_baudrate = None

    def stop(self, *_args):
        self.running = False

    def _open(self, baudrate):
        """Open the port without resetting the board.

        DTR and RTS drive the auto-reset circuit of the devkits, and pySerial
        asserts both when it opens a port: the board reboots, and a part with a
        USB serial/JTAG peripheral can be left in the download stub, where it
        never sends a frame again. Setting the two lines before the port is
        open makes pySerial drop them as it opens, leaving the firmware alone.
        """
        port = Serial()

        port.port = self.device
        port.baudrate = baudrate
        port.timeout = 0.1
        port.exclusive = True
        port.dtr = False
        port.rts = False

        port.open()

        return port

    def _command(self, port, command):
        port.write(command.encode() + b"\r\n")
        port.flush()

    def _record(self, match):
        psdu = bytes.fromhex(match.group(1))
        rssi = int(match.group(2))
        lqi = int(match.group(3))
        device_us = int(match.group(4)) % TIMESTAMP_MODULO

        psdu += fcs(psdu)

        if self.use_tap:
            psdu = tap_header(self.channel, rssi, lqi) + psdu

        return pcap_record(self.clock.unix_us(device_us), psdu)

    def _frame(self, line, sink):
        """Write one line out as a frame, and say whether it was one."""
        match = FRAME_RE.search(line)

        if match is None:
            return False

        sink(self._record(match))

        return True

    def _probe(self, reader, sink):
        """Read what the board sends, to tell a wrong baud rate apart.

        A bare newline makes the shell reprint its prompt, which answers even on
        a channel with no traffic. Frames and readable text count too: the shell
        thread runs at the lowest application priority, below the thread that
        prints the frames, so on a busy channel a command can go unanswered for
        a long time while frames keep coming out.
        """
        self._command(reader.port, "")
        deadline = time.monotonic() + self.PROBE_SECONDS

        while time.monotonic() < deadline:
            for line in reader.read():
                if not line:
                    continue

                found = self._frame(line, sink)
                self.log.count(line, found, "device")

                if found or PROMPT in line or looks_like_text(line):
                    return line

        return None

    def _connect(self, sink):
        """Open the port at the rate the board is talking at.

        The rate is chosen from what the board sends, not from whether it answers
        a command: a firmware that streams frames but never answers is still a
        capture that works, so the requested rate is kept when no rate stands
        out.
        """
        rates = [self.baudrate]
        rates += [rate for rate in FALLBACK_BAUDRATES if rate != self.baudrate]

        for rate in rates:
            port = self._open(rate)
            reader = LineReader(port)

            self.log.write(f"{self.device} open at {rate} baud")

            if self._probe(reader, sink) is not None:
                if rate != self.baudrate:
                    self.log.write(f"console runs at {rate} baud, not {self.baudrate}")

                return port, reader

            self.log.write(f"nothing readable at {rate} baud")
            port.close()

        # Every rate was silent, so capture at the rate that was asked for.
        self.log.write(f"no output from the board; capturing at {self.baudrate} baud")
        port = self._open(self.baudrate)

        return port, LineReader(port)

    def _expect(self, reader, command, tokens, sink):
        """Read replies until one carries an expected token.

        Frames keep being written out while this waits, and lines holding the
        command itself are skipped, because the shell echoes commands until
        'shell echo off' has taken effect.
        """
        deadline = time.monotonic() + self.COMMAND_TIMEOUT

        while time.monotonic() < deadline:
            for line in reader.read():
                if not line:
                    continue

                if self._frame(line, sink):
                    self.log.count(line, True, "device")
                    continue

                self.log.count(line, False, "device")

                if command not in line and any(token in line for token in tokens):
                    return line

        return None

    def _run_command(self, reader, command, tokens, sink, tries=COMMAND_TRIES):
        """Send a command until it is answered, and report whether it was.

        A command can go unanswered for reasons that do not stop a capture: the
        shell thread is the lowest priority one in the sample, and its replies
        share the UART with the frames, so a reply can be starved or come out
        mixed into a frame line.
        """
        for attempt in range(1, tries + 1):
            self._command(reader.port, command)

            reply = self._expect(reader, command, tokens, sink)
            if reply is not None:
                self.log.write(f"'{command}' -> {reply}")
                return True

            self.log.write(f"'{command}' unanswered, try {attempt} of {tries}")

        return False

    def _set_speed(self, reader, sink, baudrate):
        """Raise the console rate for the capture, and follow the board there.

        The reply arrives at the rate the command was sent at, because the
        firmware holds the change back until it has drained. A board that does
        not acknowledge keeps the rate it had, so the host goes back to it: a
        slower capture beats none.
        """
        previous = reader.port.baudrate

        if baudrate in (0, previous):
            return

        command = f"sniffer speed {baudrate}"

        self._run_command(reader, command, (f"speed {baudrate}",), sink, tries=1)

        time.sleep(self.SPEED_DRAIN_SECONDS)
        reader.port.baudrate = baudrate
        reader.flush()

        # Reading the rate back is what confirms the switch.
        if not self._run_command(reader, "sniffer speed", (f"speed {baudrate}",), sink):
            self.log.write(f"no answer at {baudrate} baud, back to {previous}")
            reader.port.baudrate = previous
            reader.flush()
            return

        self.log.write(f"console raised to {baudrate} baud")
        self.restore_baudrate = previous

    def _restore_speed(self, reader):
        """Put the console back to the rate the shell is used at.

        A late frame is no longer wanted here, so nothing is written out. A
        capture killed outright leaves the board at the capture rate, which the
        next one finds through FALLBACK_BAUDRATES and puts back on its way out.
        """
        if self.restore_baudrate is None:
            return

        command = f"sniffer speed {self.restore_baudrate}"
        tokens = (f"speed {self.restore_baudrate}",)

        if self._run_command(reader, command, tokens, discard, tries=1):
            time.sleep(self.SPEED_DRAIN_SECONDS)
            self.log.write(f"console back at {self.restore_baudrate} baud")
        else:
            self.log.write(f"the console may still be at {reader.port.baudrate} baud")

    def _configure(self, reader, sink):
        """Retune and start the radio, reporting what was not acknowledged.

        Every command is best effort. A board that ignores the shell still sends
        frames, and refusing to capture those would throw away the capture over
        a setting that may well already be right.
        """
        self._command(reader.port, "shell echo off")
        self._command(reader.port, "sniffer stop")

        # Drop the replies.
        time.sleep(self.SETTLE_SECONDS)

        self._set_speed(reader, sink, self.capture_baudrate)

        tuned = self._run_command(
            reader, f"sniffer channel {self.channel}", (f"channel {self.channel}",), sink
        )
        self._run_command(reader, "sniffer start", ("started",), sink)
        self._run_command(reader, "sniffer stats", ("enq=",), sink, tries=1)

        if not tuned:
            self.log.write(
                f"the board did not confirm channel {self.channel}; it may still be on "
                "the channel it was last set to"
            )

    def run(self, fifo_path):
        dlt = DLT_IEEE802_15_4_TAP if self.use_tap else DLT_IEEE802_15_4_WITHFCS

        # Opening the fifo blocks until Wireshark opens the read end.
        with open(fifo_path, "wb") as fifo:
            fifo.write(pcap_file_header(dlt))
            fifo.flush()

            def sink(record):
                fifo.write(record)
                fifo.flush()

            port, reader = self._connect(discard)

            with port:
                try:
                    self._configure(reader, discard)

                    while self.running:
                        for line in reader.read():
                            if line:
                                self.log.count(line, self._frame(line, sink))
                finally:
                    self._command(port, "sniffer stop")
                    self._restore_speed(reader)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0], allow_abbrev=False)

    parser.add_argument("--extcap-interfaces", action="store_true", help="list interfaces")
    parser.add_argument("--extcap-dlts", action="store_true", help="list link types")
    parser.add_argument("--extcap-config", action="store_true", help="list capture options")
    parser.add_argument("--extcap-version", help="Wireshark version, ignored")
    parser.add_argument("--extcap-interface", help="serial port of the sniffer")
    parser.add_argument("--extcap-capture-filter", help="capture filter, unsupported")
    parser.add_argument("--capture", action="store_true", help="start capturing")
    parser.add_argument("--fifo", help="fifo or pcap file to write to")
    parser.add_argument(
        "--channel", type=int, default=DEFAULT_CHANNEL, help="IEEE 802.15.4 channel, 11-26"
    )
    parser.add_argument(
        "--baudrate",
        type=int,
        default=DEFAULT_BAUDRATE,
        help="console speed of the firmware; other common rates are tried if it does not answer",
    )
    parser.add_argument(
        "--capture-baudrate",
        type=int,
        default=DEFAULT_CAPTURE_BAUDRATE,
        help="speed the console is raised to while capturing and put back to afterwards; "
        "0 leaves it alone",
    )
    parser.add_argument(
        "--log-file",
        help="write the device handshake and the lines that did not parse to this file",
    )
    parser.add_argument(
        "--metadata",
        choices=("tap", "none"),
        default="tap",
        help="per-frame IEEE 802.15.4 TAP header; 'none' writes bare frames "
        "with an FCS and is only useful for standalone captures",
    )

    return parser.parse_args()


def main():
    args = parse_args()

    if args.extcap_interfaces:
        print("\n".join(extcap_interfaces()))
        return 0

    if args.extcap_dlts:
        print("\n".join(extcap_dlts()))
        return 0

    if args.extcap_config:
        print("\n".join(extcap_config()))
        return 0

    if not args.capture:
        sys.exit("nothing to do: pass --capture, or one of the --extcap-* queries")

    if args.fifo is None or args.extcap_interface is None:
        sys.exit("--capture needs both --fifo and --extcap-interface")

    if not 11 <= args.channel <= 26:
        sys.exit(f"channel {args.channel} is outside 11-26")

    with Log(args.log_file) as log:
        capture = Capture(
            args.extcap_interface,
            args.baudrate,
            args.capture_baudrate,
            args.channel,
            args.metadata == "tap",
            log,
        )

        signal.signal(signal.SIGINT, capture.stop)
        signal.signal(signal.SIGTERM, capture.stop)

        try:
            capture.run(args.fifo)
        except SerialException as err:
            log.write(f"{args.extcap_interface}: {err}")
            sys.exit(f"{args.extcap_interface}: {err}")
        except BrokenPipeError:
            # Wireshark closed the fifo.
            pass

    return 0


if __name__ == "__main__":
    sys.exit(main())
