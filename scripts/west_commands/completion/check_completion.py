#!/usr/bin/env python3
# Copyright (c) 2026 Zephyr Project contributors
# SPDX-License-Identifier: Apache-2.0

"""Verify that committed west completion scripts match generator output."""

import sys
from pathlib import Path

# Allow importing generate.py from the same directory
sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate  # noqa: E402


def check_scripts(rendered: dict[str, str]) -> list[str]:
    stale = []
    for shell, text in rendered.items():
        path = generate.SHELL_OUTPUT[shell]
        current = path.read_text(encoding="utf-8") if path.exists() else ""
        if generate.without_generation_stamp(current) != generate.without_generation_stamp(text):
            stale.append(str(path.relative_to(generate.ZEPHYR_BASE)))
    return stale


def main(argv: list[str] | None = None) -> int:
    ir, errors = generate.collect_ir()
    if errors:
        sys.stderr.write("west completion generator failed to import in-tree parser(s):\n")
        for err in errors:
            sys.stderr.write(f"  {err}\n")
        return 1

    rendered = generate.render(ir)
    stale = check_scripts(rendered)
    if stale:
        sys.stderr.write(
            "West completion scripts are stale. Regenerate with:\n"
            "  python3 scripts/west_commands/completion/generate.py\n"
            "Out of date:\n"
        )
        for path in stale:
            sys.stderr.write(f"  {path}\n")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
