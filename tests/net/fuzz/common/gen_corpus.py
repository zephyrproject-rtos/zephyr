#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
# SPDX-License-Identifier: Apache-2.0

"""Write a fuzz harness's seed corpus as binary files.

Seeds are kept as hex in a seeds.txt because the messages they encode are
full of NUL bytes and the tree takes no binary files; libFuzzer wants a
directory of them, so this produces one. Shared by every fuzz harness
under tests/net/fuzz/, each of which keeps its own seeds.txt.
"""

import argparse
import pathlib
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("seeds", type=pathlib.Path, help="seeds.txt to read")
    parser.add_argument("outdir", type=pathlib.Path, help="directory to write the seeds into")
    args = parser.parse_args()

    args.outdir.mkdir(parents=True, exist_ok=True)

    for lineno, line in enumerate(args.seeds.read_text().splitlines(), 1):
        line = line.split("#", 1)[0].strip()
        if not line:
            continue

        name, _, payload = line.partition(" ")
        try:
            data = bytes.fromhex(payload)
        except ValueError as err:
            sys.exit(f"{args.seeds}:{lineno}: {err}")

        if not data:
            sys.exit(f"{args.seeds}:{lineno}: seed '{name}' is empty")

        (args.outdir / f"{name}.bin").write_bytes(data)


if __name__ == "__main__":
    main()
