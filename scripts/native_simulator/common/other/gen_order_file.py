#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Generate a linker order file for native simulator ordered entries.

Mach-O has no equivalent of the ELF linker script which places NSI_TASK() and
NSI_HW_EVENT() entries in priority order. Instead all entries of one kind share
a section and the linker is handed an order file listing their symbols in the
order they must end up in.

Only the native simulator's own entries are handled here. Embedded SW which
needs extra symbols ordered supplies helper programs through NSI_ORDER_HELPERS;
they are run afterwards and append to the same file.
"""

import argparse
import re
from pathlib import Path

EVENT_RE = re.compile(
    r"(?<!#define\s)NSI_HW_EVENT\s*\(\s*"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*,\s*"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*,\s*"
    r"([0-9]+)\s*\)"
)

NSI_TASK_RE = re.compile(
    r"(?<!#define\s)NSI_TASK\s*\(\s*"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*,\s*"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*,\s*"
    r"([0-9]+)\s*\)"
)

NSI_TASK_LEVELS = (
    "PRE_BOOT_1",
    "PRE_BOOT_2",
    "HW_INIT",
    "PRE_BOOT_3",
    "FIRST_SLEEP",
    "ON_EXIT_PRE",
    "ON_EXIT_POST",
)


def collect_events(paths):
    """Return HW event symbols in priority order."""
    events = []
    seen = set()

    for path in paths:
        text = Path(path).read_text(encoding="utf-8")
        for timer, function, priority in EVENT_RE.findall(text):
            symbol = f"___nsi_hw_event_{function}{timer}"
            if symbol in seen:
                continue
            seen.add(symbol)
            events.append((int(priority), symbol))

    return [symbol for _, symbol in sorted(events, key=lambda event: event[0])]


def collect_tasks(paths):
    """Return NSI task symbols, bracketed by their per-level range markers."""
    tasks = {level: [] for level in NSI_TASK_LEVELS}
    seen = set()

    for path in paths:
        text = Path(path).read_text(encoding="utf-8")
        for function, level, priority in NSI_TASK_RE.findall(text):
            if level not in tasks:
                continue
            symbol = f"___nsi_task_{function}"
            if symbol in seen:
                continue
            seen.add(symbol)
            tasks[level].append((int(priority), symbol))

    symbols = []
    for level in NSI_TASK_LEVELS:
        symbols.append(f"___nsi_task_range_start_{level}")
        symbols.extend(symbol for _, symbol in sorted(tasks[level], key=lambda task: task[0]))
        symbols.append(f"___nsi_task_range_end_{level}")

    return symbols


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("sources", nargs="*", type=Path)
    return parser.parse_args()


def main():
    args = parse_args()
    symbols = collect_events(args.sources) + collect_tasks(args.sources)
    args.output.write_text("\n".join(symbols) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
