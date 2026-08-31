#!/usr/bin/env python3

# Copyright (c) 2026 The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Discover one Pico 2, create its Twister map, and run the hardware test."""

import glob
import os
import subprocess
import sys
from pathlib import Path

PRODUCT = "RP2350 heterogeneous test"
PLATFORM = "rpi_pico2/rp2350a/m33"


def board_serial():
    ports = glob.glob("/dev/serial/by-id/*RP2350_heterogeneous*")
    if ports:
        name = Path(ports[0]).name
        return name.rsplit("_", 1)[1].removesuffix("-if00")

    for device in Path("/sys/bus/usb/devices").glob("*"):
        try:
            if (device / "idVendor").read_text().strip() == "2e8a" and (
                device / "idProduct"
            ).read_text().strip() == "000f":
                return (device / "serial").read_text().strip()
        except FileNotFoundError:
            continue

    raise RuntimeError("no RP2350 heterogeneous firmware or BOOTSEL device found")


def main():
    test_dir = Path(__file__).resolve().parent.parent
    zephyr_base = test_dir.parents[3]
    output = zephyr_base / "build" / "twister-rp2350-heterogeneous"
    output.mkdir(parents=True, exist_ok=True)

    serial = board_serial()
    serial_path = f"/dev/serial/by-id/usb-Zephyr_Project_RP2350_heterogeneous_test_{serial}-if00"
    hardware_map = zephyr_base / "build" / "rp2350-pico2.yml"
    hardware_map.write_text(
        "- connected: true\n"
        f"  id: {serial}\n"
        f"  platform: {PLATFORM}\n"
        f"  product: {PRODUCT}\n"
        "  runner: uf2\n"
        f"  serial: {serial_path}\n"
        "  serial_baud: 115200\n"
        "  flash_before: true\n"
    )

    flash_script = test_dir / "scripts" / "flash_merged_uf2.py"
    command = [
        sys.executable,
        str(zephyr_base / "scripts" / "twister"),
        "-p",
        PLATFORM,
        "-T",
        str(test_dir),
        "--device-testing",
        "--hardware-map",
        str(hardware_map),
        "--flash-command",
        str(flash_script),
        "--flash-before",
        "--inline-logs",
        "-O",
        str(output),
    ]
    env = os.environ.copy()
    env.setdefault("ZEPHYR_BASE", str(zephyr_base))
    subprocess.run(command, cwd=zephyr_base, env=env, check=True)


if __name__ == "__main__":
    main()
