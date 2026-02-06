#!/usr/bin/env python3
#
# Copyright (c) 2022 Kumar Gala <galak@kernel.org>
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os

import yaml

try:
    # Use the C LibYAML parser if available, rather than the Python parser.
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader  # type: ignore


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
    parser.add_argument("--kconfig-out", required=True, help="path to write the Kconfig file")
    parser.add_argument(
        "--bindings-dirs",
        nargs='+',
        required=True,
        help="directory with bindings in YAML format, we allow multiple",
    )

    return parser.parse_args()


def extract_compatible(filepath):
    # Extract top-level 'compatible' scalar from a binding YAML file.
    #
    # Uses a byte-level pre-filter to skip files that cannot contain a compatible key, then streams
    # YAML events via yaml.parse() and exits as soon as the compatible value is found to avoid
    # construction of the full representation.

    with open(filepath, "rb") as f:
        raw = f.read()

    # Pre-filter: skip files that can't possibly have a compatible key.
    if b"compatible" not in raw:
        return None

    depth = 0
    expect_value = False
    for event in yaml.parse(raw, Loader=SafeLoader):
        if isinstance(event, (yaml.MappingStartEvent, yaml.SequenceStartEvent)):
            depth += 1
        elif isinstance(event, (yaml.MappingEndEvent, yaml.SequenceEndEvent)):
            depth -= 1
            if depth == 0:
                return None  # Top-level mapping ended, compatible not found
        elif isinstance(event, yaml.ScalarEvent) and depth == 1:
            if expect_value:
                return event.value  # Found it, early exit
            expect_value = event.value == "compatible"
        else:
            expect_value = False

    return None


def main():
    args = parse_args()

    compats = set()

    for binding_path in binding_paths(args.bindings_dirs):
        try:
            compat = extract_compatible(binding_path)
        except OSError as e:
            print(f"WARNING: couldn't read '{binding_path}': {e}")
            continue
        except yaml.YAMLError as e:
            print(
                f"WARNING: '{binding_path}' appears in binding "
                f"directories but isn't valid YAML: {e}"
            )
            continue

        if compat is not None:
            compats.add(compat)

    with open(args.kconfig_out, "w", encoding="utf-8") as kconfig_file:
        print(HEADER, file=kconfig_file)

        for c in sorted(compats):
            out = KCONFIG_TEMPLATE.format(compat=c, COMPAT=c.upper().translate(TO_UNDERSCORES))
            print(out, file=kconfig_file)


if __name__ == "__main__":
    main()
