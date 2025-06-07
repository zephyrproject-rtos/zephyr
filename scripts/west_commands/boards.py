# Copyright (c) 2019 Nordic Semiconductor ASA
# Copyright (c) 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import re
import sys
import textwrap
from pathlib import Path

from west.commands import WestCommand
from zephyr_ext_common import ZEPHYR_BASE, ZEPHYR_SCRIPTS

sys.path.append(os.fspath(Path(__file__).parent.parent))

import list_boards
import zephyr_module

# Resolve path to twister libs and add imports
twister_path = ZEPHYR_SCRIPTS
os.environ["ZEPHYR_BASE"] = str(twister_path.parent)

sys.path.insert(0, str(twister_path))
sys.path.insert(0, str(twister_path / "pylib" / "twister"))

from twisterlib.platform import generate_platforms  # noqa: E402


class Boards(WestCommand):

    def __init__(self):
        super().__init__(
            'boards',
            # Keep this in sync with the string in west-commands.yml.
            'display information about supported boards',
            'Display information about boards',
            accepts_unknown_args=False)

    def do_add_parser(self, parser_adder):
        default_fmt = '{name}'
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description,
            epilog=textwrap.dedent(f'''\
            FORMAT STRINGS
            --------------

            Boards are listed using a Python 3 format string. Arguments
            to the format string are accessed by name.

            The default format string is:

            "{default_fmt}"

            The following arguments are available:

            - name: board name
            - full_name: board full name (typically, its commercial name)
            - revision_default: board default revision
            - revisions: list of board revisions
            - qualifiers: board qualifiers (will be empty for legacy boards)
            - arch: board architecture
            - dir: directory that contains the board definition
            - vendor: board vendor
            - aliases: board aliases
            '''))

        # Remember to update west-completion.bash if you add or remove
        # flags
        parser.add_argument('-f', '--format', default=default_fmt,
                            help='''Format string to use to list each board;
                                    see FORMAT STRINGS below.''')
        parser.add_argument('-n', '--name', dest='name_re',
                            help='''a regular expression; only boards whose
                            names match NAME_RE will be listed''')
        list_boards.add_args(parser)

        return parser

    def do_run(self, args, _):
        if args.name_re is not None:
            name_re = re.compile(args.name_re)
        else:
            name_re = None

        module_settings = {
            'arch_root': [ZEPHYR_BASE],
            'board_root': [ZEPHYR_BASE],
            'soc_root': [ZEPHYR_BASE],
        }

        for module in zephyr_module.parse_modules(ZEPHYR_BASE, self.manifest):
            for key in module_settings:
                root = module.meta.get('build', {}).get('settings', {}).get(key)
                if root is not None:
                    module_settings[key].append(Path(module.project) / root)

        args.arch_roots += module_settings['arch_root']
        args.board_roots += module_settings['board_root']
        args.soc_roots += module_settings['soc_root']

        board = {}
        for platform in generate_platforms(args.board_roots, args.soc_roots, args.arch_roots):
            if not platform.twister:
                continue
            if name_re is not None and not name_re.search(" ".join(platform.aliases)):
                continue

            if platform.board.revisions:
                revisions_list = ' '.join([rev.name for rev in platform.board.revisions])
            else:
                revisions_list = 'None'

            board_name = platform.board.name
            if board_name not in board:
                board_desc = {}
                board_desc["full_name"] = platform.board.full_name
                board_desc["revision_default"] =  platform.board.revision_default
                board_desc["revisions_list"] = revisions_list
                board_desc["dir"] = platform.board.dir
                board_desc["hwm"] = platform.board.hwm
                board_desc["vendor"] = platform.vendor
                board_desc["qual"] = ["/".join(platform.name.split('/')[1:])]
                board_desc["arch"] = [platform.arch]
                board_desc["aliases"] = platform.aliases
                board[board_name] = board_desc
            else:
                board_desc =  board[board_name]
                board_desc["qual"] += ["/".join(platform.name.split('/')[1:])]
                board_desc["arch"] += [platform.arch]
                board_desc["aliases"] += platform.aliases

        if not board.keys():
            print("no board found")

        try:
            for _name, _b in board.items():
                self.inf(
                    args.format.format(
                        name=_name,
                        full_name=_b["full_name"],
                        revision_default=_b["revision_default"],
                        revisions=_b["revisions_list"],
                        dir=_b["dir"],
                        hwm=_b["hwm"],
                        vendor=_b["vendor"],
                        qualifiers=",".join(set(_b["qual"])),
                        arch=",".join(set(_b["arch"])),
                        aliases=",".join(set(_b["aliases"])),
                    )
                )
        except KeyError:
            print(f"KeyError - reason -f {args.format} is wrong")
            print("use 'west boards -h' for supporting format")
