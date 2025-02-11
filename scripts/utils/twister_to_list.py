"""
Utility script to migrate Twister configuration files from using string-based
lists to native YAML lists.

Usage::

    python $ZEPHYR_BASE/scripts/utils/twister_to_list.py \
           -p path/to/zephyr-based-project

Copyright (c) 2023 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0
"""

import argparse
from pathlib import Path

from ruamel.yaml import YAML


FIELDS = (
    "arch_exclude",
    "arch_allow",
    "depends_on",
    "extra_args",
    "extra_sections",
    "platform_exclude",
    "platform_allow",
    "tags",
    "toolchain_exclude",
    "toolchain_allow"
)

def process(conf):
    update = False
    for field in FIELDS:
        val = conf.get(field)
        if not val or not isinstance(val, str):
            continue

        s = val.split()
        if len(s) > 1:
            conf[field] = s
            update = True

    return update


def twister_to_list(project, dry_run):
    yaml = YAML()
    yaml.indent(offset=2)
    yaml.preserve_quotes = True

    for p in project.glob("**/*"):
        if p.name not in ("testcase.yaml", "sample.yaml"):
            continue

        conf = yaml.load(p)
        update = False

        common = conf.get("common")
        if common:
            update |= process(common)

        for _, spec in conf["tests"].items():
            update |= process(spec)

        if update:
            print(f"Updating {p}{' (dry run)' if dry_run else ''}")
            if not dry_run:
                with open(p, "w") as f:
                    yaml.dump(conf, f)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument(
        "-p", "--project", type=Path, required=True, help="Zephyr-based project path"
    )
    parser.add_argument("--dry-run", action="store_true", help="Dry run")
    args = parser.parse_args()

    twister_to_list(args.project, args.dry_run)
