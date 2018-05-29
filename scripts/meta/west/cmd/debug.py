# Copyright (c) 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''west "debug" and "debugserver" commands.'''

from textwrap import dedent

from .run_common import desc_common, add_parser_common, do_run_common
from . import WestCommand


class Debug(WestCommand):

    def __init__(self):
        super(Debug, self).__init__(
            'debug',
            'Connect to the board and start a debugging session.\n\n' +
            desc_common('debug'),
            accepts_unknown_args=True)

    def do_add_parser(self, parser_adder):
        return add_parser_common(parser_adder, self)

    def do_run(self, my_args, runner_args):
        do_run_common(self, my_args, runner_args,
                      'ZEPHYR_BOARD_DEBUG_RUNNER')


class DebugServer(WestCommand):

    def __init__(self):
        super(DebugServer, self).__init__(
            'debugserver',
            dedent('''
            Connect to the board and accept debug networking connections.

            The debug server binds to a known port, and allows client software
            started elsewhere to connect to it and debug the running
            Zephyr image.\n\n''') +
            desc_common('debugserver'),
            accepts_unknown_args=True)

    def do_add_parser(self, parser_adder):
        return add_parser_common(parser_adder, self)

    def do_run(self, my_args, runner_args):
        do_run_common(self, my_args, runner_args,
                      'ZEPHYR_BOARD_DEBUG_RUNNER')
