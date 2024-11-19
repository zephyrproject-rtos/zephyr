#!/usr/bin/env python3

# Copyright (c) 2020 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import argparse
from collections import defaultdict, Counter
from dataclasses import dataclass, field
import itertools
from pathlib import Path
import pykwalify.core
import sys
from typing import List, Union
import yaml
import list_hardware
from list_hardware import unique_paths

try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader

BOARD_SCHEMA_PATH = str(Path(__file__).parent / 'schemas' / 'board-schema.yml')
with open(BOARD_SCHEMA_PATH, 'r') as f:
    board_schema = yaml.load(f.read(), Loader=SafeLoader)

BOARD_YML = 'board.yml'

#
# This is shared code between the build system's 'boards' target
# and the 'west boards' extension command. If you change it, make
# sure to test both ways it can be used.
#
# (It's done this way to keep west optional, making it possible to run
# 'ninja boards' in a build directory without west installed.)
#


@dataclass
class Revision:
    name: str
    variants: List[str] = field(default_factory=list)

    @staticmethod
    def from_dict(revision):
        revisions = []
        for r in revision.get('revisions', []):
            revisions.append(Revision.from_dict(r))
        return Revision(revision['name'], revisions)


@dataclass
class Variant:
    name: str
    variants: List[str] = field(default_factory=list)

    @staticmethod
    def from_dict(variant):
        variants = []
        for v in variant.get('variants', []):
            variants.append(Variant.from_dict(v))
        return Variant(variant['name'], variants)


@dataclass
class Cpucluster:
    name: str
    variants: List[str] = field(default_factory=list)


@dataclass
class Soc:
    name: str
    cpuclusters: List[str] = field(default_factory=list)
    variants: List[str] = field(default_factory=list)

    @staticmethod
    def from_soc(soc, variants):
        if soc is None:
            return None
        if soc.cpuclusters:
            cpus = []
            for c in soc.cpuclusters:
                cpus.append(Cpucluster(c,
                            [Variant.from_dict(v) for v in variants if c == v['cpucluster']]
                ))
            return Soc(soc.name, cpuclusters=cpus)
        return Soc(soc.name, variants=[Variant.from_dict(v) for v in variants])


@dataclass(frozen=True)
class Board:
    name: str
    # HWMv1 only supports a single Path, and requires Board dataclass to be hashable.
    directories: Union[Path, List[Path]]
    hwm: str
    full_name: str = None
    arch: str = None
    vendor: str = None
    revision_format: str = None
    revision_default: str = None
    revision_exact: bool = False
    revisions: List[str] = field(default_factory=list, compare=False)
    socs: List[Soc] = field(default_factory=list, compare=False)
    variants: List[str] = field(default_factory=list, compare=False)

    @property
    def dir(self):
        # Get the main board directory.
        if isinstance(self.directories, Path):
            return self.directories
        return self.directories[0]

    def from_qualifier(self, qualifiers):
        qualifiers_list = qualifiers.split('/')

        node = Soc(None)
        n = len(qualifiers_list)
        if n > 0:
            soc_qualifier = qualifiers_list.pop(0)
            for s in self.socs:
                if s.name == soc_qualifier:
                    node = s
                    break

        if n > 1:
            if node.cpuclusters:
                cpu_qualifier = qualifiers_list.pop(0)
                for c in node.cpuclusters:
                    if c.name == cpu_qualifier:
                        node = c
                        break
                else:
                    node = Variant(None)

        for q in qualifiers_list:
            for v in node.variants:
                if v.name == q:
                    node = v
                    break
            else:
                node = Variant(None)

        if node in (Soc(None), Variant(None)):
            sys.exit(f'ERROR: qualifiers {qualifiers} not found when extending board {self.name}')

        return node


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

    for root in unique_paths(args.board_roots):
        for arch, boards in find_arch2board_set_in(root, arches, args.board_dir).items():
            if args.board is not None:
                ret[arch] |= {b for b in boards if b.name == args.board}
            else:
                ret[arch] |= boards

    return ret


def find_arches(args):
    arch_set = set()

    for root in unique_paths(args.arch_roots):
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


def find_arch2board_set_in(root, arches, board_dir):
    ret = defaultdict(set)
    boards = root / 'boards'

    for arch in arches:
        if not (boards / arch).is_dir():
            continue
        for maybe_board in (boards / arch).iterdir():
            if not maybe_board.is_dir():
                continue
            if board_dir and maybe_board not in board_dir:
                continue
            for maybe_defconfig in maybe_board.iterdir():
                file_name = maybe_defconfig.name
                if file_name.endswith('_defconfig') and not (maybe_board / BOARD_YML).is_file():
                    board_name = file_name[:-len('_defconfig')]
                    ret[arch].add(Board(board_name, maybe_board, 'v1', arch=arch))

    return ret


def load_v2_boards(board_name, board_yml, systems):
    boards = {}
    board_extensions = []
    if board_yml.is_file():
        with board_yml.open('r', encoding='utf-8') as f:
            b = yaml.load(f.read(), Loader=SafeLoader)

        try:
            pykwalify.core.Core(source_data=b, schema_data=board_schema).validate()
        except pykwalify.errors.SchemaError as e:
            sys.exit('ERROR: Malformed "build" section in file: {}\n{}'
                     .format(board_yml.as_posix(), e))

        mutual_exclusive = {'board', 'boards'}
        if len(mutual_exclusive - b.keys()) < 1:
            sys.exit(f'ERROR: Malformed content in file: {board_yml.as_posix()}\n'
                     f'{mutual_exclusive} are mutual exclusive at this level.')

        board_array = b.get('boards', [b.get('board', None)])
        for board in board_array:
            mutual_exclusive = {'name', 'extend'}
            if len(mutual_exclusive - board.keys()) < 1:
                sys.exit(f'ERROR: Malformed "board" section in file: {board_yml.as_posix()}\n'
                         f'{mutual_exclusive} are mutual exclusive at this level.')

            # This is a extending an existing board, place in array to allow later processing.
            if 'extend' in board:
                board.update({'dir': board_yml.parent})
                board_extensions.append(board)
                continue

            # Create board
            if board_name is not None:
                if board['name'] != board_name:
                    # Not the board we're looking for, ignore.
                    continue

            board_revision = board.get('revision')
            if board_revision is not None and board_revision.get('format') != 'custom':
                if board_revision.get('default') is None:
                    sys.exit(f'ERROR: Malformed "board" section in file: {board_yml.as_posix()}\n'
                             "Cannot find required key 'default'. Path: '/board/revision.'")
                if board_revision.get('revisions') is None:
                    sys.exit(f'ERROR: Malformed "board" section in file: {board_yml.as_posix()}\n'
                             "Cannot find required key 'revisions'. Path: '/board/revision.'")

            mutual_exclusive = {'socs', 'variants'}
            if len(mutual_exclusive - board.keys()) < 1:
                sys.exit(f'ERROR: Malformed "board" section in file: {board_yml.as_posix()}\n'
                         f'{mutual_exclusive} are mutual exclusive at this level.')
            socs = [Soc.from_soc(systems.get_soc(s['name']), s.get('variants', []))
                    for s in board.get('socs', {})]

            boards[board['name']] = Board(
                name=board['name'],
                directories=[board_yml.parent],
                vendor=board.get('vendor'),
                full_name=board.get('full_name'),
                revision_format=board.get('revision', {}).get('format'),
                revision_default=board.get('revision', {}).get('default'),
                revision_exact=board.get('revision', {}).get('exact', False),
                revisions=[Revision.from_dict(v) for v in
                           board.get('revision', {}).get('revisions', [])],
                socs=socs,
                variants=[Variant.from_dict(v) for v in board.get('variants', [])],
                hwm='v2',
            )
            board_qualifiers = board_v2_qualifiers(boards[board['name']])
            duplicates = [q for q, n in Counter(board_qualifiers).items() if n > 1]
            if duplicates:
                sys.exit(f'ERROR: Duplicated board qualifiers detected {duplicates} for board: '
                         f'{board["name"]}.\nPlease check content of: {board_yml.as_posix()}\n')
    return boards, board_extensions


def extend_v2_boards(boards, board_extensions):
    for e in board_extensions:
        board = boards.get(e['extend'])
        if board is None:
            continue
        board.directories.append(e['dir'])

        for v in e.get('variants', []):
            node = board.from_qualifier(v['qualifier'])
            if str(v['qualifier'] + '/' + v['name']) in board_v2_qualifiers(board):
                board_yml = e['dir'] / BOARD_YML
                sys.exit(f'ERROR: Variant: {v["name"]}, defined multiple times for board: '
                         f'{board.name}.\nLast defined in {board_yml}')
            node.variants.append(Variant.from_dict(v))


# Note that this does not share the args.board functionality of find_v2_boards
def find_v2_board_dirs(args):
    dirs = []
    board_files = []
    for root in unique_paths(args.board_roots):
        board_files.extend((root / 'boards').rglob(BOARD_YML))

    dirs = [board_yml.parent for board_yml in board_files if board_yml.is_file()]
    return dirs


def find_v2_boards(args):
    root_args = argparse.Namespace(**{'soc_roots': args.soc_roots})
    systems = list_hardware.find_v2_systems(root_args)

    boards = {}
    board_extensions = []
    board_files = []
    if args.board_dir:
        board_files = [d / BOARD_YML for d in args.board_dir]
    else:
        for root in unique_paths(args.board_roots):
            board_files.extend((root / 'boards').rglob(BOARD_YML))

    for board_yml in board_files:
        b, e = load_v2_boards(args.board, board_yml, systems)
        conflict_boards = set(boards.keys()).intersection(b.keys())
        if conflict_boards:
            sys.exit(f'ERROR: Board(s): {conflict_boards}, defined multiple times.\n'
                     f'Last defined in {board_yml}')
        boards.update(b)
        board_extensions.extend(e)

    extend_v2_boards(boards, board_extensions)
    return boards


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    add_args(parser)
    add_args_formatting(parser)
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
    parser.add_argument("--soc-root", dest='soc_roots', default=[],
                        type=Path, action='append',
                        help='add a soc root, may be given more than once')
    parser.add_argument("--board", dest='board', default=None,
                        help='lookup the specific board, fail if not found')
    parser.add_argument("--board-dir", default=[], type=Path, action='append',
                        help='Only look for boards at the specific location')


def add_args_formatting(parser):
    parser.add_argument("--cmakeformat", default=None,
                        help='''CMake Format string to use to list each board''')


def variant_v2_qualifiers(variant, qualifiers = None):
    qualifiers_list = [variant.name] if qualifiers is None else [qualifiers + '/' + variant.name]
    for v in variant.variants:
        qualifiers_list.extend(variant_v2_qualifiers(v, qualifiers_list[0]))
    return qualifiers_list


def board_v2_qualifiers(board):
    qualifiers_list = []

    for s in board.socs:
        if s.cpuclusters:
            for c in s.cpuclusters:
                id_str = s.name + '/' + c.name
                qualifiers_list.append(id_str)
                for v in c.variants:
                    qualifiers_list.extend(variant_v2_qualifiers(v, id_str))
        else:
            qualifiers_list.append(s.name)
            for v in s.variants:
                qualifiers_list.extend(variant_v2_qualifiers(v, s.name))

    for v in board.variants:
        qualifiers_list.extend(variant_v2_qualifiers(v))
    return qualifiers_list


def board_v2_qualifiers_csv(board):
    # Return in csv (comma separated value) format
    return ",".join(board_v2_qualifiers(board))


def dump_v2_boards(args):
    boards = find_v2_boards(args)

    for b in boards.values():
        qualifiers_list = board_v2_qualifiers(b)
        if args.cmakeformat is not None:
            notfound = lambda x: x or 'NOTFOUND'
            info = args.cmakeformat.format(
                NAME='NAME;' + b.name,
                DIR='DIR;' + ';'.join(
                    [str(x.as_posix()) for x in b.directories]),
                VENDOR='VENDOR;' + notfound(b.vendor),
                HWM='HWM;' + b.hwm,
                REVISION_DEFAULT='REVISION_DEFAULT;' + notfound(b.revision_default),
                REVISION_FORMAT='REVISION_FORMAT;' + notfound(b.revision_format),
                REVISION_EXACT='REVISION_EXACT;' + str(b.revision_exact),
                REVISIONS='REVISIONS;' + ';'.join(
                          [x.name for x in b.revisions]),
                SOCS='SOCS;' + ';'.join([s.name for s in b.socs]),
                QUALIFIERS='QUALIFIERS;' + ';'.join(qualifiers_list)
            )
            print(info)
        else:
            print(f'{b.name}')


def dump_boards(args):
    arch2boards = find_arch2boards(args)
    for arch, boards in arch2boards.items():
        if args.cmakeformat is None:
            print(f'{arch}:')
        for board in boards:
            if args.cmakeformat is not None:
                info = args.cmakeformat.format(
                    NAME='NAME;' + board.name,
                    DIR='DIR;' + str(board.dir.as_posix()),
                    HWM='HWM;' + board.hwm,
                    VENDOR='VENDOR;NOTFOUND',
                    REVISION_DEFAULT='REVISION_DEFAULT;NOTFOUND',
                    REVISION_FORMAT='REVISION_FORMAT;NOTFOUND',
                    REVISION_EXACT='REVISION_EXACT;NOTFOUND',
                    REVISIONS='REVISIONS;NOTFOUND',
                    VARIANT_DEFAULT='VARIANT_DEFAULT;NOTFOUND',
                    SOCS='SOCS;',
                    QUALIFIERS='QUALIFIERS;'
                )
                print(info)
            else:
                print(f'  {board.name}')


if __name__ == '__main__':
    args = parse_args()
    dump_boards(args)
    dump_v2_boards(args)
