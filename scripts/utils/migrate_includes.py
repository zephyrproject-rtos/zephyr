"""
Utility script to migrate Zephyr-based projects to the new <zephyr/...> include
prefix.

.. note::
    The script will also migrate <zephyr.h> or <zephyr/zephyr.h> to
    <zephyr/kernel.h>.

Usage::

    python $ZEPHYR_BASE/scripts/utils/migrate_includes.py \
           -p path/to/zephyr-based-project

Copyright (c) 2022 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0
"""

import argparse
import re
import sys
from pathlib import Path

ZEPHYR_BASE = Path(__file__).parents[2]

EXTENSIONS = ("c", "cpp", "h", "hpp", "dts", "dtsi", "rst", "S", "overlay", "ld")


def update_includes(project, dry_run):
    for p in project.glob("**/*"):
        if not p.is_file() or not p.suffix or p.suffix[1:] not in EXTENSIONS:
            continue

        try:
            with open(p) as f:
                lines = f.readlines()
        except UnicodeDecodeError:
            print(f"File with invalid encoding: {p}, skipping", file=sys.stderr)
            continue

        content = ""
        migrate = False
        for line in lines:
            m = re.match(r"^(.*)#include <(.*\.h)>(.*)$", line)
            if m and m.group(2) in ("zephyr.h", "zephyr/zephyr.h"):
                content += (
                    m.group(1)
                    + "#include <zephyr/kernel.h>"
                    + m.group(3)
                    + "\n"
                )
                migrate = True
            elif (
                m
                and not m.group(2).startswith("zephyr/")
                and (ZEPHYR_BASE / "include" / "zephyr" / m.group(2)).exists()
            ):
                content += (
                    m.group(1)
                    + "#include <zephyr/"
                    + m.group(2)
                    + ">"
                    + m.group(3)
                    + "\n"
                )
                migrate = True
            else:
                content += line

        if migrate:
            print(f"Updating {p}{' (dry run)' if dry_run else ''}")
            if not dry_run:
                with open(p, "w") as f:
                    f.write(content)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument(
        "-p", "--project", type=Path, required=True, help="Zephyr-based project path"
    )
    parser.add_argument("--dry-run", action="store_true", help="Dry run")
    args = parser.parse_args()

    update_includes(args.project, args.dry_run)
