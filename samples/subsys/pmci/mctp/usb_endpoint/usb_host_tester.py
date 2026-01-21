#!/usr/bin/env python3
# Copyright 2025-2026 NXP
# SPDX-License-Identifier: Apache-2.0
#
# Simple Zephyr MCTP-over-USB host tester (no CLI args)
# - HS/FS agnostic (uses endpoint wMaxPacketSize at runtime)
# - Works across repeated runs (detach kernel driver, clear halt, drain IN)
# - Correct framed RX (handles header+payload in same read, multiple frames in one read)
# - Includes a ZLP-focused test (meaningful mainly on Full-Speed due to 1-byte LEN)

from __future__ import annotations

import time
from contextlib import suppress

import usb.core
import usb.util

VID = 0x2FE3
PID = 0x0001

MCTP_DMTF0 = 0x1A
MCTP_DMTF1 = 0xB4

DEFAULT_TIMEOUT_MS = 1500


class USBDevice:
    def __init__(self, vid, pid):
        self.vid = vid
        self.pid = pid
        self.device = None
        self.endpoint_in = None
        self.endpoint_out = None
        self.interface = None
        self._detached = False

        self.in_mps = None
        self.out_mps = None

        # SW buffer for leftover bytes (if we read more than one frame at once)
        self._rx_buf = bytearray()

    def connect(self):
        self.device = usb.core.find(idVendor=self.vid, idProduct=self.pid)
        if self.device is None:
            raise ValueError("Device not found. Make sure the board is connected (check lsusb).")

        # Prefer using the current active configuration if already set by OS/userspace.
        # Calling set_configuration() can fail with EBUSY on some systems.
        try:
            cfg = self.device.get_active_configuration()
        except usb.core.USBError:
            # No active configuration yet -> set it
            self.device.set_configuration()
            cfg = self.device.get_active_configuration()

        self.interface = cfg[(0, 0)]
        intf_num = self.interface.bInterfaceNumber

        # Detach kernel driver if present (important if /dev/ttyACM* grabs it)
        with suppress(NotImplementedError, usb.core.USBError):
            if self.device.is_kernel_driver_active(intf_num):
                self.device.detach_kernel_driver(intf_num)
                self._detached = True

        usb.util.claim_interface(self.device, intf_num)

        # Find bulk IN and OUT endpoints
        self.endpoint_in = usb.util.find_descriptor(
            self.interface,
            custom_match=lambda e: (
                usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN
                and usb.util.endpoint_type(e.bmAttributes) == usb.util.ENDPOINT_TYPE_BULK
            ),
        )
        self.endpoint_out = usb.util.find_descriptor(
            self.interface,
            custom_match=lambda e: (
                usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT
                and usb.util.endpoint_type(e.bmAttributes) == usb.util.ENDPOINT_TYPE_BULK
            ),
        )

        if not self.endpoint_in or not self.endpoint_out:
            raise ValueError("Bulk IN/OUT endpoints not found")

        self.in_mps = int(self.endpoint_in.wMaxPacketSize)
        self.out_mps = int(self.endpoint_out.wMaxPacketSize)

        # Clear halts just in case a previous run left the pipe stalled
        with suppress(usb.core.USBError):
            self.device.clear_halt(self.endpoint_in.bEndpointAddress)
        with suppress(usb.core.USBError):
            self.device.clear_halt(self.endpoint_out.bEndpointAddress)

        # Drain stale IN bytes so we don't parse an old reply
        self._drain_in()

        print(
            f"Connected to device {self.vid:04x}:{self.pid:04x} "
            f"(IN_MPS={self.in_mps}, OUT_MPS={self.out_mps})"
        )

        # Print descriptor information (debug)
        for cfg in self.device:
            print(cfg)

    def _drain_in(self):
        self._rx_buf = bytearray()
        while True:
            with suppress(usb.core.USBError):
                _ = self.endpoint_in.read(self.in_mps, timeout=20)
                continue
            break

    def send_data(self, data, timeout=1000):
        if not self.endpoint_out:
            raise RuntimeError("OUT endpoint not initialized")
        return self.endpoint_out.write(data, timeout=timeout)

    def _read_in(self, timeout_ms):
        chunk = bytes(self.endpoint_in.read(self.in_mps, timeout=timeout_ms))
        if chunk:
            self._rx_buf.extend(chunk)

    @staticmethod
    def _find_header(buf):
        # Find first occurrence of DMTF0 DMTF1 0x00
        for i in range(max(0, len(buf) - 2)):
            if buf[i] == MCTP_DMTF0 and buf[i + 1] == MCTP_DMTF1 and buf[i + 2] == 0x00:
                return i
        return 0

    def recv_mctp_frame(self, timeout=DEFAULT_TIMEOUT_MS):
        """
        Read exactly one MCTP-over-USB framed packet:
        [DMTF0][DMTF1][RSVD][LEN] + (LEN-4) bytes payload

        Robust to:
        - header+payload in same read
        - multiple frames in one read
        - stale bytes (we also drain on connect)
        """
        deadline = time.time() + (timeout / 1000.0)

        while True:
            # Need at least 4 bytes for header
            while len(self._rx_buf) < 4:
                remain_ms = int(max(0.0, (deadline - time.time()) * 1000))
                if remain_ms == 0:
                    raise usb.core.USBTimeoutError("Timeout waiting for MCTP header")
                self._read_in(remain_ms)

            # Resync to header
            idx = self._find_header(self._rx_buf)
            if idx > 0:
                del self._rx_buf[:idx]

            if len(self._rx_buf) < 4:
                continue

            if (
                self._rx_buf[0] != MCTP_DMTF0
                or self._rx_buf[1] != MCTP_DMTF1
                or self._rx_buf[2] != 0x00
            ):
                del self._rx_buf[0]
                continue

            total_len = int(self._rx_buf[3])
            if total_len < 4:
                del self._rx_buf[:4]
                continue

            while len(self._rx_buf) < total_len:
                remain_ms = int(max(0.0, (deadline - time.time()) * 1000))
                if remain_ms == 0:
                    raise usb.core.USBTimeoutError(
                        f"Timeout waiting full frame: have={len(self._rx_buf)} need={total_len}"
                    )
                self._read_in(remain_ms)

            frame = bytes(self._rx_buf[:total_len])
            del self._rx_buf[:total_len]
            return frame

    def disconnect(self):
        if self.device is None or self.interface is None:
            return

        intf_num = self.interface.bInterfaceNumber

        with suppress(usb.core.USBError):
            usb.util.release_interface(self.device, intf_num)

        with suppress(usb.core.USBError):
            usb.util.dispose_resources(self.device)

        # Re-attach kernel driver if we detached it
        if self._detached:
            with suppress(usb.core.USBError):
                self.device.attach_kernel_driver(intf_num)

        self.device = None
        self.interface = None
        self.endpoint_in = None
        self.endpoint_out = None
        self._detached = False
        self._rx_buf = bytearray()


def build_frame(payload):
    """
    Build: DMTF0 DMTF1 0x00 LEN payload
    LEN is 1 byte -> total frame length must be <= 255.
    """
    total_len = 4 + len(payload)
    if total_len > 255:
        raise ValueError(f"Frame too large for 1-byte LEN: total_len={total_len} (>255)")
    return bytes([MCTP_DMTF0, MCTP_DMTF1, 0x00, total_len]) + payload


if __name__ == "__main__":
    usb_dev = USBDevice(VID, PID)
    try:
        usb_dev.connect()

        print("Test1: small message")
        usb_dev.send_data(build_frame(b"\x01\x0a\x14\xc0Hello, MCX\x00"))
        resp = usb_dev.recv_mctp_frame(timeout=DEFAULT_TIMEOUT_MS)
        print("Received:", resp)

        print("Test2: long message (still <=255 total)")
        payload2 = b"\x01\x0a\x14\xc0" + (b"A" * 160)
        usb_dev.send_data(build_frame(payload2))
        resp = usb_dev.recv_mctp_frame(timeout=DEFAULT_TIMEOUT_MS)
        print("Received:", resp)

        print("Test3: two messages in one OUT transfer (second reply is optional)")
        msg1 = build_frame(b"\x01\x0a\x14\xc0Hello, MCX, 1\x00")
        msg2 = build_frame(b"\x01\x0a\x14\xc0Hello, MCX, 2\x00")
        usb_dev.send_data(msg1 + msg2)
        resp = usb_dev.recv_mctp_frame(timeout=DEFAULT_TIMEOUT_MS)
        print("Received:", resp)

        print("Test4: ZLP-focused stress")
        print("  NOTE: With 1-byte LEN (<=255), this is meaningful mainly for FS (MPS=64).")
        if usb_dev.in_mps > 255:
            print(
                "  Skipping: IN_MPS="
                f"{usb_dev.in_mps} but LEN<=255, cannot hit multiple-of-MPS for ZLP."
            )
        else:
            candidates = [usb_dev.in_mps, usb_dev.in_mps * 2, usb_dev.in_mps * 3]
            targets = [x for x in candidates if 4 <= x <= 255]
            if not targets:
                print("  Skipping: no valid target total lengths within 4..255.")
            else:
                target_total = max(targets)  # prefer 192 for FS
                target_payload = target_total - 4
                base_hdr = b"\x01\x0a\x14\xc0"
                fill_len = max(0, target_payload - len(base_hdr))
                payload_z = base_hdr + (b"Z" * fill_len)
                frame_z = build_frame(payload_z)

                iters = 50
                print(
                    f"  Target total_len={target_total} (payload={target_payload}), iters={iters}"
                )
                for _i in range(iters):
                    usb_dev.send_data(frame_z)
                    _ = usb_dev.recv_mctp_frame(timeout=DEFAULT_TIMEOUT_MS)
                print("  ZLP stress done (stability check).")

        print("Test5: HS stability stress (bulk IN completion)")
        iters = 200
        for _i in range(iters):
            usb_dev.send_data(build_frame(b"\x01\x0a\x14\xc0HS_STRESS\x00"))
            _ = usb_dev.recv_mctp_frame(timeout=DEFAULT_TIMEOUT_MS)
        print("  HS stability stress done.")

    finally:
        usb_dev.disconnect()
