#!/usr/bin/env python3

# Copyright (c) 2024 Vestas Wind Systems A/S
# Copyright (c) 2020 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import argparse
from dataclasses import dataclass
from pathlib import Path

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

def shield_key(shield):
    return shield.name

def find_shields(args):
    ret = []

    for root in args.board_roots:
        for shields in find_shields_in(root):
            ret.append(shields)

    return sorted(ret, key=shield_key)

def find_shields_in(root):
    shields = root / 'boards' / 'shields'
    ret = []

    for maybe_shield in (shields).iterdir():
        if not maybe_shield.is_dir():
            continue
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
    return parser.parse_args()

def add_args(parser):
    # Remember to update west-completion.bash if you add or remove
    # flags
    parser.add_argument("--board-root", dest='board_roots', default=[],
                        type=Path, action='append',
                        help='add a board root, may be given more than once')

def dump_shields(shields):
    for shield in shields:
        print(f'  {shield.name}')

if __name__ == '__main__':
    dump_shields(find_shields(parse_args()))
