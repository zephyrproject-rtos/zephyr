#!/usr/bin/env python3
# Copyright (c) 2026 Vincent Jardin <vjardin@free.fr>, Free Mobile
# SPDX-License-Identifier: Apache-2.0
#
# Reboot RP2040/RP2350 boards running shell_bootloader firmware into BOOTSEL.
# Sends a USB SET_LINE_CODING control transfer with 1200 baud directly
# to the device by VID:PID, without going through /dev/ttyACM*.
#
# Usage:
#   python3 bootsel.py                    # all matching devices
#   python3 bootsel.py --bus 1 --addr 10  # specific device by USB address
#
# Requires: pip install pyusb

import argparse
import contextlib
import struct
import sys
import time

import usb.core
import usb.util

# Zephyr CDC ACM default VID:PID
VID = 0x2FE3
PID = 0x0004

# CDC SET_LINE_CODING request
CDC_SET_LINE_CODING = 0x20


def bootsel_one(dev):
    """Reboot a single device into BOOTSEL mode."""
    print(f"  bus {dev.bus} addr {dev.address}: ", end="")

    # Detach kernel driver if active
    for intf in (0, 1):
        with contextlib.suppress(usb.core.USBError):
            if dev.is_kernel_driver_active(intf):
                dev.detach_kernel_driver(intf)

    # SET_LINE_CODING: 1200 baud, 1 stop bit, no parity, 8 data bits
    line_coding = struct.pack("<IBBB", 1200, 0, 0, 8)

    # Device may reset before ACK
    with contextlib.suppress(usb.core.USBError):
        dev.ctrl_transfer(0x21, CDC_SET_LINE_CODING, 0, 0, line_coding)

    print("BOOTSEL triggered")


def main():
    parser = argparse.ArgumentParser(
        description="Reboot RP2040/RP2350 into BOOTSEL via USB",
        allow_abbrev=False,
    )
    parser.add_argument("--bus", type=int, help="USB bus number")
    parser.add_argument("--addr", type=int, help="USB device address")
    args = parser.parse_args()

    kwargs = {"idVendor": VID, "idProduct": PID}
    if args.bus is not None:
        kwargs["bus"] = args.bus
    if args.addr is not None:
        kwargs["address"] = args.addr

    if args.bus is not None or args.addr is not None:
        # Target a specific device
        dev = usb.core.find(**kwargs)
        if dev is None:
            print(
                f"No device found with VID:PID {VID:04x}:{PID:04x}",
                file=sys.stderr,
            )
            sys.exit(1)
        bootsel_one(dev)
    else:
        # Target all matching devices
        devices = list(usb.core.find(find_all=True, **kwargs))
        if not devices:
            print(
                f"No devices found with VID:PID {VID:04x}:{PID:04x}",
                file=sys.stderr,
            )
            sys.exit(1)

        print(f"Found {len(devices)} device(s)")
        for dev in devices:
            bootsel_one(dev)
            time.sleep(0.5)

    print("Done")


if __name__ == "__main__":
    main()
