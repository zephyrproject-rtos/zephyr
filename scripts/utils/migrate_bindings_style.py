#!/usr/bin/env python3

# Copyright (c) 2025 James Roy <rruuaanng@outlook.com>
# SPDX-License-Identifier: Apache-2.0

# This script is used to replace the properties containing
# underscores in the binding.


import argparse
import os
import sys
from pathlib import Path

import yaml

# bindings base
BINDINGS_PATH = [Path("dts/bindings/")]
BINDINGS_PROPERTIES_AL = None
with open(Path(__file__).parent / 'bindings_properties_allowlist.yaml') as f:
    allowlist = yaml.safe_load(f.read())
    if allowlist is not None:
        BINDINGS_PROPERTIES_AL = set(allowlist)
    else:
        BINDINGS_PROPERTIES_AL = set()


def parse_cmd_args():
    parse = argparse.ArgumentParser(allow_abbrev=False)
    parse.add_argument("--path", nargs="+", help="User binding path directory", type=Path)
    return parse.parse_args()


def get_yaml_binding_paths() -> list:
    """Returns a list of 'dts/bindings/**/*.yaml'"""
    from glob import glob

    bindings = []
    for path in BINDINGS_PATH:
        yamls = glob(f"{os.fspath(path)}/**/*.yaml", recursive=True)
        bindings.extend(yamls)
    return bindings


def replace_bindings_underscores(binding_paths):
    """
    Replace all property names containing underscores in the bindings.
    """
    for binding_path in binding_paths:
        with open(binding_path, encoding="utf-8") as f:
            yaml_binding = yaml.safe_load(f)
            properties = yaml_binding.get("properties", {})
            for prop_name in properties:
                if '_' in prop_name and prop_name not in BINDINGS_PROPERTIES_AL:
                    _replace_underscores(binding_path, prop_name)


def _replace_underscores(binding_path, prop_name):
    """Replace implementation"""
    with open(binding_path, "r+", encoding="utf-8") as f:
        lines = f.readlines()
        for i, rowline in enumerate(lines):
            line = rowline.split(":")[0].strip()
            if prop_name == line and "#" not in rowline:
                new_key = prop_name.replace("_", "-")
                if new_key != line:
                    lines[i] = rowline.replace(line, new_key)
        f.seek(0)
        f.writelines(lines)


if __name__ == '__main__':
    args = parse_cmd_args()

    if args.path is not None:
        BINDINGS_PATH.extend(args.path)

    bindings = get_yaml_binding_paths()
    replace_bindings_underscores(bindings)

    sys.exit(0)
