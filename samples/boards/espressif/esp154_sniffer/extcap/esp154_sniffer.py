#!/usr/bin/env python3
# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

"""Wireshark extcap plugin for the Espressif IEEE 802.15.4 sniffer sample.

The firmware prints one line per captured frame on its console UART:

    received: <psdu-hex> power: <rssi-dbm> lqi: <lqi> time: <us>

The PSDU carries no FCS: the radio drops frames with a bad checksum and
overwrites the checksum bytes with RSSI and LQI, so the sample strips them.
Lines that do not match are ignored, which is what lets the capture share the
UART with the shell and the log backend.

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
DOC_URL = (
    "https://docs.zephyrproject.org/latest/samples/boards/espressif/esp154_sniffer/README.html"
)

# http://www.tcpdump.org/linktypes.html
DLT_IEEE802_15_4_NOFCS = 230
DLT_IEEE802_15_4_TAP = 283

# TLV types from the IEEE 802.15.4 TAP specification,
# https://github.com/jkcko/ieee802.15.4-tap
TAP_TLV_RSS = 1
TAP_TLV_CHANNEL = 3
TAP_TLV_LQI = 10

# The firmware reports an unavailable RSSI as INT16_MIN
# (IEEE802154_MAC_RSSI_DBM_UNDEFINED).
RSSI_UNDEFINED = -32768

# The device timestamp is a 32-bit microsecond counter, so it wraps every
# ~71.6 minutes and captures routinely outlive that.
TIMESTAMP_MODULO = 2**32

FRAME_RE = re.compile(
    r"received:\s+([0-9a-fA-F]+)\s+"
    r"power:\s+(-?\d+)\s+"
    r"lqi:\s+(\d+)\s+"
    r"time:\s+(-?\d+)"
)

DEFAULT_CHANNEL = 20
DEFAULT_BAUDRATE = 921600


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


def pcap_file_header(dlt):
    """Classic libpcap file header, microsecond resolution."""
    return struct.pack("<IHHiIII", 0xA1B2C3D4, 2, 4, 0, 0, 65535, dlt)


def tap_header(channel, rssi, lqi):
    """IEEE 802.15.4 TAP header carrying the out-of-band metadata.

    No FCS Type TLV is emitted; its absence is what tells the dissector the
    frame has no FCS.
    """
    tlvs = b""

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
    legacy /dev/ttyS* devices out of Wireshark's interface list. The devkits
    appear either as an Espressif USB Serial/JTAG device or through the
    board's USB-UART bridge, so the vendor id is not narrowed further than
    that; the port description tells several boards apart.
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
        "{tooltip=Must match the console speed the firmware was built with}"
        "{type=integer}{default=" + str(DEFAULT_BAUDRATE) + "}{group=Capture}",
    ]


class Capture:
    """Drives one capture session: serial in, pcap out."""

    def __init__(self, device, baudrate, channel, use_tap):
        self.device = device
        self.baudrate = baudrate
        self.channel = channel
        self.use_tap = use_tap
        self.clock = DeviceClock()
        self.running = True

    def stop(self, *_args):
        self.running = False

    def _command(self, serial, command):
        serial.write(command.encode() + b"\r\n")
        serial.flush()

    def _configure(self, serial):
        # Command echo would otherwise come back interleaved with frames.
        self._command(serial, "shell echo off")
        # The channel can only be retuned while the radio is stopped.
        self._command(serial, "sniffer stop")
        serial.reset_input_buffer()
        self._command(serial, f"sniffer channel {self.channel}")
        self._command(serial, "sniffer start")

    def _record(self, match):
        psdu = bytes.fromhex(match.group(1))
        rssi = int(match.group(2))
        lqi = int(match.group(3))
        device_us = int(match.group(4)) % TIMESTAMP_MODULO

        if self.use_tap:
            psdu = tap_header(self.channel, rssi, lqi) + psdu

        return pcap_record(self.clock.unix_us(device_us), psdu)

    def run(self, fifo_path):
        dlt = DLT_IEEE802_15_4_TAP if self.use_tap else DLT_IEEE802_15_4_NOFCS

        with Serial(self.device, self.baudrate, timeout=0.1, exclusive=True) as serial:
            self._configure(serial)

            # Opening the fifo blocks until Wireshark opens the read end.
            with open(fifo_path, "wb") as fifo:
                fifo.write(pcap_file_header(dlt))
                fifo.flush()

                while self.running:
                    line = serial.readline()
                    if not line:
                        continue

                    match = FRAME_RE.search(line.decode("ascii", errors="replace"))
                    if match is None:
                        continue

                    fifo.write(self._record(match))
                    # Wireshark only shows what has reached the fifo.
                    fifo.flush()

            self._command(serial, "sniffer stop")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])

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
        "--baudrate", type=int, default=DEFAULT_BAUDRATE, help="console speed of the firmware"
    )
    parser.add_argument(
        "--metadata",
        choices=("tap", "none"),
        default="tap",
        help="per-frame IEEE 802.15.4 TAP header; 'none' writes bare frames "
        "and is only useful for standalone captures",
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

    capture = Capture(args.extcap_interface, args.baudrate, args.channel, args.metadata == "tap")

    signal.signal(signal.SIGINT, capture.stop)
    signal.signal(signal.SIGTERM, capture.stop)

    try:
        capture.run(args.fifo)
    except SerialException as err:
        sys.exit(f"{args.extcap_interface}: {err}")
    except BrokenPipeError:
        # Wireshark closed the fifo, which is how a capture normally ends.
        pass

    return 0


if __name__ == "__main__":
    sys.exit(main())
