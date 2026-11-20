#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Append Zephyr symbols to a native simulator linker order file.

Mach-O cannot express the section-name sorting the ELF linker script uses to
order init entries and native tasks, so every entry of one level shares a
section and ld64 is handed an order file instead. This is the native simulator
NSI_ORDER_HELPERS hook: it reads the symbol tables of the embedded images and
appends the Zephyr symbols in the order they must be linked in.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

NATIVE_TASK_LEVELS = (
    "PRE_BOOT_1",
    "PRE_BOOT_2",
    "PRE_BOOT_3",
    "FIRST_SLEEP",
    "ON_EXIT",
)

NATIVE_TASK_SYMBOL_RE = re.compile(
    r"_+native_task_(?P<level>PRE_BOOT_[123]|FIRST_SLEEP|ON_EXIT)_"
    r"(?P<priority>[0-9]+)_(?P<function>[A-Za-z_][A-Za-z0-9_]*)$"
)

INIT_LEVELS = (
    "EARLY",
    "PRE_KERNEL_1",
    "PRE_KERNEL_2",
    "POST_KERNEL",
    "APPLICATION",
    "SMP",
)

INIT_SYMBOL_RE = re.compile(
    r"___init_order_(?P<level>EARLY|PRE_KERNEL_[12]|POST_KERNEL|APPLICATION|SMP)_"
    r"(?P<priority>[0-9]+)_"
    r"(?P<sub_priority>[0-9]+)_(?P<name>[A-Za-z_][A-Za-z0-9_]*)$"
)

INIT_ARRAY_SYMBOL_RE = re.compile(r"___zephyr_init_array_(?P<bound>start|end)_(?P<image>[0-9]+)$")


def read_symbols(nm, objects):
    """Return the symbol names defined by the given object files."""
    symbols = []

    for obj in objects:
        result = subprocess.run(
            [str(nm), "-j", str(obj)],
            check=True,
            capture_output=True,
            text=True,
        )
        symbols.extend(line.strip() for line in result.stdout.splitlines() if line.strip())

    return symbols


def order_symbols(symbols):
    """Return the Zephyr symbols of the images in the order they must link in."""
    tasks = {level: [] for level in NATIVE_TASK_LEVELS}
    entries = {level: [] for level in INIT_LEVELS}
    init_array_bounds = {}
    seen = set()

    for symbol in symbols:
        if symbol in seen:
            continue
        seen.add(symbol)

        init_array_match = INIT_ARRAY_SYMBOL_RE.fullmatch(symbol)
        if init_array_match is not None:
            image = int(init_array_match.group("image"))
            init_array_bounds.setdefault(image, {})[init_array_match.group("bound")] = symbol
            continue

        init_match = INIT_SYMBOL_RE.fullmatch(symbol)
        if init_match is not None:
            entries[init_match.group("level")].append(
                (int(init_match.group("priority")), int(init_match.group("sub_priority")), symbol)
            )
            continue

        task_match = NATIVE_TASK_SYMBOL_RE.fullmatch(symbol)
        if task_match is not None:
            tasks[task_match.group("level")].append((int(task_match.group("priority")), symbol))

    ordered = []

    for level in NATIVE_TASK_LEVELS:
        ordered.append(f"___native_task_range_start_{level}")
        ordered.extend(symbol for _, symbol in sorted(tasks[level], key=lambda task: task[0]))
        ordered.append(f"___native_task_range_end_{level}")

    for level in INIT_LEVELS:
        ordered.append(f"___macho_init_range_start_{level}")
        ordered.extend(
            symbol
            for _, _, symbol in sorted(entries[level], key=lambda entry: (entry[0], entry[1]))
        )
        ordered.append(f"___macho_init_range_end_{level}")

    for image in sorted(init_array_bounds):
        bounds = init_array_bounds[image]
        if {"start", "end"}.issubset(bounds):
            ordered.extend((bounds["start"], bounds["end"]))

    return ordered


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--nm", required=True, type=Path)
    parser.add_argument("--object", action="append", default=[], type=Path)
    return parser.parse_args()


def main():
    args = parse_args()
    if not args.object:
        sys.exit("No embedded image was given, the link order would be wrong")

    ordered = order_symbols(read_symbols(args.nm, args.object))
    with args.output.open("a", encoding="utf-8") as output:
        output.write("\n".join(ordered) + "\n")


if __name__ == "__main__":
    main()
