# Copyright (c) 2018 Open Source Foundries Limited.
# Copyright 2019 Foundries.io
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''west "flash" command'''

from west.commands import WestCommand

from run_common import add_parser_common, do_run_common, get_build_dir

from pathlib import Path


class Flash(WestCommand):

    def __init__(self):
        super(Flash, self).__init__(
            'flash',
            # Keep this in sync with the string in west-commands.yml.
            'flash and run a binary on a board',
            "Permanently reprogram a board's flash with a new binary.",
            accepts_unknown_args=True)
        self.runner_key = 'flash-runner'  # in runners.yaml

    def do_add_parser(self, parser_adder):
        return add_parser_common(self, parser_adder)

    def do_run(self, my_args, runner_args):
        build_dir = get_build_dir(my_args)
        domains_file = Path(build_dir) / 'domains.yaml'
        do_run_common(self, my_args, runner_args, domain_file=domains_file)
