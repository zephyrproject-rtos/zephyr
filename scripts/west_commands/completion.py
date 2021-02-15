# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os

from west import log
from west.commands import WestCommand

# Relative to the folder where this script lives
COMPLETION_REL_PATH = 'completion/west-completion'

class Completion(WestCommand):

    def __init__(self):
        super().__init__(
            'completion',
            # Keep this in sync with the string in west-commands.yml.
            'display shell completion scripts',
            'Display shell completion scripts.',
            accepts_unknown_args=False)

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description)

        # Remember to update west-completion.bash if you add or remove
        # flags
        parser.add_argument('shell', nargs=1, choices=['bash'],
                            help='''Select the shell that which the completion
                            script is intended for.
                            Currently only bash is supported.''')
        return parser

    def do_run(self, args, unknown_args):
        cf = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                          *COMPLETION_REL_PATH.split('/'))

        cf += '.' + args.shell[0]

        try:
            with open(cf, 'r') as f:
                print(f.read())
        except FileNotFoundError as e:
            log.die('Unable to find completion file: {}'.format(e))
