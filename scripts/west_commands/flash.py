# Copyright (c) 2018 Open Source Foundries Limited.
# Copyright 2019 Foundries.io
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''west "flash" command'''

from west.commands import WestCommand

from run_common import desc_common, add_parser_common, do_run_common


class Flash(WestCommand):

    def __init__(self):
        super(Flash, self).__init__(
            'flash',
            # Keep this in sync with the string in west-commands.yml.
            'flash and run a binary on a board',
            'Permanently reprogram a board with a new binary.\n' +
            desc_common('flash'),
            accepts_unknown_args=True)
        self.runner_key = 'flash-runner'  # in runners.yaml

    def do_add_parser(self, parser_adder):
        return add_parser_common(self, parser_adder)

    def do_run(self, my_args, runner_args):
        do_run_common(self, my_args, runner_args)
