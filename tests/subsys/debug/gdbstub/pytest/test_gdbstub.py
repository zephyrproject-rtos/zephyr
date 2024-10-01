# Copyright (c) 2023 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

import os
import subprocess
import sys
import logging
import shlex
import re
import pytest
from twister_harness import DeviceAdapter

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "pylib", "twister"))
from twisterlib.cmakecache import CMakeCache

logger = logging.getLogger(__name__)

@pytest.fixture()
def gdb_process(dut: DeviceAdapter, gdb_script, gdb_timeout, gdb_target_remote) -> subprocess.CompletedProcess:
    build_dir = dut.device_config.build_dir
    cmake_cache = CMakeCache.from_file(os.path.join(build_dir, 'CMakeCache.txt'))
    gdb_exec = cmake_cache.get('CMAKE_GDB', None)
    assert gdb_exec
    source_dir = cmake_cache.get('APPLICATION_SOURCE_DIR', None)
    assert source_dir
    build_image = cmake_cache.get('BYPRODUCT_KERNEL_ELF_NAME', None)
    assert build_image
    gdb_log_file = os.path.join(build_dir, 'gdb.log')
    cmd = [gdb_exec, '-batch',
           '-ex', f'set pagination off',
           '-ex', f'set trace-commands on',
           '-ex', f'set logging file {gdb_log_file}',
           '-ex', f'set logging enabled on',
           '-ex', f'target remote {gdb_target_remote}',
           '-x', f'{source_dir}/{gdb_script}', build_image]
    logger.info(f'Run GDB: {shlex.join(cmd)}')
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=gdb_timeout)
    logger.info(f'GDB ends rc={result.returncode}')
    return result
#

@pytest.fixture(scope="module")
def expected_app():
    return [
    re.compile(r"Booting from ROM"),
    re.compile(r"Booting Zephyr OS build"),
    re.compile(r"main\(\):enter"),
    ]

@pytest.fixture(scope="module")
def expected_gdb():
    return [
    re.compile(r'Breakpoint 1 at 0x'),
    re.compile(r'Breakpoint 2 at 0x'),
    re.compile(r'Breakpoint 1, test '),
    re.compile(r'Breakpoint 2, main '),
    re.compile(r'GDB:PASSED'),
    re.compile(r'Breakpoint 3, k_thread_abort '),
    re.compile(r'2 .* breakpoint .* in main '),
    ]

@pytest.fixture(scope="module")
def unexpected_gdb():
    return [
    re.compile(r'breakpoint .* in test '),
    re.compile(r'breakpoint .* in k_thread_abort '),
    ]

@pytest.fixture(scope="module")
def expected_gdb_detach():
    return [
    re.compile(r'Inferior.*will be killed'),  # Zephyr gdbstub
    re.compile(r'Inferior.*detached')  # QEMU gdbstub
    ]


def test_gdbstub(dut: DeviceAdapter, gdb_process, expected_app, expected_gdb, expected_gdb_detach, unexpected_gdb):
    """
    Test gdbstub feature using a GDB script. We connect to the DUT, run the
    GDB script then evaluate return code and expected patterns at the GDB
    and Test Applicaiton outputs.
    """
    logger.debug(f"GDB output:\n{gdb_process.stdout}\n")
    assert gdb_process.returncode == 0
    assert all([ex_re.search(gdb_process.stdout, re.MULTILINE) for ex_re in expected_gdb]), 'No expected GDB output'
    assert not any([ex_re.search(gdb_process.stdout, re.MULTILINE) for ex_re in unexpected_gdb]), 'Unexpected GDB output'
    assert any([ex_re.search(gdb_process.stdout, re.MULTILINE) for ex_re in expected_gdb_detach]), 'No expected GDB quit'
    app_output = '\n'.join(dut.readlines(print_output = False))
    logger.debug(f"App output:\n{app_output}\n")
    assert all([ex_re.search(app_output, re.MULTILINE) for ex_re in expected_app]), 'No expected Application output'
#
