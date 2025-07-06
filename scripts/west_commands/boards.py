# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import re
import sys
import textwrap
from pathlib import Path

from west.commands import WestCommand
from zephyr_ext_common import ZEPHYR_BASE

sys.path.append(os.fspath(Path(__file__).parent.parent))
import list_boards
import zephyr_module


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
            - arch: board architecture (deprecated)
                    (arch is ambiguous for boards described in new hw model)
            - dir: directory that contains the board definition
            - vendor: board vendor
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

        for board in list_boards.find_boards(args):
            if name_re is not None and not name_re.search(board.name):
                continue

            if board.revisions:
                revisions_list = ' '.join([rev.name for rev in board.revisions])
            else:
                revisions_list = 'None'

            self.inf(args.format.format(name=board.name, arch=board.arch,
                                        revision_default=board.revision_default,
                                        revisions=revisions_list,
                                        dir=board.dir, hwm=board.hwm, qualifiers=''))

        for board in list_boards.find_v2_boards(args).values():
            if name_re is not None and not name_re.search(board.name):
                continue

            if board.revisions:
                revisions_list = ' '.join([rev.name for rev in board.revisions])
            else:
                revisions_list = 'None'

            self.inf(
                args.format.format(
                    name=board.name,
                    full_name=board.full_name,
                    revision_default=board.revision_default,
                    revisions=revisions_list,
                    dir=board.dir,
                    hwm=board.hwm,
                    vendor=board.vendor,
                    qualifiers=list_boards.board_v2_qualifiers_csv(board),
                )
            )
