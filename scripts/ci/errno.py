#!/usr/bin/env python3
#
# Copyright (c) 2021 Nordic Semiconductor NA
#
# SPDX-License-Identifier: Apache-2.0

"""Check minimal libc error numbers against newlib.

This script loads the errno.h included in Zephyr's minimal libc and checks its
contents against the SDK's newlib errno.h. This is done to ensure that both C
libraries are aligned at all times.
"""


import os
from pathlib import Path
import re
import sys

def parse_errno(path):
    with open(path, 'r') as f:
        r = re.compile(r'^\s*#define\s+([A-Z]+)\s+([0-9]+)')
        errnos = []
        for line in f:
            m = r.match(line)
            if m:
                errnos.append(m.groups())

    return errnos

def main():

    minimal = Path("lib/libc/minimal/include/errno.h")
    newlib = Path("arm-zephyr-eabi/arm-zephyr-eabi/include/sys/errno.h")

    try:
        minimal = os.environ['ZEPHYR_BASE'] / minimal
        newlib = os.environ['ZEPHYR_SDK_INSTALL_DIR'] / newlib
    except KeyError as e:
        print(f'Environment variable missing: {e}', file=sys.stderr)
        sys.exit(1)

    minimal = parse_errno(minimal)
    newlib = parse_errno(newlib)

    for e in minimal:
        if e[0] not in [x[0] for x in newlib] or e[1] != next(
            filter(lambda _e: _e[0] == e[0], newlib))[1]:
            print('Invalid entry in errno.h:', file=sys.stderr)
            print(f'{e[0]} (with value {e[1]})', file=sys.stderr)
            sys.exit(1)

    print('errno.h validated correctly')

if __name__ == "__main__":
    main()
