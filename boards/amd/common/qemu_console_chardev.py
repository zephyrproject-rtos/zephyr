#!/usr/bin/env python3
# Copyright (c) 2026 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# Rewrite the "chardev" property of one serial node in a (decompiled) Xilinx
# QEMU hardware device tree to "con".
#
# The Xilinx arm-generic-fdt machine binds only the chardev named "con" to the
# QEMU stdio backend under -nographic. The stock Xilinx/qemu-devicetrees models
# name every UART chardev "serialN", so the UART that Zephyr uses as its console
# produces no output until its chardev is renamed. This patches exactly the
# console UART (identified by its register base address), leaving the rest of the
# model untouched. See boards/amd/common/qemu_hw_dtb.cmake for the caller.

import argparse
import re
import sys


def main():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--input", required=True, help="input .dts")
    parser.add_argument("--output", required=True, help="output .dts")
    parser.add_argument(
        "--reg",
        required=True,
        help="console serial register base, e.g. 0xf1920000",
    )
    args = parser.parse_args()

    target = int(args.reg, 16)
    node_re = re.compile(r"^\s*serial@0x?([0-9a-fA-F]+)\s*\{")
    chardev_re = re.compile(r'(chardev\s*=\s*)"[^"]*"')

    with open(args.input) as f:
        lines = f.readlines()

    depth = 0
    in_target = False
    patched = False
    for i, line in enumerate(lines):
        if not in_target:
            m = node_re.match(line)
            if m and int(m.group(1), 16) == target:
                in_target = True
                depth = line.count("{") - line.count("}")
                continue
        else:
            depth += line.count("{") - line.count("}")
            new = chardev_re.sub(r'\1"con"', line)
            if new != line:
                lines[i] = new
                patched = True
            if depth <= 0:
                in_target = False

    if not patched:
        sys.exit(f"error: no chardev found in serial@{args.reg}")

    with open(args.output, "w") as f:
        f.writelines(lines)


if __name__ == "__main__":
    main()
