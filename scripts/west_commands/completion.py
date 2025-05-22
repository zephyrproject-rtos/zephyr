# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os

from west.commands import WestCommand

# Relative to the folder where this script lives
COMPLETION_REL_PATH = 'completion/west-completion'

COMP_DESCRIPTION = '''\
Output shell completion scripts for west.

This command outputs completion scripts for different shells by printing them
to stdout. Using the completion scripts:

  bash:
    # one-time
    source <(west completion bash)
    # permanent
    west completion bash > ~/west-completion.bash
    # edit your .bashrc or .bash_profile and add:
    source $HOME/west-completion.bash

  zsh:
    # one-time
    source <(west completion zsh)
    # permanent (might require sudo)
    west completion zsh > "${fpath[1]}/_west"

  fish:
    # one-time
    west completion fish | source
    # permanent
    west completion fish > $HOME/.config/fish/completions/west.fish

positional arguments:
  source_dir            application source directory
  cmake_opt             extra options to pass to cmake; implies -c
                        (these must come after "--" as shown above)
'''


class Completion(WestCommand):

    def __init__(self):
        super().__init__(
            'completion',
            # Keep this in sync with the string in west-commands.yml.
            'output shell completion scripts',
            COMP_DESCRIPTION,
            accepts_unknown_args=False)

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description)

        # Remember to update west-completion.bash if you add or remove
        # flags
        parser.add_argument('shell', nargs=1, choices=['bash', 'zsh', 'fish'],
                            help='''Shell that which the completion
                            script is intended for.''')
        return parser

    def do_run(self, args, unknown_args):
        cf = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                          *COMPLETION_REL_PATH.split('/'))

        cf += '.' + args.shell[0]

        try:
            with open(cf, 'r') as f:
                print(f.read())
        except FileNotFoundError as e:
            self.die('Unable to find completion file: {}'.format(e))
