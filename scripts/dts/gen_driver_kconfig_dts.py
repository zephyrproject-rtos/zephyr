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
    from yaml import SafeLoader     # type: ignore


# Cache the event types for faster comparison
MAPPING_START_EVENT = yaml.events.MappingStartEvent
MAPPING_END_EVENT = yaml.events.MappingEndEvent
SEQUENCE_START_EVENT = yaml.events.SequenceStartEvent
SEQUENCE_END_EVENT = yaml.events.SequenceEndEvent
SCALAR_EVENT = yaml.events.ScalarEvent

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
    parser.add_argument("--kconfig-out", required=True,
                        help="path to write the Kconfig file")
    parser.add_argument("--bindings-dirs", nargs='+', required=True,
                        help="directory with bindings in YAML format, "
                        "we allow multiple")

    return parser.parse_args()


def compatible_from_yaml_file(path: str) -> str | None:
    try:
        with open(path, encoding="utf-8") as f:
            mapping_depth = 0
            expecting_key = False
            pending_compat = False
            skip = 0

            for event in yaml.parse(f, Loader=SafeLoader):
                t = event.__class__

                if t is MAPPING_START_EVENT:
                    mapping_depth += 1
                    if skip:
                        skip += 1
                    elif mapping_depth == 1:
                        expecting_key = True
                    elif mapping_depth == 2 and not expecting_key:
                        pending_compat = False
                        skip = 1
                    continue

                if t is MAPPING_END_EVENT:
                    if skip:
                        skip -= 1
                    mapping_depth -= 1
                    if mapping_depth == 1 and skip == 0:
                        expecting_key = True
                    continue

                if t is SEQUENCE_START_EVENT:
                    if skip:
                        skip += 1
                    elif mapping_depth == 1 and not expecting_key:
                        pending_compat = False
                        skip = 1
                    continue

                if t is SEQUENCE_END_EVENT:
                    if skip:
                        skip -= 1
                    if mapping_depth == 1 and skip == 0:
                        expecting_key = True
                    continue

                if skip or mapping_depth != 1:
                    continue

                if t is SCALAR_EVENT:
                    value = event.value
                    if expecting_key:
                        pending_compat = (value == "compatible")
                        expecting_key = False
                    else:
                        if pending_compat:
                            return value
                        pending_compat = False
                        expecting_key = True

    except yaml.YAMLError as e:
        print(f"WARNING: '{path}' appears in binding directories but isn't valid YAML: {e}")
    except OSError as e:
        print(f"WARNING: can't read '{path}': {e}")

    return None

def main():
    args = parse_args()

    compats = set()

    for binding_path in binding_paths(args.bindings_dirs):
        compat = compatible_from_yaml_file(binding_path)
        if compat is not None:
            compats.add(compat)

    with open(args.kconfig_out, "w", encoding="utf-8") as kconfig_file:
        print(HEADER, file=kconfig_file)

        for c in sorted(compats):
            out = KCONFIG_TEMPLATE.format(
                compat=c, COMPAT=c.upper().translate(TO_UNDERSCORES)
            )
            print(out, file=kconfig_file)


if __name__ == "__main__":
    main()
