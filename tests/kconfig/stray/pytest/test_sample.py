# Copyright (c) 2020 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import pytest
from twister_harness import DeviceAdapter

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "pylib", "twister"))
from twisterlib.cmakecache import CMakeCache


def test_case(dut: DeviceAdapter, test_kconfig):
    build_dir = dut.device_config.build_dir
    cmake_cache = CMakeCache.from_file(os.path.join(build_dir, 'CMakeCache.txt'))
    source_dir = cmake_cache.get('APPLICATION_SOURCE_DIR', None)
    assert source_dir
    expected = []
    enforced = [
        "CONFIG_COMPILER_WARNINGS_AS_ERRORS=y\n",
    ]

    with open(os.path.join(source_dir, test_kconfig), "r") as f:
        for line in f:
            if line.startswith("CONFIG_"):
                expected.append(line)
    with open(os.path.join(build_dir, "zephyr", ".config"), "r") as f:
        for line in f:
            if line.startswith("CONFIG_") and line not in expected and line not in enforced:
                assert False, f"Stray Kconfig option found: {line}"


if __name__ == "__main__":
    pytest.main()
