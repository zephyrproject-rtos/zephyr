# Copyright (c) 2024 Vestas Wind Systems A/S
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
import list_shields
import zephyr_module


class Shields(WestCommand):

    def __init__(self):
        super().__init__(
            'shields',
            # Keep this in sync with the string in west-commands.yml.
            'display list of supported shield',
            'Display supported shields',
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

            Shields are listed using a Python 3 format string. Arguments
            to the format string are accessed by name.

            The default format string is:

            "{default_fmt}"

            The following arguments are available:

            - name: shield name
            - dir: directory that contains the shield definition
            '''))

        # Remember to update west-completion.bash if you add or remove
        # flags
        parser.add_argument('-f', '--format', default=default_fmt,
                            help='''Format string to use to list each shield;
                                    see FORMAT STRINGS below.''')
        parser.add_argument('-n', '--name', dest='name_re',
                            help='''a regular expression; only shields whose
                            names match NAME_RE will be listed''')
        list_shields.add_args(parser)

        return parser

    def do_run(self, args, _):
        if args.name_re is not None:
            name_re = re.compile(args.name_re)
        else:
            name_re = None

        modules_board_roots = [ZEPHYR_BASE]

        for module in zephyr_module.parse_modules(ZEPHYR_BASE, self.manifest):
            board_root = module.meta.get('build', {}).get('settings', {}).get('board_root')
            if board_root is not None:
                modules_board_roots.append(Path(module.project) / board_root)

        args.board_roots += modules_board_roots

        for shield in list_shields.find_shields(args):
            if name_re is not None and not name_re.search(shield.name):
                continue
            self.inf(args.format.format(name=shield.name, dir=shield.dir))
