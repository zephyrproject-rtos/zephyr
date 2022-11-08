#!/usr/bin/env python3

# Copyright (c) 2020 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import argparse
from collections import defaultdict
import itertools
from pathlib import Path
from typing import NamedTuple
import sys
import yaml

#
# This is shared code between the build system's 'boards' target
# and the 'west boards' extension command. If you change it, make
# sure to test both ways it can be used.
#
# (It's done this way to keep west optional, making it possible to run
# 'ninja boards' in a build directory without west installed.)
#

class Board(NamedTuple):
    name: str
    arch: str
    dir: Path

def board_key(board):
    return board.name

def find_arch2boards(args):
    arch2board_set = find_arch2board_set(args)
    return {arch: sorted(arch2board_set[arch], key=board_key)
            for arch in arch2board_set}

def find_boards(args):
    return sorted(itertools.chain(*find_arch2board_set(args).values()),
                  key=board_key)

def find_arch2board_set(args):
    arches = sorted(find_arches(args))
    ret = defaultdict(set)

    for root in args.board_roots:
        for arch, boards in find_arch2board_set_in(root, arches, args.board_vendor, args.soc_vendor).items():
            ret[arch] |= boards

    return ret

def find_arches(args):
    arch_set = set()

    for root in args.arch_roots:
        arch_set |= find_arches_in(root)

    return arch_set

def find_arches_in(root):
    ret = set()
    arch = root / 'arch'
    common = arch / 'common'

    if not arch.is_dir():
        return ret

    for maybe_arch in arch.iterdir():
        if not maybe_arch.is_dir() or maybe_arch == common:
            continue
        ret.add(maybe_arch.name)

    return ret

def get_vendors(yaml_file):

    vendors = ()

    with open(yaml_file, "r") as stream:
        try:
            data = yaml.safe_load(stream)
        except yaml.YAMLError:
            sys.stderr.write("Error parsing yaml in " + str(yaml_file) + ". Skipping file\n")
            return None

        for v in ('board_vendor','soc_vendor'):
            if v in data.keys():
                vendors = vendors + (data[v].upper(),)
            else:
                vendors = vendors + (None,)

    return vendors

def find_arch2board_set_in(root, arches, board_vendor, soc_vendor):
    ret = defaultdict(set)
    boards = root / 'boards'
    bvsv = ()

    if board_vendor is not None:
        board_vendor = board_vendor.upper()
    if soc_vendor is not None:
        soc_vendor = soc_vendor.upper()

    for arch in arches:
        if not (boards / arch).is_dir():
            continue

        for maybe_board in (boards / arch).iterdir():
            if not maybe_board.is_dir():
                continue
            for maybe_yaml in maybe_board.iterdir():
                bvsv = ()
                file_name = maybe_yaml.name
                if file_name.endswith('.yaml'):
                    bvsv = get_vendors(maybe_board / file_name)
                    if bvsv is None:
                        continue

                    if soc_vendor is None and board_vendor == bvsv[0]:
                        board_name = file_name[:-len('.yaml')]
                        ret[arch].add(Board(board_name, arch, maybe_board))
                    elif board_vendor is None and soc_vendor == bvsv[1]:
                        board_name = file_name[:-len('.yaml')]
                        ret[arch].add(Board(board_name, arch, maybe_board))
                    elif soc_vendor == bvsv[1] and board_vendor == bvsv[0]:
                        board_name = file_name[:-len('.yaml')]
                        ret[arch].add(Board(board_name, arch, maybe_board))
                    elif soc_vendor is None and board_vendor is None:
                        board_name = file_name[:-len('.yaml')]
                        ret[arch].add(Board(board_name, arch, maybe_board))

                    if bvsv == (board_vendor, soc_vendor):
                        board_name = file_name[:-len('.yaml')]
                        ret[arch].add(Board(board_name, arch, maybe_board))

    return ret

def parse_args():
    parser = argparse.ArgumentParser()
    add_args(parser)
    return parser.parse_args()

def add_args(parser):
    # Remember to update west-completion.bash if you add or remove
    # flags
    parser.add_argument("--arch-root", dest='arch_roots', default=[],
                        type=Path, action='append',
                        help='add an architecture root, may be given more than once')
    parser.add_argument("--board-root", dest='board_roots', default=[],
                        type=Path, action='append',
                        help='add a board root, may be given more than once')
    parser.add_argument('--board-vendor', dest='board_vendor', default=None,
                        help='''board vendor name''')
    parser.add_argument('--soc-vendor', dest='soc_vendor', default=None,
                        help='''SoC vendor name''')


def dump_boards(arch2boards):
    for arch, boards in arch2boards.items():
        print(f'{arch}:')
        for board in boards:
            print(f'  {board.name}')

if __name__ == '__main__':
    dump_boards(find_arch2boards(parse_args()))
