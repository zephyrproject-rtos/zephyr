#!/usr/bin/env python3
# Copyright (c) 2026 Infineon Technologies AG
# SPDX-License-Identifier: Apache-2.0
"""Cross-platform BLE throughput client for the gatt_tput "TPUT" peripheral.

A portable, open-source replacement for Infineon's TputClient.exe, built on the
Bleak BLE library (works on Windows, macOS, and Linux). It speaks the same
custom GATT contract as the firmware, so the board needs no changes.

Directions are labelled from the *device's* point of view so the numbers line
up with the board's console:
  - TX = notifications the device sends  (client subscribes / receives)
  - RX = writes the device receives      (client sends to WriteMe)

Usage:
    pip install bleak
    python tput_client.py --mode both --duration 10 --throttle 0
"""

import argparse
import asyncio
import contextlib
import time

from bleak import BleakClient, BleakScanner

DEVICE_NAME = "TPUT"

NOTIFY_CHR = "f7b01381-91e9-4d30-9e78-84675921251e"  # device -> client (TX)
WRITEME_CHR = "d4707b26-85a3-4465-ade4-afb370cf58c7"  # client -> device (RX)
THROTTLE_CHR = "1940fd66-77b7-4e98-977b-007097c4f30c"  # KB/s target, 0 = max


class Meter:
    """Byte counters, from the device's perspective."""

    def __init__(self):
        self.tx = 0  # bytes the device notified to us
        self.rx = 0  # bytes we wrote to the device
        self.rx_ok = 0  # write calls that returned success
        self.rx_fail = 0  # write calls that raised


meter = Meter()


def on_notify(_sender, data: bytearray):
    meter.tx += len(data)


async def reporter(stop: asyncio.Event):
    start = time.monotonic()
    last = start
    prev_tx = prev_rx = 0
    while not stop.is_set():
        await asyncio.sleep(1.0)
        now = time.monotonic()
        dt = now - last
        last = now

        tx, rx = meter.tx, meter.rx
        d_tx, d_rx = tx - prev_tx, rx - prev_rx
        prev_tx, prev_rx = tx, rx

        elapsed = now - start
        print(
            f"[t={elapsed:7.3f}s dt={dt * 1000:.0f}ms] "
            f"Throughput  TX = {int(d_tx * 8 / dt / 1000)} kbps   "
            f"RX = {int(d_rx * 8 / dt / 1000)} kbps   "
            f"[total TX={tx} B  RX={rx} B]"
        )


async def writer(client: BleakClient, payload_len: int, stop: asyncio.Event, inflight: int):
    # A single sequential await under-drives the link; run several write loops
    # in parallel to keep the controller's TX buffers full.
    buf = bytes(payload_len)

    async def worker():
        while not stop.is_set():
            try:
                await client.write_gatt_char(WRITEME_CHR, buf, response=False)
                meter.rx += payload_len
                meter.rx_ok += 1
            except Exception:
                meter.rx_fail += 1
                await asyncio.sleep(0.001)

    await asyncio.gather(*(worker() for _ in range(max(1, inflight))))


async def run(args):
    print(f"Scanning for '{args.name}'...")
    device = await BleakScanner.find_device_by_name(args.name, timeout=15.0)
    if device is None:
        print("Device not found. Is it advertising as 'TPUT'?")
        return

    async with BleakClient(device) as client:
        print(f"Connected: {client.address}")
        # WinRT negotiates the MTU shortly after connect; reading it immediately
        # can return the default 23. Poll briefly until it rises.
        mtu = client.mtu_size
        for _ in range(20):
            if mtu > 23:
                break
            await asyncio.sleep(0.1)
            mtu = client.mtu_size
        payload = max(20, mtu - 3)
        print(f"ATT MTU = {mtu}, write payload = {payload}")

        # Throttle only affects the device's notify (TX) pacing; 0 = max rate.
        try:
            await client.write_gatt_char(
                THROTTLE_CHR, int(args.throttle).to_bytes(2, "little"), response=True
            )
            print(f"Throttle set to {args.throttle} KB/s")
        except Exception as exc:
            print(f"Throttle write skipped: {exc}")

        stop = asyncio.Event()
        tasks = [asyncio.create_task(reporter(stop))]

        if args.mode in ("notify", "both"):
            await client.start_notify(NOTIFY_CHR, on_notify)
            print("Notify (device TX) started")
        if args.mode in ("write", "both"):
            tasks.append(asyncio.create_task(writer(client, payload, stop, args.inflight)))
            print(f"Write (device RX) started ({args.inflight} concurrent)")

        try:
            await asyncio.sleep(args.duration)
        finally:
            stop.set()
            if args.mode in ("notify", "both"):
                with contextlib.suppress(Exception):
                    await client.stop_notify(NOTIFY_CHR)
            for task in tasks:
                task.cancel()
            await asyncio.gather(*tasks, return_exceptions=True)

        print("Done.")
        if args.mode in ("write", "both"):
            total = meter.rx_ok + meter.rx_fail
            fail_pct = (100.0 * meter.rx_fail / total) if total else 0.0
            print(
                f"Write summary: ok={meter.rx_ok} fail={meter.rx_fail} "
                f"({fail_pct:.1f}% failed), counted_tx_bytes={meter.rx}"
            )


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument(
        "--name", default=DEVICE_NAME, help="advertised device name (default: TPUT)"
    )
    parser.add_argument(
        "--mode",
        choices=["notify", "write", "both"],
        default="both",
        help="which direction(s) to exercise",
    )
    parser.add_argument("--duration", type=float, default=10.0, help="test length in seconds")
    parser.add_argument(
        "--throttle", type=int, default=0, help="device TX target in KB/s (0 = max)"
    )
    parser.add_argument(
        "--inflight",
        type=int,
        default=8,
        help="concurrent writes in flight for the RX test (default: 8)",
    )
    asyncio.run(run(parser.parse_args()))


if __name__ == "__main__":
    main()
