#!/usr/bin/env python3
# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

"""
IPED provisioning and test runner for frdm_rw612 IPED sample.

Automates the full cycle:
  1. Build the test with west
  2. Extract image components (FCB, image_version, headerless) from zephyr.bin
  3. Pad headerless image to fill the IPED region
  4. Provision IPED via blhost (board must be in ISP mode)
  5. Reset board and capture serial output
  6. Parse and report test results

Also supports disabling IPED to restore the board to normal operation.

Prerequisites:
  - pip install spsdk   (provides blhost and nxpimage)
  - west and Zephyr SDK installed
  - Board connected via USB

Usage:
  # Full cycle: build + provision + test
  python3 provision_and_test.py

  # Provision only (already built)
  python3 provision_and_test.py --skip-build

  # Just capture serial output (already provisioned)
  python3 provision_and_test.py --skip-build --skip-provision

  # Disable IPED and restore board to normal operation
  python3 provision_and_test.py --disable-iped
"""

import argparse
import os
import re
import subprocess
import sys
import time
from pathlib import Path

# --------------------------------------------------------------------------- #
# Defaults — override with CLI args
# --------------------------------------------------------------------------- #
DEFAULT_BLHOST_PORT = "/dev/cu.usbmodem0010633821641"
DEFAULT_CAPTURE_PORT = "/dev/tty.usbmodem0010633821641"
DEFAULT_BAUD = 115200
DEFAULT_BOARD = "frdm_rw612"

# IPED region 0 (code, GCM mode): must match the macros in main.c
# AHB addresses = flash offset + 0x08000000
IPED_FLASH_START = 0x1000
IPED_FLASH_END = 0x18000
IPED_AHB_START = 0x08000000 + IPED_FLASH_START  # 0x08001000
IPED_AHB_END = 0x08000000 + IPED_FLASH_END  # 0x08018000
IPED_ERASE_SIZE = IPED_FLASH_END - IPED_FLASH_START  # 0x17000

# IPED region 1 (test area, CTR mode): for write+read roundtrip tests.
# Located in the slot1 partition area (not used without MCUboot).
IPED2_FLASH_START = 0x320000
IPED2_FLASH_END = 0x324000
IPED2_AHB_START = 0x08000000 + IPED2_FLASH_START  # 0x08320000
IPED2_AHB_END = 0x08000000 + IPED2_FLASH_END  # 0x08324000
IPED2_ERASE_SIZE = IPED2_FLASH_END - IPED2_FLASH_START  # 0x4000

# Image layout offsets inside zephyr.bin (bare Zephyr, not TFM)
FCB_OFFSET = 0x400
FCB_SIZE = 0x200
IMG_VER_OFFSET = 0x600
IMG_VER_SIZE = 0x200
HEADERLESS_OFFSET = 0x1000


def run_cmd(cmd, check=True, capture=True, timeout=30, cwd=None):
    """Run a command (list form only), print it, return stdout."""
    cmd = [str(c) for c in cmd]
    print(f"  $ {' '.join(cmd)}")
    result = subprocess.run(
        cmd,
        capture_output=capture,
        text=True,
        timeout=timeout,
        cwd=cwd,
    )
    if capture and result.stdout:
        for line in result.stdout.strip().split("\n")[:5]:
            print(f"    {line}")
        if result.stdout.strip().count("\n") > 5:
            print("    ...")
    if check and result.returncode != 0:
        stderr = result.stderr.strip() if capture else ""
        print(f"  ERROR (rc={result.returncode}): {stderr}")
        sys.exit(1)
    return result


def blhost_cmd(port, baud, *args):
    """Run a blhost command."""
    cmd = ["blhost", "-p", f"{port},{baud}", "--"] + list(args)
    return run_cmd(cmd, timeout=60)


# --------------------------------------------------------------------------- #
# Step 1: Build
# --------------------------------------------------------------------------- #
def step_build(zephyr_base, build_dir, board):
    print("\n" + "=" * 60)
    print("Step 1: Build sample")
    print("=" * 60)

    sample_dir = os.path.join(zephyr_base, "samples", "boards", "nxp", "frdm_rw612", "iped")

    # Find the west workspace root (parent of zephyr_base that contains .west/)
    workspace = zephyr_base
    for _ in range(5):
        if os.path.isdir(os.path.join(workspace, ".west")):
            break
        workspace = os.path.dirname(workspace)
    else:
        print("WARNING: could not find .west/ directory, using zephyr_base parent")
        workspace = os.path.dirname(zephyr_base)

    cmd = [
        "west",
        "build",
        "-p",
        "always",
        "-b",
        board,
        sample_dir,
        f"-DZEPHYR_BASE={zephyr_base}",
    ]
    if build_dir:
        cmd += ["-d", build_dir]
    else:
        build_dir = os.path.join(workspace, "build")

    run_cmd(cmd, timeout=300, cwd=workspace)

    zephyr_bin = os.path.join(build_dir, "zephyr", "zephyr.bin")
    if not os.path.isfile(zephyr_bin):
        print(f"ERROR: {zephyr_bin} not found after build")
        sys.exit(1)

    print(f"  Built: {zephyr_bin} ({os.path.getsize(zephyr_bin)} bytes)")
    return zephyr_bin


# --------------------------------------------------------------------------- #
# Step 2: Extract image components
# --------------------------------------------------------------------------- #
def step_extract(zephyr_bin, output_dir):
    print("\n" + "=" * 60)
    print("Step 2: Extract image components")
    print("=" * 60)

    os.makedirs(output_dir, exist_ok=True)

    fcb_bin = os.path.join(output_dir, "fcb.bin")
    img_ver_bin = os.path.join(output_dir, "image_version.bin")
    headerless_bin = os.path.join(output_dir, "headerless.bin")
    padded_bin = os.path.join(output_dir, "headerless_padded.bin")

    # Extract using nxpimage
    run_cmd(
        [
            "nxpimage",
            "utils",
            "binary-image",
            "extract",
            "-b",
            zephyr_bin,
            "-a",
            str(FCB_OFFSET),
            "-s",
            str(FCB_SIZE),
            "-o",
            fcb_bin,
        ]
    )
    run_cmd(
        [
            "nxpimage",
            "utils",
            "binary-image",
            "extract",
            "-b",
            zephyr_bin,
            "-a",
            str(IMG_VER_OFFSET),
            "-s",
            str(IMG_VER_SIZE),
            "-o",
            img_ver_bin,
        ]
    )
    run_cmd(
        [
            "nxpimage",
            "utils",
            "binary-image",
            "extract",
            "-b",
            zephyr_bin,
            "-a",
            str(HEADERLESS_OFFSET),
            "-s",
            "0",
            "-o",
            headerless_bin,
        ]
    )

    # Pad headerless image to fill IPED region exactly
    headerless_size = os.path.getsize(headerless_bin)
    padded_size = IPED_ERASE_SIZE  # 0x17000

    if headerless_size > padded_size:
        print(f"ERROR: image ({headerless_size} bytes) exceeds IPED region ({padded_size} bytes)")
        print("Increase IPED_FLASH_END or reduce the image size.")
        sys.exit(1)

    with open(headerless_bin, "rb") as f:
        data = f.read()

    pad_bytes = padded_size - headerless_size
    print(f"  Image: {headerless_size} bytes, padding {pad_bytes} bytes to {padded_size}")

    with open(padded_bin, "wb") as f:
        f.write(data)
        f.write(b"\x00" * pad_bytes)

    print(f"  FCB:        {fcb_bin} ({os.path.getsize(fcb_bin)} bytes)")
    print(f"  ImgVer:     {img_ver_bin} ({os.path.getsize(img_ver_bin)} bytes)")
    print(f"  Headerless: {headerless_bin} ({headerless_size} bytes)")
    print(f"  Padded:     {padded_bin} ({os.path.getsize(padded_bin)} bytes)")

    return fcb_bin, img_ver_bin, padded_bin


# --------------------------------------------------------------------------- #
# Step 3: Provision IPED via blhost
# --------------------------------------------------------------------------- #
def step_provision(port, baud, fcb_bin, img_ver_bin, padded_bin):
    print("\n" + "=" * 60)
    print("Step 3: Provision IPED via blhost")
    print("=" * 60)
    print(f"  Port: {port}")
    print(f"  IPED region: 0x{IPED_AHB_START:08X} - 0x{IPED_AHB_END:08X}")

    # Ping ROM bootloader
    print("\n--- Ping ROM bootloader ---")
    blhost_cmd(port, baud, "get-property", "1")

    # Step 3a: Write FCB to SRAM and configure FlexSPI
    print("\n--- Configure FlexSPI ---")
    blhost_cmd(port, baud, "write-memory", "0x2000f000", fcb_bin)
    blhost_cmd(port, baud, "configure-memory", "9", "0x2000f000")

    # Step 3b: Write FCB and image version to flash
    print("\n--- Write FCB + image version to flash ---")
    blhost_cmd(port, baud, "flash-erase-region", "0x08000000", "0x1000")
    blhost_cmd(port, baud, "write-memory", "0x08000400", fcb_bin)
    blhost_cmd(port, baud, "write-memory", "0x08000600", img_ver_bin)

    # Step 3c: Configure IPED region descriptors
    #   Uses ROM default fuse key (zero-derived) — test only, no real security.
    #   Production deployments must program device-specific OTP key fuses.
    #   Region 0: GCM mode (code, XIP) — GCM bit is in IPEDCTX0START[0]
    #   Region 1: CTR mode (test area, non-XIP) — GCM bit = 0
    print("\n--- Configure IPED regions ---")
    print(f"  Region 0 (GCM): 0x{IPED_AHB_START:08X} - 0x{IPED_AHB_END:08X}")
    print(f"  Region 1 (CTR): 0x{IPED2_AHB_START:08X} - 0x{IPED2_AHB_END:08X}")
    fill_cmds = [
        ("0x2000F000", "0xA0000001"),  # Control word: configure mode, GCM
        ("0x2000F004", "0x0"),
        ("0x2000F008", f"0x{IPED_AHB_START:08X}"),  # Region 0 start
        ("0x2000F00C", f"0x{IPED_AHB_END:08X}"),  # Region 0 end
        ("0x2000F010", f"0x{IPED2_AHB_START:08X}"),  # Region 1 start (CTR, no GCM bit)
        ("0x2000F014", f"0x{IPED2_AHB_END:08X}"),  # Region 1 end
        ("0x2000F018", "0x0"),  # Region 2 (unused)
        ("0x2000F01C", "0x0"),
        ("0x2000F020", "0x0"),  # Region 3 (unused)
        ("0x2000F024", "0x0"),
        ("0x2000F028", "0x0"),  # Padding
        ("0x2000F02C", "0x0"),
        ("0x2000F030", "0x0"),
        ("0x2000F034", "0x0"),
    ]
    for addr, val in fill_cmds:
        blhost_cmd(port, baud, "fill-memory", addr, "4", val)
    blhost_cmd(port, baud, "configure-memory", "9", "0x2000f000")

    # Step 3d: Enable IPED
    print("\n--- Enable IPED ---")
    blhost_cmd(port, baud, "fill-memory", "0x2000F000", "4", "0xAA000001")
    blhost_cmd(port, baud, "fill-memory", "0x2000F004", "4", "0x08000000")
    blhost_cmd(port, baud, "configure-memory", "9", "0x2000f000")

    # Step 3e: Erase IPED regions and write encrypted image
    print("\n--- Flash encrypted image (region 0) ---")
    blhost_cmd(
        port,
        baud,
        "flash-erase-region",
        f"0x{IPED_AHB_START:08X}",
        f"0x{IPED_ERASE_SIZE:X}",
    )
    blhost_cmd(port, baud, "write-memory", f"0x{IPED_AHB_START:08X}", padded_bin)

    # Erase region 1 (test area) so it starts in a known state
    print("\n--- Erase test region (region 1) ---")
    blhost_cmd(
        port,
        baud,
        "flash-erase-region",
        f"0x{IPED2_AHB_START:08X}",
        f"0x{IPED2_ERASE_SIZE:X}",
    )

    print("\n  IPED provisioning complete.")
    print("  Press RESET on the board to boot the encrypted image.")


# --------------------------------------------------------------------------- #
# Step 4: Capture serial output and parse results
# --------------------------------------------------------------------------- #
def step_capture(port, baud, timeout_sec=15):
    print("\n" + "=" * 60)
    print("Step 4: Capture serial output")
    print("=" * 60)

    try:
        import serial
    except ImportError:
        print("ERROR: pyserial not installed. Install with: pip install pyserial")
        print("Alternatively, open a serial terminal manually:")
        print(f"  screen {port} {baud}")
        return None

    lines = []
    try:
        with serial.Serial(port, baud, timeout=1) as ser:
            print(f"  Listening on {port} @ {baud}...")
            print("  (Press RESET on the board if not already done)\n")

            start = time.time()
            while (time.time() - start) < timeout_sec:
                line = ser.readline().decode("utf-8", errors="replace").rstrip()
                if line:
                    print(f"  | {line}")
                    lines.append(line)
                    if "ALL TESTS PASSED" in line or "SOME TESTS FAILED" in line:
                        # Capture a few more lines then stop
                        time.sleep(0.5)
                        while ser.in_waiting:
                            extra = ser.readline().decode("utf-8", errors="replace").rstrip()
                            if extra:
                                print(f"  | {extra}")
                                lines.append(extra)
                        break
    except serial.SerialException as e:
        print(f"  Serial error: {e}")
        print(f"  Try closing other programs using {port}")
        return None

    return lines


def parse_results(lines):
    """Parse test output and print summary."""
    if not lines:
        print("\n  No output captured.")
        return

    print("\n" + "=" * 60)
    print("Test Results Summary")
    print("=" * 60)

    passes = []
    fails = []
    skips = []

    for line in lines:
        m = re.search(r"\s(PASS|FAIL|SKIP)\s+-\s+(.+?)(?:\s+in\s+\d+)", line)
        if m:
            result, name = m.group(1), m.group(2).strip()
            if result == "PASS":
                passes.append(name)
            elif result == "FAIL":
                fails.append(name)
            else:
                skips.append(name)

    for line in lines:
        m = re.search(r"pass\s*=\s*(\d+),\s*fail\s*=\s*(\d+),\s*skip\s*=\s*(\d+)", line)
        if m:
            print(f"  Total: {m.group(1)} passed, {m.group(2)} failed, {m.group(3)} skipped")
            break

    if fails:
        print("\n  FAILURES:")
        for f in fails:
            print(f"    ✗ {f}")

    if skips:
        print("\n  SKIPPED:")
        for s in skips:
            print(f"    ○ {s}")

    if not fails:
        print("\n  ✓ All tests passed!")
    else:
        print(f"\n  ✗ {len(fails)} test(s) failed")
        sys.exit(1)


# --------------------------------------------------------------------------- #
# Disable IPED — restore board to normal operation
# --------------------------------------------------------------------------- #
def step_disable_iped(port, baud):
    print("\n" + "=" * 60)
    print("Disable IPED")
    print("=" * 60)

    blhost_cmd(port, baud, "get-property", "1")

    print("\n--- Clearing IPED configuration ---")
    blhost_cmd(port, baud, "fill-memory", "0x2000F000", "4", "0xA0000000")
    blhost_cmd(port, baud, "fill-memory", "0x2000F004", "4", "0x08000000")
    blhost_cmd(port, baud, "configure-memory", "9", "0x2000f000")

    print("\n  IPED disabled. The board will boot normally after RESET.")
    print("  You can now flash any image with 'west flash' or JLink.")


# --------------------------------------------------------------------------- #
# Main
# --------------------------------------------------------------------------- #
def main():
    global IPED_FLASH_START, IPED_FLASH_END, IPED_AHB_START, IPED_AHB_END, IPED_ERASE_SIZE

    parser = argparse.ArgumentParser(
        description="IPED provisioning and test runner for frdm_rw612",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
        allow_abbrev=False,
    )
    parser.add_argument(
        "--port",
        default=DEFAULT_BLHOST_PORT,
        help=f"blhost serial port (default: {DEFAULT_BLHOST_PORT})",
    )
    parser.add_argument(
        "--capture-port",
        default=DEFAULT_CAPTURE_PORT,
        help=f"Serial capture port (default: {DEFAULT_CAPTURE_PORT}). "
        "On macOS, console output uses /dev/tty.* while blhost uses /dev/cu.*",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUD,
        help=f"Baud rate (default: {DEFAULT_BAUD})",
    )
    parser.add_argument(
        "--board",
        default=DEFAULT_BOARD,
        help=f"Board target (default: {DEFAULT_BOARD})",
    )
    parser.add_argument(
        "--zephyr-base",
        help="Path to Zephyr base (default: auto-detect from script location)",
    )
    parser.add_argument(
        "--build-dir",
        help="Build output directory (default: <workspace>/build)",
    )
    parser.add_argument(
        "--output-dir",
        help="Directory for extracted image components (default: <build-dir>/iped_provision)",
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Skip the build step (use existing build)",
    )
    parser.add_argument(
        "--skip-provision",
        action="store_true",
        help="Skip IPED provisioning (board already provisioned)",
    )
    parser.add_argument(
        "--skip-capture",
        action="store_true",
        help="Skip serial capture (provision only)",
    )
    parser.add_argument(
        "--capture-timeout",
        type=int,
        default=15,
        help="Serial capture timeout in seconds (default: 15)",
    )
    parser.add_argument(
        "--iped-start",
        type=lambda x: int(x, 0),
        default=IPED_FLASH_START,
        help=f"IPED region start flash offset (default: 0x{IPED_FLASH_START:X})",
    )
    parser.add_argument(
        "--iped-end",
        type=lambda x: int(x, 0),
        default=IPED_FLASH_END,
        help=f"IPED region end flash offset (default: 0x{IPED_FLASH_END:X})",
    )
    parser.add_argument(
        "--disable-iped",
        action="store_true",
        help="Disable IPED and restore the board to normal operation. "
        "Board must be in ISP mode. This clears the IPED config from "
        "flash IFR so subsequent images boot without IPED.",
    )
    parser.add_argument(
        "--no-prompt",
        action="store_true",
        help="Non-interactive mode: use countdown timers instead of input() prompts",
    )
    parser.add_argument(
        "--prompt-delay",
        type=int,
        default=10,
        help="Seconds to wait at each prompt in --no-prompt mode (default: 10)",
    )

    args = parser.parse_args()

    # Update globals from args
    IPED_FLASH_START = args.iped_start
    IPED_FLASH_END = args.iped_end
    IPED_AHB_START = 0x08000000 + IPED_FLASH_START
    IPED_AHB_END = 0x08000000 + IPED_FLASH_END
    IPED_ERASE_SIZE = IPED_FLASH_END - IPED_FLASH_START

    def wait_for_user(msg):
        if args.no_prompt:
            for i in range(args.prompt_delay, 0, -1):
                print(f"\r  Waiting {i}s... {msg}", end="", flush=True)
                time.sleep(1)
            print()
        else:
            input(msg)

    # Handle --disable-iped early (no build/extract needed)
    if args.disable_iped:
        print("\n" + "*" * 60)
        print("* Put the board in ISP mode:                               *")
        print("*   1. Hold ISP button                                     *")
        print("*   2. Press RESET                                         *")
        print("*   3. Release ISP button                                  *")
        print("*" * 60)
        wait_for_user("\nPress Enter when the board is in ISP mode...")
        step_disable_iped(args.port, args.baud)
        return

    # Resolve paths — normalize to absolute to prevent path traversal
    script_dir = Path(__file__).resolve().parent
    if args.zephyr_base:
        zephyr_base = str(Path(args.zephyr_base).resolve())
    else:
        # Script is at samples/boards/nxp/frdm_rw612/iped/scripts/
        zephyr_base = str(script_dir.parents[5])

    if args.build_dir:
        build_dir = str(Path(args.build_dir).resolve())
    else:
        # Use workspace/build (parent of zephyr_base)
        workspace = zephyr_base
        for _ in range(5):
            if os.path.isdir(os.path.join(workspace, ".west")):
                break
            workspace = os.path.dirname(workspace)
        build_dir = os.path.join(workspace, "build")

    if args.output_dir:
        output_dir = str(Path(args.output_dir).resolve())
    else:
        output_dir = os.path.join(build_dir, "iped_provision")

    print("IPED Flash Test Runner")
    print(f"  Zephyr base:  {zephyr_base}")
    print(f"  Build dir:    {build_dir}")
    print(f"  Output dir:   {output_dir}")
    print(f"  blhost port:  {args.port}")
    print(f"  Capture port: {args.capture_port}")
    print(
        f"  IPED region:  0x{IPED_FLASH_START:05X} - 0x{IPED_FLASH_END:05X} "
        f"({IPED_ERASE_SIZE} bytes)"
    )

    # Step 1: Build
    if args.skip_build:
        zephyr_bin = os.path.join(build_dir, "zephyr", "zephyr.bin")
        if not os.path.isfile(zephyr_bin):
            print(f"ERROR: --skip-build but {zephyr_bin} not found")
            sys.exit(1)
        print(f"\n  Using existing build: {zephyr_bin}")
    else:
        zephyr_bin = step_build(zephyr_base, build_dir, args.board)

    # Step 2: Extract
    fcb_bin, img_ver_bin, padded_bin = step_extract(zephyr_bin, output_dir)

    # Step 3: Provision
    if args.skip_provision:
        print("\n  Skipping provisioning (--skip-provision)")
    else:
        print("\n" + "*" * 60)
        print("* Put the board in ISP mode:                               *")
        print("*   1. Hold ISP button                                     *")
        print("*   2. Press RESET                                         *")
        print("*   3. Release ISP button                                  *")
        print("* Close any serial terminal first!                         *")
        print("*" * 60)
        wait_for_user("\nPress Enter when the board is in ISP mode...")

        step_provision(args.port, args.baud, fcb_bin, img_ver_bin, padded_bin)

        if not args.skip_capture:
            print("\n" + "*" * 60)
            print("* Press RESET on the board to boot the encrypted image.    *")
            print("*" * 60)
            wait_for_user("\nPress Enter after pressing RESET...")

    # Step 4: Capture
    if args.skip_capture:
        print("\n  Skipping serial capture (--skip-capture)")
    else:
        lines = step_capture(args.capture_port, args.baud, args.capture_timeout)
        parse_results(lines)


if __name__ == "__main__":
    main()
