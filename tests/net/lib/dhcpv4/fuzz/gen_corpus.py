#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
# SPDX-License-Identifier: Apache-2.0

"""Write the seed corpus of the DHCPv4 client fuzz harness as binary files.

The seeds are kept as hex in seeds.txt because a DHCP message is full of
NUL bytes and the tree takes no binary files; libFuzzer wants a
directory of them, so this produces one.
"""

import argparse
import pathlib
import sys

SEEDS = pathlib.Path(__file__).with_name("seeds.txt")


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("outdir", type=pathlib.Path, help="directory to write the seeds into")
    args = parser.parse_args()

    args.outdir.mkdir(parents=True, exist_ok=True)

    for lineno, line in enumerate(SEEDS.read_text().splitlines(), 1):
        line = line.split("#", 1)[0].strip()
        if not line:
            continue

        name, _, payload = line.partition(" ")
        try:
            data = bytes.fromhex(payload)
        except ValueError as err:
            sys.exit(f"{SEEDS}:{lineno}: {err}")

        if not data:
            sys.exit(f"{SEEDS}:{lineno}: seed '{name}' is empty")

        (args.outdir / f"{name}.bin").write_bytes(data)


if __name__ == "__main__":
    main()
