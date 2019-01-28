# Copyright (c) 2018 Open Source Foundries Limited.
# Copyright 2019 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

'''west "debug" and "debugserver" commands.'''

from textwrap import dedent

from west.commands import WestCommand

from run_common import desc_common, add_parser_common, do_run_common


class Debug(WestCommand):

    def __init__(self):
        super(Debug, self).__init__(
            'debug',
            # Keep this in sync with the string in west-commands.yml.
            'flash and interactively debug a Zephyr application',
            dedent('''
            Connect to the board, program the flash, and start a
            debugging session.\n\n''') +
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
            # Keep this in sync with the string in west-commands.yml.
            'connect to board and launch a debug server',
            dedent('''
            Connect to the board and launch a debug server which accepts
            incoming connections for debugging the connected board.

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


class Attach(WestCommand):

    def __init__(self):
        super(Attach, self).__init__(
            'attach',
            # Keep this in sync with the string in west-commands.yml.
            'interactively debug a board',
            dedent('''
            Like 'debug', this connects to the board and starts a debugging
            session, but it doesn't reflash the program on the board.\n\n''') +
            desc_common('attach'),
            accepts_unknown_args=True)

    def do_add_parser(self, parser_adder):
        return add_parser_common(parser_adder, self)

    def do_run(self, my_args, runner_args):
        do_run_common(self, my_args, runner_args,
                      'ZEPHYR_BOARD_DEBUG_RUNNER')
