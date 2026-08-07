# Copyright (c) 2026 The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

import os
import re
import selectors
import subprocess
import sys
import time
from pathlib import Path

import yaml
from twister_harness import DeviceAdapter

sys.path.insert(0, os.path.join(os.environ["ZEPHYR_BASE"], "scripts", "pylib", "twister"))
from twisterlib.cmakecache import CMakeCache  # noqa: E402


def test_ram_load_split(unlaunched_dut: DeviceAdapter) -> None:
    build_dir = Path(unlaunched_dut.device_config.build_dir)
    domains = yaml.safe_load((build_dir / "domains.yaml").read_text())
    domain_dirs = {domain["name"]: Path(domain["build_dir"]) for domain in domains["domains"]}
    app_dir = next(path for name, path in domain_dirs.items() if name != "mcuboot")
    mcuboot_dir = domain_dirs["mcuboot"]

    qemu = CMakeCache.from_file(mcuboot_dir / "CMakeCache.txt").get("QEMU")
    assert qemu is not None
    command = (
        qemu,
        "-cpu",
        "cortex-m3",
        "-machine",
        "mps2-an385",
        "-nographic",
        "-device",
        f"loader,file={mcuboot_dir / 'zephyr' / 'zephyr.hex'}",
        "-device",
        f"loader,file={app_dir / 'zephyr' / 'zephyr.signed.bin'},addr=0x20050000",
    )
    expected = re.compile(r"PASS: split RAM-load initialized data")
    output = ""

    with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as proc:
        assert proc.stdout is not None
        selector = selectors.DefaultSelector()
        selector.register(proc.stdout.fileno(), selectors.EVENT_READ)
        deadline = time.monotonic() + 60.0
        try:
            while not expected.search(output) and time.monotonic() < deadline:
                if not selector.select(deadline - time.monotonic()):
                    break
                chunk = os.read(proc.stdout.fileno(), 4096)
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

    assert expected.search(output), f"application did not initialize split data:\n{output}"
