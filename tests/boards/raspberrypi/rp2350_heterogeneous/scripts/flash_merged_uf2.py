#!/usr/bin/env python3

# Copyright (c) 2026 The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Flash the merged RP2350 test UF2 without requiring an SWD probe."""

import argparse
import fcntl
import glob
import os
import shutil
import struct
import subprocess
import termios
import time
from pathlib import Path


def wait_until(predicate, timeout, message):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        value = predicate()
        if value:
            return value
        time.sleep(0.1)
    raise RuntimeError(message)


def request_bootsel():
    ports = glob.glob("/dev/serial/by-id/*RP2350_heterogeneous*")
    if not ports:
        return

    fd = os.open(ports[0], os.O_RDWR | os.O_NOCTTY)
    try:
        fcntl.ioctl(fd, termios.TIOCMBIS, struct.pack("I", termios.TIOCM_DTR))
        os.write(fd, b"b")
        termios.tcdrain(fd)
    finally:
        os.close(fd)


def rp2350_block_device():
    label = Path("/dev/disk/by-label/RP2350")
    return label.resolve() if label.exists() else None


def mountpoint(device):
    result = subprocess.run(
        ["findmnt", "-rn", "-S", str(device), "-o", "TARGET"],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.stdout.strip():
        return Path(result.stdout.strip())

    subprocess.run(["udisksctl", "mount", "-b", str(device)], check=True)
    result = subprocess.run(
        ["findmnt", "-rn", "-S", str(device), "-o", "TARGET"],
        check=True,
        capture_output=True,
        text=True,
    )
    return Path(result.stdout.strip())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--board-id")
    args = parser.parse_args()

    uf2 = args.build_dir / "rp2350_heterogeneous.uf2"
    if not uf2.is_file():
        raise FileNotFoundError(f"merged UF2 not found: {uf2}")

    request_bootsel()
    device = wait_until(rp2350_block_device, 10, "RP2350 BOOTSEL disk did not appear")
    destination = mountpoint(device) / uf2.name
    shutil.copyfile(uf2, destination)
    os.sync()


if __name__ == "__main__":
    main()
