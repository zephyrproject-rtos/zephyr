# Copyright (c) 2026 Intercreate, Inc.
#
# SPDX-License-Identifier: Apache-2.0

"""
Boot a two-key MCUboot bootloader on the mps2/an385 QEMU machine and assert
which key_id validates the application.

The application is linked for MCUboot RAM_LOAD and only boots once MCUboot has
validated it and copied it out of slot0, so standard twister cannot launch it:
the bootloader and a signed slot0 image are loaded into QEMU together.

The application is signed at build time with the bootloader's first (development)
key. To exercise the second embedded key, --resign-key re-signs the built image
with a different key before loading it -- reusing the build's own imgtool
invocation, as a production custodian would sign the release binary out-of-band.
"""

from __future__ import annotations

import logging
import os
import re
import selectors
import shlex
import subprocess
import sys
import time
from collections.abc import Sequence
from pathlib import Path
from typing import Final, NamedTuple

import pytest
import yaml  # type: ignore[import-untyped]
from twister_harness import DeviceAdapter  # type: ignore[import-not-found]

ZEPHYR_BASE: Final = os.environ["ZEPHYR_BASE"]
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "pylib", "twister"))
from twisterlib.cmakecache import CMakeCache  # type: ignore[import-not-found] # noqa: E402

logger: Final = logging.getLogger(__name__)


class DomainDirs(NamedTuple):
    """Build directories of the two sysbuild domains this test launches."""

    mcuboot: Path
    app: Path


def domain_dirs(build_dir: Path) -> DomainDirs:
    domains = yaml.safe_load((build_dir / "domains.yaml").read_text())
    by_name: Final[dict[str, Path]] = {
        domain["name"]: Path(domain["build_dir"]) for domain in domains["domains"]
    }
    mcuboot: Final = by_name.pop("mcuboot")
    if len(by_name) != 1:
        raise RuntimeError(
            f"expected one application domain besides mcuboot, got {sorted(by_name)}"
        )
    (app,) = by_name.values()
    return DomainDirs(mcuboot=mcuboot, app=app)


def _imgtool_sign_argv(app_build: Path) -> list[str]:
    """The imgtool 'sign' command the build used, parsed from build.ninja so a
    re-sign reuses the exact header, slot, alignment and version parameters."""
    ninja: Final = (app_build / "build.ninja").read_text()
    for command in (
        segment.strip()
        for line in ninja.splitlines()
        if "imgtool" in line
        for segment in line.split("&&")
    ):
        if "imgtool" in command and " sign " in command:
            return shlex.split(command)
    raise RuntimeError(f"no imgtool sign command found in {app_build / 'build.ninja'}")


def resign_application(app_build: Path, key_name: str) -> Path:
    """Re-sign the built application with a different key and return the new
    image. Models a production custodian signing the release binary out-of-band:
    the build's imgtool command is reused verbatim except for the key (resolved
    against the build's signing-key directory) and the output path."""
    argv: Final = _imgtool_sign_argv(app_build)
    key: Final = argv.index("--key")
    signing_key: Final = Path(argv[key + 1]).parent / key_name
    output: Final = app_build / "zephyr" / "zephyr.signed.resigned.bin"
    subprocess.run(
        [*argv[: key + 1], str(signing_key), *argv[key + 2 : -1], str(output)],
        check=True,
    )
    return output


def qemu_command(dirs: DomainDirs, app_image: Path) -> tuple[str, ...]:
    qemu: Final = CMakeCache.from_file(dirs.mcuboot / "CMakeCache.txt").get("QEMU")
    assert qemu, "QEMU binary not found in MCUboot CMakeCache"
    return (
        qemu,
        "-cpu",
        "cortex-m3",
        "-machine",
        "mps2-an385",
        "-nographic",
        "-device",
        f"loader,file={dirs.mcuboot / 'zephyr' / 'zephyr.hex'}",
        "-device",
        f"loader,file={app_image},addr=0x20050000",
    )


def run_qemu(command: Sequence[str], patterns: Sequence[re.Pattern[str]], timeout: float) -> str:
    """Run QEMU until every pattern has matched or the timeout elapses, then
    terminate it and return the console output."""
    output = ""
    with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as proc:
        assert proc.stdout is not None
        fd: Final = proc.stdout.fileno()
        selector = selectors.DefaultSelector()
        selector.register(fd, selectors.EVENT_READ)
        deadline: Final = time.monotonic() + timeout
        try:
            while not all(pattern.search(output) for pattern in patterns):
                remaining = deadline - time.monotonic()
                if remaining <= 0 or not selector.select(remaining):
                    break
                chunk = os.read(fd, 4096)
                if not chunk:
                    break
                output += chunk.decode(errors="replace")
        finally:
            selector.close()
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
    return output


def test_multiple_keys(unlaunched_dut: DeviceAdapter, request: pytest.FixtureRequest) -> None:
    expected_key_id: Final[int] = request.config.getoption("--expected-key-id")
    resign_key: Final[str] = request.config.getoption("--resign-key")

    dirs: Final = domain_dirs(Path(unlaunched_dut.device_config.build_dir))
    app_image: Final = (
        resign_application(dirs.app, resign_key)
        if resign_key
        else dirs.app / "zephyr" / "zephyr.signed.bin"
    )
    command: Final = qemu_command(dirs, app_image)
    logger.info("Launching QEMU: %s", " ".join(command))

    key_id: Final = re.compile(rf"bootutil_verify_sig: ED25519 key_id {expected_key_id}\b")
    banner: Final = re.compile(r"Hello mcuboot multiple keys!")
    output: Final = run_qemu(command, (key_id, banner), timeout=60.0)
    logger.info("QEMU console:\n%s", output)

    assert key_id.search(output), (
        f"MCUboot did not validate the application against key_id {expected_key_id}"
    )
    assert banner.search(output), "application did not boot from the primary slot"
