# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
from pathlib import Path
import re
import sys
import textwrap

from west import log
from west.commands import WestCommand

sys.path.append(os.fspath(Path(__file__).parent.parent))
import list_boards

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
            - arch: board architecture
            - dir: directory that contains the board definition
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

        for board in list_boards.find_boards(args):
            if name_re is not None and not name_re.search(board.name):
                continue
            log.inf(args.format.format(name=board.name, arch=board.arch,
                                       dir=board.dir))
