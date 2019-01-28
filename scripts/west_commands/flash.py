# Copyright (c) 2018 Open Source Foundries Limited.
# Copyright 2019 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

'''west "flash" command'''

from textwrap import dedent

from west.commands import WestCommand

from run_common import desc_common, add_parser_common, do_run_common


class Flash(WestCommand):

    def __init__(self):
        super(Flash, self).__init__(
            'flash',
            # Keep this in sync with the string in west-commands.yml.
            'flash and run a binary on a board',
            dedent('''
            Connects to the board and reprograms it with a new binary\n\n''') +
            desc_common('flash'),
            accepts_unknown_args=True)

    def do_add_parser(self, parser_adder):
        return add_parser_common(parser_adder, self)

    def do_run(self, my_args, runner_args):
        do_run_common(self, my_args, runner_args,
                      'ZEPHYR_BOARD_FLASH_RUNNER')
