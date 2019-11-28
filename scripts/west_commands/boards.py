# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import collections
import os
import re
import textwrap

from west import log
from west.commands import WestCommand
from zcmake import run_cmake

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
            epilog=textwrap.dedent('''\
            FORMAT STRINGS

            Boards are listed using a Python 3 format string. Arguments
            to the format string are accessed by name.

            The default format string is:

            "{}"

            The following arguments are available:

            - name: board name
            - arch: board architecture
            '''.format(default_fmt)))

        # Remember to update scripts/west-completion.bash if you add or remove
        # flags
        parser.add_argument('-f', '--format', default=default_fmt,
                            help='''Format string to use to list each board;
                                    see FORMAT STRINGS below.''')

        return parser

    def do_run(self, args, unknown_args):
        zb = os.environ.get('ZEPHYR_BASE')
        if not zb:
            log.die('Internal error: ZEPHYR_BASE not set in the environment, '
                    'and should have been by the main script')

        cmake_args = ['-DBOARD_ROOT_SPACE_SEPARATED={}'.format(zb),
                      '-P', '{}/cmake/boards.cmake'.format(zb)]
        lines = run_cmake(cmake_args, capture_output=True)
        arch_re = re.compile(r'\s*([\w-]+)\:')
        board_re = re.compile(r'\s*([\w-]+)\s*')
        arch = None
        boards = collections.OrderedDict()
        for line in lines:
            match = arch_re.match(line)
            if match:
                arch = match.group(1)
                boards[arch] = []
                continue
            match = board_re.match(line)
            if match:
                if not arch:
                    log.die('Invalid board output from CMake: {}'.
                            format(lines))
                board = match.group(1)
                boards[arch].append(board)

        for arch in boards:
            for board in boards[arch]:
                try:
                    result = args.format.format(
                        name=board,
                        arch=arch)
                    print(result)
                except KeyError as e:
                    # The raised KeyError seems to just put the first
                    # invalid argument in the args tuple, regardless of
                    # how many unrecognizable keys there were.
                    log.die('unknown key "{}" in format string "{}"'.
                            format(e.args[0], args.format))
