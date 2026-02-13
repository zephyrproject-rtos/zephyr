# Copyright (c) 2026 Antmicro <www.antmicro.com>
# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

import sys

f_str = sys.argv[1] if len(sys.argv) >= 2 else ""

fs = f_str.split(",") if f_str else []

out_path = sys.argv[2]

with open(out_path, "w") as out_f:
    for f in fs:
        print(f"extern void {f}(void);", file=out_f)

    print(f"unsigned instr_exclude_list_len = {len(fs)};", file=out_f)
    print("void (*instr_exclude_list[])(void) __attribute__((retain)) = {", file=out_f)

    for f in fs:
        print(f"    {f},", file=out_f)

    print("};", file=out_f)
