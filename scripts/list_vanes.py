#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
List the test setups defined under tests/vanes and the board targets
realizing each with a vane. See doc/develop/test/vanes.rst.
"""

import argparse
import json
import sys
from argparse import Namespace
from pathlib import Path

import list_boards
import list_hardware
import vanes

ZEPHYR_BASE = Path(__file__).resolve().parents[1]


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument(
        '--board-root',
        dest='board_roots',
        default=[],
        type=Path,
        action='append',
        help='add a board root, may be given more than once',
    )
    parser.add_argument(
        '--soc-root',
        dest='soc_roots',
        default=[],
        type=Path,
        action='append',
        help='add a SoC root, may be given more than once',
    )
    parser.add_argument('--setup', help='list the targets realizing this setup')
    parser.add_argument('--board', help='list the setups each target of this board realizes')
    parser.add_argument('--json', action='store_true', help='print JSON')
    return parser.parse_args()


def targets_of(board):
    """The board's targets, with what names their vane files."""
    single_soc = len(board.socs) == 1
    for qualifiers in list_boards.board_v2_qualifiers(board):
        revisions = [r.name for r in board.revisions if r.name] or [None]
        for revision in revisions:
            name = (
                f'{board.name}@{revision}/{qualifiers}'
                if revision
                else f'{board.name}/{qualifiers}'
            )
            yield name, vanes.Target(board.name, qualifiers, revision, single_soc)


def fixture_text(fixture: str | dict[str, str] | None) -> str | None:
    """A vane's fixture statement as one line: a name, or 'a->b' pairs."""
    if isinstance(fixture, dict):
        return ', '.join(f'{named}->{replacement}' for named, replacement in fixture.items())
    return fixture


def main():
    args = parse_args()
    board_roots = [ZEPHYR_BASE, *args.board_roots]
    soc_roots = [ZEPHYR_BASE, *args.soc_roots]
    setups = vanes.find_setups()
    if args.setup and args.setup not in setups:
        sys.exit(f'No test setup named {args.setup!r} under tests/vanes')

    lb_args = Namespace(
        board_roots=board_roots,
        soc_roots=soc_roots,
        arch_roots=board_roots,
        board=args.board,
        board_dir=None,
    )
    boards = list_boards.find_v2_boards(lb_args)
    if args.board and args.board not in boards:
        sys.exit(f'No board named {args.board!r}')
    soc2dirs = {
        soc.name: soc.folder
        for soc in list_hardware.find_v2_systems(Namespace(soc_roots=soc_roots)).get_socs()
    }

    # setup -> target -> {files, fixture}; fixture is what the vane says, if anything
    realized = {name: {} for name in setups}
    for board in boards.values():
        if args.board and board.name != args.board:
            continue
        for target, spec in targets_of(board):
            for name in setups:
                if args.setup and name != args.setup:
                    continue
                vane = vanes.find_vane(name, spec, board.directories, soc2dirs.get(spec.soc, []))
                if vane is None:
                    continue
                realized[name][target] = {
                    'files': [
                        str(f.relative_to(ZEPHYR_BASE)) if f.is_relative_to(ZEPHYR_BASE) else str(f)
                        for f in vane.files
                    ],
                    'fixture': fixture_text(vane.fixture),
                }

    if args.json:
        out = {
            name: {
                'description': setup.description.strip(),
                'devicetree': setup.evidence,
                'targets': realized[name],
            }
            for name, setup in setups.items()
            if not args.setup or name == args.setup
        }
        json.dump(out, sys.stdout, indent=2)
        print()
        return

    if args.setup:
        for target, info in sorted(realized[args.setup].items()):
            fixture = info['fixture'] or '-'
            print(f'{target:48} {fixture:20} {" ".join(info["files"])}')
        return

    if args.board:
        for name in setups:
            for target, info in sorted(realized[name].items()):
                fixture = info['fixture'] or '-'
                print(f'{target:48} {name:24} {fixture:20} {" ".join(info["files"])}')
        return

    print(f'{"setup":24} {"targets":>8}  description')
    for name, setup in setups.items():
        first = setup.description.strip().splitlines()[0]
        print(f'{name:24} {len(realized[name]):8}  {first}')


if __name__ == '__main__':
    main()
