#!/usr/bin/env python3
# Copyright (c) 2026 The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Launch QEMU with AAVMF and a partitioned FAT ESP containing an EFI app.

AArch64 QEMU + AAVMF does not accept the same uefi-run / OVMF path used on
x86_64. AAVMF expects a MBR-partitioned FAT32 ESP; this helper builds one
with mtools (no root required) and execs qemu-system-aarch64.
"""

import os
import shutil
import subprocess
import sys

AAVMF_VARS_DEFAULT = "/usr/share/AAVMF/AAVMF_VARS.fd"

# Host tools required to build the ESP image (Debian/Ubuntu package names).
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


def prepare_esp_image(work_dir, efi_path):
    """Create a MBR-partitioned FAT32 ESP. Partition 1 starts at 1MiB."""
    esp = os.path.join(work_dir, "esp.img")
    if os.path.exists(esp):
        os.remove(esp)
    run_checked(["dd", "if=/dev/zero", f"of={esp}", "bs=1M", "count=64", "status=none"])
    run_checked(
        ["parted", "-s", esp, "mklabel", "msdos", "mkpart", "primary", "fat32", "1MiB", "100%"]
    )
    run_checked(["mkfs.vfat", "-F", "32", "-n", "ESP", "--offset=2048", esp])
    part = f"{esp}@@1M"
    run_checked(["mmd", "-i", part, "::EFI", "::EFI/BOOT"])
    run_checked(["mcopy", "-i", part, efi_path, "::EFI/BOOT/BOOTAA64.EFI"])
    return esp


def main():
    args = sys.argv[1:]
    bios = None
    qemu = None
    efi = None
    qemu_args = []
    i = 0
    while i < len(args):
        if args[i] == "-b" and i + 1 < len(args):
            bios = args[i + 1]
            i += 2
        elif args[i] == "-q" and i + 1 < len(args):
            qemu = args[i + 1]
            i += 2
        elif args[i] == "--":
            qemu_args = args[i + 1 :]
            break
        elif args[i].endswith(".efi"):
            efi = args[i]
            i += 1
        else:
            i += 1

    if efi is None:
        for j, a in enumerate(qemu_args):
            if a.endswith(".efi"):
                efi = a
                qemu_args = qemu_args[:j] + qemu_args[j + 1 :]
                break

    if not all([bios, qemu, efi]):
        print(
            "usage: uefi_run.py -b CODE.fd -q qemu-system-aarch64 APP.efi -- [qemu args]",
            file=sys.stderr,
        )
        sys.exit(2)

    require_host_tools()

    work = os.path.dirname(os.path.abspath(efi))
    vars_src = os.environ.get("AAVMF_VARS_PATH", AAVMF_VARS_DEFAULT)
    if not os.path.isfile(vars_src):
        print(
            f"AAVMF vars image not found: {vars_src}\n"
            "Install qemu-efi-aarch64 or set AAVMF_VARS_PATH.",
            file=sys.stderr,
        )
        sys.exit(1)

    if not os.path.isfile(bios):
        print(f"AAVMF code image not found: {bios}", file=sys.stderr)
        sys.exit(1)

    if not os.path.isfile(efi):
        print(f"EFI application not found: {efi}", file=sys.stderr)
        sys.exit(1)

    vars_dst = os.path.join(work, "aarch64_qemu_VARS.fd")
    shutil.copy(vars_src, vars_dst)

    esp = prepare_esp_image(work, os.path.abspath(efi))

    cmd = [
        qemu,
        "-drive",
        f"if=pflash,format=raw,readonly=on,file={bios}",
        "-drive",
        f"if=pflash,format=raw,readonly=off,file={vars_dst}",
        "-drive",
        f"if=none,format=raw,file={esp},id=espdisk",
        "-device",
        "virtio-blk-pci,drive=espdisk,bootindex=1",
    ] + qemu_args

    print(f"uefi_run.py: exec {' '.join(cmd)}", file=sys.stderr)
    try:
        os.execvp(qemu, cmd)
    except OSError as exc:
        print(f"uefi_run.py: failed to exec {qemu}: {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
