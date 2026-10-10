#!/usr/bin/env python3
# Copyright (c) 2026 KylinSoft Corporation
# SPDX-License-Identifier: Apache-2.0
"""Launch QEMU with AAVMF and a partitioned FAT ESP containing an EFI app.

AArch64 QEMU + AAVMF does not accept the same uefi-run / OVMF path used on
x86_64. AAVMF expects a MBR-partitioned FAT32 ESP; this helper builds one
with mtools (no root required) and execs qemu-system-aarch64.

Host tools are checked here at run time (not at CMake configure) so a plain
``west build`` does not require them. Install them only when running QEMU
with CONFIG_QEMU_UEFI_BOOT.
"""

import argparse
import os
import shutil
import subprocess
import sys

# Host tools required to build the ESP image (Debian/Ubuntu package names).
#
# All of these operate only on a temporary file under the Zephyr build
# directory (esp.img next to zephyr.efi). None of them is pointed at a real
# block device such as /dev/sdX.
#
#   dd         Create a fixed-size zero-filled image file (64 MiB).
#   parted     Write an MBR partition table and a single primary partition
#              starting at 1 MiB on that image file. Invoked with ``-s``
#              (script / non-interactive) and with the image path as the
#              only device argument, so it never touches host disks.
#   mkfs.vfat  Format the partition region inside the image as FAT32 using
#              ``--offset=2048`` (1 MiB / 512-byte sectors).
#   mmd/mcopy  Create EFI/BOOT and copy BOOTAA64.EFI into the FAT filesystem
#              via mtools (``-i esp.img@@1M``), without mounting the image.
REQUIRED_TOOLS = (
    ("dd", "coreutils"),
    ("parted", "parted"),
    ("mkfs.vfat", "dosfstools"),
    ("mmd", "mtools"),
    ("mcopy", "mtools"),
)


def require_host_tools():
    """Exit with a clear message if ESP-building tools are missing."""
    missing = []
    for name, pkg in REQUIRED_TOOLS:
        if shutil.which(name) is None:
            missing.append(f"{name} (package: {pkg})")
    if missing:
        print(
            "uefi_run.py: missing host tool(s):\n  - "
            + "\n  - ".join(missing)
            + "\nInstall parted, dosfstools, and mtools.",
            file=sys.stderr,
        )
        sys.exit(1)


def run_checked(cmd):
    """Run cmd; on failure print the command and exit non-zero."""
    try:
        subprocess.run(cmd, check=True)
    except FileNotFoundError as exc:
        print(f"uefi_run.py: command not found: {cmd[0]} ({exc})", file=sys.stderr)
        sys.exit(1)
    except subprocess.CalledProcessError as exc:
        print(
            f"uefi_run.py: command failed (exit {exc.returncode}): {' '.join(cmd)}",
            file=sys.stderr,
        )
        sys.exit(exc.returncode or 1)


def prepare_esp_image(esp, efi_path):
    """Create a MBR-partitioned FAT32 ESP. Partition 1 starts at 1MiB.

    CMake supplies a regular image-file path in the build directory. parted
    is only ever invoked on that path.
    """
    if os.path.exists(esp):
        if not os.path.isfile(esp):
            raise ValueError(f"refusing to replace non-regular ESP path: {esp}")
        os.remove(esp)
    run_checked(["dd", "if=/dev/zero", f"of={esp}", "bs=1M", "count=64", "status=none"])
    # Scripted; device argument is the build-dir image file only.
    run_checked(
        ["parted", "-s", esp, "mklabel", "msdos", "mkpart", "primary", "fat32", "1MiB", "100%"]
    )
    run_checked(["mkfs.vfat", "-F", "32", "-n", "ESP", "--offset=2048", esp])
    part = f"{esp}@@1M"
    run_checked(["mmd", "-i", part, "::EFI", "::EFI/BOOT"])
    run_checked(["mcopy", "-i", part, efi_path, "::EFI/BOOT/BOOTAA64.EFI"])
    return esp


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("-b", "--bios", required=True, help="AAVMF code image")
    parser.add_argument("-q", "--qemu", required=True, help="QEMU executable")
    parser.add_argument("--vars-template", required=True, help="AAVMF variables template")
    parser.add_argument("--vars-output", required=True, help="Writable AAVMF variables image")
    parser.add_argument("--esp-image", required=True, help="Output EFI System Partition image")
    parser.add_argument("efi", help="EFI application to boot")
    parser.add_argument("qemu_args", nargs=argparse.REMAINDER, help="Arguments passed to QEMU")
    return parser.parse_args()


def main():
    args = parse_args()

    require_host_tools()

    if not os.path.isfile(args.vars_template):
        print(
            f"AAVMF vars image not found: {args.vars_template}\n"
            "Install qemu-efi-aarch64 or set AAVMF_VARS_PATH.",
            file=sys.stderr,
        )
        sys.exit(1)

    if not os.path.isfile(args.bios):
        print(f"AAVMF code image not found: {args.bios}", file=sys.stderr)
        sys.exit(1)

    if not os.path.isfile(args.efi):
        print(f"EFI application not found: {args.efi}", file=sys.stderr)
        sys.exit(1)

    shutil.copy(args.vars_template, args.vars_output)
    prepare_esp_image(args.esp_image, os.path.abspath(args.efi))

    cmd = [
        args.qemu,
        "-drive",
        f"if=pflash,format=raw,readonly=on,file={args.bios}",
        "-drive",
        f"if=pflash,format=raw,readonly=off,file={args.vars_output}",
        "-drive",
        f"if=none,format=raw,file={args.esp_image},id=espdisk",
        "-device",
        "virtio-blk-pci,drive=espdisk,bootindex=1",
    ] + args.qemu_args

    print(f"uefi_run.py: exec {' '.join(cmd)}", file=sys.stderr)
    try:
        os.execvp(args.qemu, cmd)
    except OSError as exc:
        print(f"uefi_run.py: failed to exec {args.qemu}: {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
