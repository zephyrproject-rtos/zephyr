#!/usr/bin/env python3

# Copyright (c) 2024 Vestas Wind Systems A/S
# Copyright (c) 2020 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path

import pykwalify.core
import yaml

try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader

SHIELD_SCHEMA_PATH = str(Path(__file__).parent / 'schemas' / 'shield-schema.yml')
with open(SHIELD_SCHEMA_PATH) as f:
    shield_schema = yaml.load(f.read(), Loader=SafeLoader)

SHIELD_YML = 'shield.yml'

#
# This is shared code between the build system's 'shields' target
# and the 'west shields' extension command. If you change it, make
# sure to test both ways it can be used.
#
# (It's done this way to keep west optional, making it possible to run
# 'ninja shields' in a build directory without west installed.)
#

@dataclass(frozen=True)
class Shield:
    name: str
    dir: Path
    full_name: str | None = None
    vendor: str | None = None
    supported_features: list[str] | None = None

def shield_key(shield):
    return shield.name

def process_shield_data(shield_data, shield_dir):
    # Create shield from yaml data
    return Shield(
        name=shield_data['name'],
        dir=shield_dir,
        full_name=shield_data.get('full_name'),
        vendor=shield_data.get('vendor'),
        supported_features=shield_data.get('supported_features', []),
    )

def find_shields(args):
    ret = []

    for root in args.board_roots:
        for shields in find_shields_in(root):
            ret.append(shields)

    return sorted(ret, key=shield_key)

def find_shields_in(root):
    shields = root / 'boards' / 'shields'
    ret = []

    if not shields.exists():
        return ret

    for maybe_shield in (shields).iterdir():
        if not maybe_shield.is_dir():
            continue

        # Check for shield.yml first
        shield_yml = maybe_shield / SHIELD_YML
        if shield_yml.is_file():
            with shield_yml.open('r', encoding='utf-8') as f:
                shield_data = yaml.load(f.read(), Loader=SafeLoader)

            try:
                pykwalify.core.Core(source_data=shield_data, schema_data=shield_schema).validate()
            except pykwalify.errors.SchemaError as e:
                sys.exit(f'ERROR: Malformed shield.yml in file: {shield_yml.as_posix()}\n{e}')

            if 'shields' in shield_data:
                # Multiple shields format
                for shield_info in shield_data['shields']:
                    ret.append(process_shield_data(shield_info, maybe_shield))
            elif 'shield' in shield_data:
                # Single shield format
                ret.append(process_shield_data(shield_data['shield'], maybe_shield))
            continue

        # Fallback to legacy method if no shield.yml
        for maybe_kconfig in maybe_shield.iterdir():
            if maybe_kconfig.name == 'Kconfig.shield':
                for maybe_overlay in maybe_shield.iterdir():
                    file_name = maybe_overlay.name
                    if file_name.endswith('.overlay'):
                        shield_name = file_name[:-len('.overlay')]
                        ret.append(Shield(shield_name, maybe_shield))

    return sorted(ret, key=shield_key)

def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    add_args(parser)
    add_args_formatting(parser)
    return parser.parse_args()

def add_args(parser):
    # Remember to update west-completion.bash if you add or remove
    # flags
    parser.add_argument("--board-root", dest='board_roots', default=[],
                        type=Path, action='append',
                        help='add a board root, may be given more than once')

def add_args_formatting(parser):
    parser.add_argument("--json", action='store_true',
                        help='''output list of shields in JSON format''')

def dump_shields(shields):
    if args.json:
        print(
            json.dumps([{'dir': shield.dir.as_posix(), 'name': shield.name} for shield in shields])
        )
    else:
        for shield in shields:
            print(f'  {shield.name}')

if __name__ == '__main__':
    args = parse_args()
    dump_shields(find_shields(args))
