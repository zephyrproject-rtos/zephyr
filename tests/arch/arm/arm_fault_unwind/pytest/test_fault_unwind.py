# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Check the fault dump and, when QEMU waits for GDB, that GDB unwinds from the
fault handlers back to the thread entry.
"""

import logging
import os
import re
import subprocess
import sys

from twister_harness import DeviceAdapter

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "pylib", "twister"))
from twisterlib.cmakecache import CMakeCache  # noqa: E402

logger = logging.getLogger(__name__)

# The frames below the exception, from the faulting function up
CALL_CHAIN = ['fault_here', 'fault_level2', 'fault_level1', 'fault_entry', 'z_thread_entry']


def run_gdb(dut: DeviceAdapter, port: str) -> str:
    build_dir = dut.device_config.build_dir
    cache = CMakeCache.from_file(os.path.join(build_dir, 'CMakeCache.txt'))
    gdb = cache.get('CMAKE_GDB', None)
    source_dir = cache.get('APPLICATION_SOURCE_DIR', None)
    elf = cache.get('BYPRODUCT_KERNEL_ELF_NAME', None)
    assert gdb and source_dir and elf
    cmd = [
        gdb,
        '-batch',
        '-ex',
        f'target remote :{port}',
        '-x',
        os.path.join(source_dir, 'fault_unwind.gdbinit'),
        elf,
    ]
    try:
        # Within the timeout of the test, to report the GDB output
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=45)
    except subprocess.TimeoutExpired as e:
        raise AssertionError(f'GDB timed out:\n{e.stdout}\n{e.stderr}') from e
    logger.info(result.stdout)
    assert result.returncode == 0, result.stderr
    # A backtrace error aborts the GDB script before the end of the test
    assert re.search(r'Breakpoint \d+, test_done', result.stdout), (
        f'GDB stopped before test_done:\n{result.stdout[-2000:]}\n{result.stderr[-2000:]}'
    )
    return result.stdout


def backtraces(gdb_output: str) -> list[list[str]]:
    """Return the function names of each backtrace, innermost first."""
    traces = []
    for line in gdb_output.splitlines():
        m = re.match(r'#(\d+)\s+(?:0x[0-9a-f]+ in )?(<signal handler called>|\S+)', line)
        if m is None:
            continue
        if m.group(1) == '0':
            traces.append([])
        traces[-1].append(m.group(2))
    return traces


def test_fault_unwind(dut: DeviceAdapter, gdb_port):
    gdb_output = run_gdb(dut, gdb_port) if gdb_port is not None else None

    lines = dut.readlines_until(regex='DONE', timeout=60)
    cases = []
    faulted = []
    dump_sp = None
    for line in lines:
        m = re.search(r'^case (\S+) (\S+)', line)
        if m:
            cases.append(m.group(1))
            dump_sp = None
            continue
        m = re.search(r'r13/sp: (0x[0-9a-f]+)', line)
        if m:
            dump_sp = m.group(1)
            continue
        m = re.search(r'fault reason \d+, fault_sp (0x[0-9a-f]+)', line)
        if m:
            # The dump shows the SP of the faulting instruction
            assert dump_sp == m.group(1), f'{cases[-1]}: dump SP {dump_sp}, expected {m.group(1)}'
            faulted.append(cases[-1])
    assert cases, 'no fault case ran'
    # Each case faults, except the FPU trap, which only stops in GDB
    assert faulted == [c for c in cases if c != 'fpu_trap'], f'cases {cases}, faulted {faulted}'

    if gdb_output is None:
        return

    assert 'Backtrace stopped' not in gdb_output
    # Every breakpoint is in a handler of an exception taken in fault_here().
    # With FPU sharing, an undefined instruction stops in z_arm_fpu_trap()
    # too, so some cases have more than one backtrace.
    traces = backtraces(gdb_output)
    assert len(traces) >= len(cases), f'{len(traces)} backtraces for {len(cases)} cases'
    for trace in traces:
        assert 'fault_here' in trace, trace
        i = trace.index('fault_here')
        # The exception frame, then the call chain of the interrupted thread
        assert trace[i - 1] == '<signal handler called>', trace
        assert trace[i : i + len(CALL_CHAIN)] == CALL_CHAIN, trace
