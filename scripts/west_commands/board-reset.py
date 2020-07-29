# Copyright (c) 2018 Open Source Foundries Limited.
# Copyright 2019 Foundries.io
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''west "board-reset" command'''

from west.commands import WestCommand

from run_common import add_parser_common, do_run_common


class BoardReset(WestCommand):

    def __init__(self):
        super(BoardReset, self).__init__(
            'board-reset',
            # Keep this in sync with the string in west-commands.yml.
            'reset a board',
            "Reset a board.",
            accepts_unknown_args=True)
        self.runner_key = 'reset-runner'  # in runners.yaml

    def do_add_parser(self, parser_adder):
        return add_parser_common(self, parser_adder)

    def do_run(self, my_args, runner_args):
        do_run_common(self, my_args, runner_args)
