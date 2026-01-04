#!/usr/bin/env python3
#
# Copyright (c) 2022 Kumar Gala <galak@kernel.org>
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import re

HEADER = """\
# Generated devicetree Kconfig
#
# SPDX-License-Identifier: Apache-2.0"""


KCONFIG_TEMPLATE = """
DT_COMPAT_{COMPAT} := {compat}

config DT_HAS_{COMPAT}_ENABLED
\tdef_bool $(dt_compat_enabled,$(DT_COMPAT_{COMPAT}))"""


# Character translation table used to derive Kconfig symbol names
TO_UNDERSCORES = str.maketrans("-,.@/+", "______")

# Matches lines like:
#   compatible: "item1[,item2[,item3...]]"
#
# - One or more comma-separated items inside quotes
# - Items must be non-empty and contain no whitespace, commas, or quotes
# - No spaces allowed around commas
# - Supports an arbitrary number of items
#
# Matches:     "foo", "foo1,foo2", "foo1,foo2,foo3"
# Does not:    ",foo", "foo,", "foo1, foo2"
COMPAT_RE = re.compile(br'^compatible: "([^\s",]+(?:,[^\s",]+)*)"\r?$', re.MULTILINE)


def binding_paths(bindings_dirs):
    # Yields paths to all bindings (.yaml files) in 'bindings_dirs'

    for bindings_dir in bindings_dirs:
        for root, _, filenames in os.walk(bindings_dir):
            for filename in filenames:
                if filename.endswith((".yaml", ".yml")):
                    yield os.path.join(root, filename)


def parse_args():
    # Returns parsed command-line arguments

    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--kconfig-out", required=True,
                        help="path to write the Kconfig file")
    parser.add_argument("--bindings-dirs", nargs='+', required=True,
                        help="directory with bindings in YAML format, "
                        "we allow multiple")

    return parser.parse_args()


def main():
    args = parse_args()

    compats = set()

    for binding_path in binding_paths(args.bindings_dirs):
        with open(binding_path, 'rb') as f:
            matches = COMPAT_RE.findall(f.read())
            if not matches:
                continue
            for match in matches:
                compats.add(match.decode("utf-8"))

    with open(args.kconfig_out, "w", encoding="utf-8") as kconfig_file:
        print(HEADER, file=kconfig_file)

        for c in sorted(compats):
            out = KCONFIG_TEMPLATE.format(
                compat=c, COMPAT=c.upper().translate(TO_UNDERSCORES)
            )
            print(out, file=kconfig_file)


if __name__ == "__main__":
    main()
