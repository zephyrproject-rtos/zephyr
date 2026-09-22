#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""
Drop the function positions from gcovr JSON tracefiles.

gcovr records a function's source position only when gcov reports "JSON format
version: 2", which gcc grew in 14; with an older gcov it parses the text output
instead, which carries no positions. Merging tracefiles from both aborts with a
KeyError as soon as a function sits on different lines in the two: the merged
entry gains the new line for the counters but not for the position, and writing
the report then fails. Nothing downstream reads the positions.
"""

import argparse
import json
import sys


def strip_positions(path: str) -> int:
    with open(path) as fp:
        tracefile = json.load(fp)

    dropped = 0
    for filecov in tracefile.get("files", []):
        for function in filecov.get("functions", []):
            if function.pop("pos", None) is not None:
                dropped += 1

    if dropped > 0:
        with open(path, "w") as fp:
            json.dump(tracefile, fp)

    return dropped


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument("tracefile", nargs="+", help="gcovr JSON tracefile to rewrite in place")
    args = parser.parse_args()

    for path in args.tracefile:
        print(f"{path}: dropped {strip_positions(path)} function positions")

    return 0


if __name__ == "__main__":
    sys.exit(main())
