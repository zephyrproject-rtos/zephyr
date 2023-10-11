# Copyright (c) 2023 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

import os
import subprocess
from twister_harness import DeviceAdapter
import sys
import logging
import shlex

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "pylib", "twister"))
from twisterlib.cmakecache import CMakeCache

logger = logging.getLogger(__name__)


def test_gdbstub(dut: DeviceAdapter):
    """
    Test gdbstub feature using a gdb script. We connect to the DUT and run some
    basic gdb commands and evaluate return code to determine pass or failure.
    """
    build_dir = dut.device_config.build_dir
    cmake_cache = CMakeCache.from_file(build_dir / 'CMakeCache.txt')
    gdb = cmake_cache.get('CMAKE_GDB', None)
    assert gdb
    source_dir = cmake_cache.get('APPLICATION_SOURCE_DIR', None)
    assert source_dir
    cmd = [gdb, '-x', f'{source_dir}/run.gdbinit', f'{build_dir}/zephyr/zephyr.elf']
    logger.info(f'Test command: {shlex.join(cmd)}')
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=20)
    logger.debug('Output:\n%s' % result.stdout)
    assert result.returncode == 0
