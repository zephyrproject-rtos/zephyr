#!/usr/bin/env python3
#
# Copyright (c) 2022 Kumar Gala <galak@kernel.org>
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import sys
import re

import yaml
try:
    # Use the C LibYAML parser if available, rather than the Python parser.
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader     # type: ignore

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'python-devicetree',
                                'src'))

def binding_paths(bindings_dirs):
    # Returns a list with the paths to all bindings (.yaml files) in
    # 'bindings_dirs'

    binding_paths = []

    for bindings_dir in bindings_dirs:
        for root, _, filenames in os.walk(bindings_dir):
            for filename in filenames:
                if filename.endswith(".yaml") or filename.endswith(".yml"):
                    binding_paths.append(os.path.join(root, filename))

    return binding_paths

def parse_args():
    # Returns parsed command-line arguments

    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--kconfig-out", required=True,
                        help="path to write the Kconfig file")
    parser.add_argument("--bindings-dirs", nargs='+', required=True,
                        help="directory with bindings in YAML format, "
                        "we allow multiple")

    return parser.parse_args()

def printfile(s):
    print(s, file=kconfig_file)

def str2ident(s):
    # Converts 's' to a form suitable for (part of) an identifier

    return re.sub('[-,.@/+]', '_', s.upper())

def compat2kconfig(compat):
    compat_ident = str2ident(compat)

    printfile(f'')
    printfile(f'DT_COMPAT_{compat_ident} := {compat}')
    printfile(f'')
    printfile(f'config DT_HAS_{compat_ident}_ENABLED')
    printfile(f'\tdef_bool $(dt_compat_enabled,$(DT_COMPAT_{compat_ident}))')

def main():
    global kconfig_file
    args = parse_args()

    compat_list = []

    for binding_path in binding_paths(args.bindings_dirs):
        with open(binding_path, encoding="utf-8") as f:
            contents = f.read()

            try:
                # Parsed PyYAML output (Python lists/dictionaries/strings/etc.,
                # representing the file)
                raw = yaml.load(contents, Loader=SafeLoader)
            except yaml.YAMLError as e:
                print(f"WARNING: '{binding_path}' appears in binding "
                      f"directories but isn't valid YAML: {e}")
                continue
        if raw is None or 'compatible' not in raw:
            continue

        compat_list.append(raw['compatible'])

    # Remove any duplicates and sort the list
    compat_list = sorted(set(compat_list))

    with open(args.kconfig_out, "w", encoding="utf-8") as kconfig_file:
        printfile(f'# Generated devicetree Kconfig')
        printfile(f'#')
        printfile(f'# SPDX-License-Identifier: Apache-2.0')

        for c in compat_list:
            compat2kconfig(c)


if __name__ == "__main__":
    main()
