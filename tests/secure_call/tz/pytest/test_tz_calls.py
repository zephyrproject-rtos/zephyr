# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
"""
Integration test: verifies that __secure_call functions cross the TrustZone
boundary correctly on mps2-an521 (QEMU Cortex-M33).

The test spawns QEMU with two images:
  - Secure image  (secure_s): configures SAU, exposes cmse_nonsecure_entry veneers.
  - NS image      (this app): calls sc_add / sc_nop / sc_fill through veneers.

Output lines from the NS image are checked against expected values.
"""

import os
import re
import shutil
import subprocess
import time
from pathlib import Path

import pytest

TIMEOUT = 60  # seconds to wait for NS "test complete" line


def _find_qemu() -> str:
    """Return path to qemu-system-arm, or skip the test if not found."""
    # 1. Honour an explicit override.
    override = os.environ.get("QEMU_BIN_PATH")
    if override:
        candidate = Path(override) / "qemu-system-arm"
        if candidate.is_file():
            return str(candidate)

    # 2. Zephyr SDK default location.
    sdk_base = os.environ.get("ZEPHYR_SDK_INSTALL_DIR") or os.path.expanduser(
        "~/zephyr-sdk-0.17.0"
    )
    for suffix in ("", "-0.17.0", "-0.16.8"):
        candidate = Path(sdk_base + suffix) / "sysroots/x86_64-pokysdk-linux/usr/bin/qemu-system-arm"
        if candidate.is_file():
            return str(candidate)

    # 3. System PATH.
    found = shutil.which("qemu-system-arm")
    if found:
        return found

    pytest.skip("qemu-system-arm not found; set QEMU_BIN_PATH or install Zephyr SDK")


def _sysbuild_root(build_dir: Path) -> Path:
    """
    Given the twister build directory (which may point at the sysbuild root or
    a domain sub-directory), return the sysbuild root where both domain
    sub-directories live.

    Twister with sysbuild=true sets build_dir to the sysbuild root, but the
    DeviceConfig may resolve app_build_dir to the default-domain sub-dir.
    Walk up until we find the sibling 'secure_s' directory.
    """
    candidate = build_dir
    for _ in range(3):
        if (candidate / "secure_s").is_dir():
            return candidate
        candidate = candidate.parent
    return build_dir  # fallback: return as-is, caller will handle missing paths


def _read_output(proc: subprocess.Popen, timeout: float) -> str:
    """Collect stdout until 'NS: test complete' or timeout."""
    lines: list[str] = []
    deadline = time.monotonic() + timeout
    assert proc.stdout is not None
    while time.monotonic() < deadline:
        try:
            line = proc.stdout.readline()
        except Exception:
            break
        if not line:
            break
        decoded = line if isinstance(line, str) else line.decode(errors="replace")
        lines.append(decoded)
        if "NS: test complete" in decoded:
            break
    return "".join(lines)


def test_tz_secure_calls(device_object):
    """
    Verify that sc_add, sc_nop, and sc_fill produce correct results across
    the TrustZone boundary on mps2-an521 (QEMU).
    """
    qemu = _find_qemu()

    build_dir = Path(device_object.device_config.build_dir)
    sb_root = _sysbuild_root(build_dir)

    # Secure image ELF — built by the sysbuild 'secure_s' domain.
    s_elf = sb_root / "secure_s" / "zephyr" / "zephyr.elf"
    # NS image ELF — the default sysbuild domain (this application).
    # Twister sets app_build_dir to the domain's build dir when sysbuild is used.
    app_build = Path(device_object.device_config.app_build_dir or build_dir)
    ns_elf = app_build / "zephyr" / "zephyr.elf"

    if not s_elf.is_file():
        pytest.fail(f"Secure image ELF not found: {s_elf}")
    if not ns_elf.is_file():
        pytest.fail(f"NS image ELF not found: {ns_elf}")

    cmd = [
        qemu,
        "-cpu", "cortex-m33",
        "-machine", "mps2-an521",
        "-m", "16",
        "-display", "none",
        "-serial", "stdio",
        "-monitor", "none",
        "-kernel", str(s_elf),
        # Load NS image at its linked address without overriding the boot PC.
        "-device", f"loader,file={ns_elf}",
    ]

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    try:
        output = _read_output(proc, TIMEOUT)
    finally:
        proc.kill()
        proc.wait()

    assert output, "No output received from QEMU"

    # sc_add(3, 4) must return 7
    assert re.search(r"SC_ADD:\s*7\b", output), (
        f"Expected 'SC_ADD: 7' in output:\n{output}"
    )

    # sc_nop() must return 42
    assert re.search(r"SC_NOP:\s*42\b", output), (
        f"Expected 'SC_NOP: 42' in output:\n{output}"
    )

    # sc_fill must fill buf with 0,1,2,3
    assert re.search(r"SC_FILL:\s*OK\b", output), (
        f"Expected 'SC_FILL: OK' in output:\n{output}"
    )

    assert "NS: test complete" in output, (
        f"NS image did not reach 'test complete':\n{output}"
    )
