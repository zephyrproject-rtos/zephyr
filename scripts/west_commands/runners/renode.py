# Copyright (c) 2024 Antmicro <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0

'''Runner stub for Renode.'''

import subprocess

from runners.core import RunnerCaps, ZephyrBinaryRunner


class RenodeRunner(ZephyrBinaryRunner):
    '''Place-holder for Renode runner customizations.'''

    def __init__(self, cfg, args):
        super().__init__(cfg)
        self.renode_arg = args.renode_arg
        self.renode_command = args.renode_command
        self.renode_help = args.renode_help

    @classmethod
    def name(cls):
        return 'renode'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'simulate'}, hide_load_files=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--renode-arg',
                            metavar='ARG',
                            action='append',
                            help='''additional argument passed to Renode;
                            `--help` will print all possible arguments''')
        parser.add_argument('--renode-command',
                            metavar='COMMAND',
                            action='append',
                            help='additional command passed to Renode\'s simulation')
        parser.add_argument('--renode-help',
                            default=False,
                            action='store_true',
                            help='print all possible `Renode` arguments')

    @classmethod
    def do_create(cls, cfg, args):
        return RenodeRunner(cfg, args)

    def do_run(self, command, **kwargs):
        self.run_test(**kwargs)

    def run_test(self, **kwargs):
        cmd = ['renode']
        if self.renode_help is True:
            cmd.append('--help')
        else:
            if self.renode_arg is not None:
                for arg in self.renode_arg:
                    cmd.append(arg)
            if self.renode_command is not None:
                for command in self.renode_command:
                    cmd.append('-e')
                    cmd.append(command)
        subprocess.run(cmd, check=True)
