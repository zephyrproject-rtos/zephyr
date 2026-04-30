#!/usr/bin/env python3
# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

"""
Post-build check for data-bus loads from IROM on Xtensa.

On Harvard-architecture Xtensa SoCs (ESP32, ESP32-S3), IROM is only
accessible via the instruction bus. Any data-bus load instruction
(lsi, lsiu, lsx, lsxu, l32i, l16ui, l8ui, etc.) targeting an IROM
address will cause EXCCAUSE 3 (load/store error) at runtime.

GCC 14 may generate FPU loads (lsi) from literal pools placed in IROM.
This script detects the pattern at build time by tracking l32r values
and checking if subsequent data-bus loads use those IROM addresses.

Register tracking is reset at function boundaries and when a register
is overwritten by a non-l32r instruction.
"""

import re
import subprocess
import sys

L32R_RE = re.compile(
    r"([0-9a-f]+):\s+\S+\s+l32r\s+(a\d+),\s*[0-9a-f]+\s+[^(]*\(([0-9a-f]+)",
    re.IGNORECASE,
)

FPU_LOAD_RE = re.compile(
    r"([0-9a-f]+):\s+\S+\s+(lsi|lsiu|lsx|lsxu)\s+(f\d+),\s*(a\d+)",
    re.IGNORECASE,
)

REG_WRITE_RE = re.compile(
    r"[0-9a-f]+:\s+\S+\s+(?!s32i|s16i|s8i|ssi|ssiu|ssx|ssxu)\S{1,12}\s+(a\d+),",
    re.IGNORECASE,
)

CALL_RE = re.compile(
    r"[0-9a-f]+:\s+\S+\s+callx?[048]",
    re.IGNORECASE,
)

FUNC_RE = re.compile(r"^[0-9a-f]+ <\S+>:")


def get_irom_range(objdump, elf):
    """Get the IROM (.flash.text) address range from ELF headers."""
    result = subprocess.run([objdump, "-h", elf], capture_output=True, text=True, check=True)
    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 4 and parts[1] == ".flash.text":
            size = int(parts[2], 16)
            vma = int(parts[3], 16)
            return vma, vma + size
    return None, None


def _handle_l32r(line, reg_values):
    """Track l32r: loads a literal pool value into aX."""
    m = L32R_RE.search(line)
    if not m:
        return False
    reg = m.group(2)
    value = int(m.group(3), 16)
    reg_values[reg] = (value, int(m.group(1), 16))
    return True


def _check_fpu_load(line, reg_values, irom_start, irom_end):
    """Check if an FPU load accesses an IROM address. Returns error or None."""
    m = FPU_LOAD_RE.search(line)
    if not m:
        return None

    areg = m.group(4)
    if areg == "a1" or areg not in reg_values:
        return None

    addr, l32r_pc = reg_values[areg]
    if addr < irom_start or addr >= irom_end:
        return None

    pc = int(m.group(1), 16)
    insn = m.group(2)
    freg = m.group(3)
    return (
        f"  {insn} {freg}, {areg} at 0x{pc:08x} "
        f"loads from IROM 0x{addr:08x} "
        f"(l32r at 0x{l32r_pc:08x})"
    )


def _handle_reg_write(line, reg_values):
    """Clear register tracking when overwritten by non-l32r/non-store."""
    m = REG_WRITE_RE.search(line)
    if m:
        reg_values.pop(m.group(1), None)


def check_data_loads_from_irom(objdump, elf):
    """Scan for data-bus load instructions accessing IROM addresses."""
    irom_start, irom_end = get_irom_range(objdump, elf)
    if irom_start is None:
        return []

    result = subprocess.run([objdump, "-d", elf], capture_output=True, text=True, check=True)

    reg_values = {}
    errors = []

    for line in result.stdout.splitlines():
        if FUNC_RE.match(line) or CALL_RE.search(line):
            reg_values.clear()
            continue

        if _handle_l32r(line, reg_values):
            continue

        error = _check_fpu_load(line, reg_values, irom_start, irom_end)
        if error is not None:
            errors.append(error)
            continue

        _handle_reg_write(line, reg_values)

    return errors


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <objdump> <elf>", file=sys.stderr)
        sys.exit(2)

    objdump = sys.argv[1]
    elf = sys.argv[2]

    errors = check_data_loads_from_irom(objdump, elf)
    if errors:
        print("ERROR: data-bus load instructions accessing IROM (instruction-bus only) detected.")
        print("This causes EXCCAUSE 3 (load/store error) at runtime.")
        print()
        for e in errors:
            print(e)
        print()
        print(f"Total: {len(errors)} problematic instruction(s).")
        sys.exit(1)

    sys.exit(0)


if __name__ == "__main__":
    main()
