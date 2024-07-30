"""
Utility script to migrate SYS_INIT() calls from the old signature
``int (*init_fn)(const struct device *dev)`` to ``int (*init_fn)(void)``.

.. warning::
    Review the output carefully! This script may not cover all cases!

Usage::

    python $ZEPHYR_BASE/scripts/utils/migrate_sys_init.py \
           -p path/to/zephyr-based-project

Copyright (c) 2022 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0
"""

import argparse
from pathlib import Path
import re


def update_sys_init(project, dry_run):
    for p in project.glob("**/*"):
        if not p.is_file() or not p.suffix or p.suffix[1:] not in ("c", "cpp"):
            continue

        with open(p) as f:
            lines = f.readlines()

        sys_inits = []
        for line in lines:
            m = re.match(r"^SYS_INIT\(([A-Za-z0-9_]+),.*", line)
            if m:
                sys_inits.append(m.group(1))
                continue

            m = re.match(r"^SYS_INIT_NAMED\([A-Za-z0-9_]+,\s?([A-Za-z0-9_]+).*", line)
            if m:
                sys_inits.append(m.group(1))
                continue

        if not sys_inits:
            continue

        arg = None
        content = ""
        update = False
        unused = False
        for line in lines:
            m = re.match(
                r"(.*)int ("
                + "|".join(sys_inits)
                + r")\(const\s+struct\s+device\s+\*\s?(.*)\)(.*)",
                line,
            )
            if m:
                b, sys_init, arg, e = m.groups()
                content += f"{b}int {sys_init}(void){e}\n"
                update = True
            elif arg:
                m = re.match(r"^\s?ARG_UNUSED\(" + arg + r"\);.*$", line)
                if m:
                    arg = None
                    unused = True
                else:
                    content += line
            elif unused:
                m = re.match(r"^\s?\n$", line)
                if not m:
                    content += line
                unused = False
            else:
                content += line

        if update:
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

    update_sys_init(args.project, args.dry_run)
