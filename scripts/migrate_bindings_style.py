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
BINDINGS_PROPERTIES_ALLOWLIST = {
    "mmc-hs400-1_8v",
    "mmc-hs200-1_8v",
}


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
                if '_' in prop_name and prop_name not in BINDINGS_PROPERTIES_ALLOWLIST:
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


def check_valid_cmd_path():
    """
    Checks whether the given command line argument is a valid path.
    """
    for path in BINDINGS_PATH:
        path = Path(path)
        if not path.exists() or not path.is_dir():
            raise FileNotFoundError(f"No such directory: '{path}'")


if __name__ == '__main__':
    args = parse_cmd_args()

    if args.path is not None:
        BINDINGS_PATH.extend(args.path)

    check_valid_cmd_path()

    bindings = get_yaml_binding_paths()
    replace_bindings_underscores(bindings)

    sys.exit(0)
