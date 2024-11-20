# Copyright (c) 2024 Antmicro <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0

'''Runner stub for renode-test.'''

import subprocess

from runners.core import RunnerCaps, ZephyrBinaryRunner


class RenodeRobotRunner(ZephyrBinaryRunner):
    '''Place-holder for Renode runner customizations.'''

    def __init__(self, cfg, args):
        super().__init__(cfg)
        self.testsuite = args.testsuite
        self.renode_robot_arg = args.renode_robot_arg
        self.renode_robot_help = args.renode_robot_help

    @classmethod
    def name(cls):
        return 'renode-robot'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'robot'}, hide_load_files=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--testsuite',
                            metavar='SUITE',
                            action='append',
                            help='path to Robot test suite')
        parser.add_argument('--renode-robot-arg',
                            metavar='ARG',
                            action='append',
                            help='additional argument passed to renode-test')
        parser.add_argument('--renode-robot-help',
                            default=False,
                            action='store_true',
                            help='print all possible `renode-test` arguments')

    @classmethod
    def do_create(cls, cfg, args):
        return RenodeRobotRunner(cfg, args)

    def do_run(self, command, **kwargs):
        self.run_test(**kwargs)

    def run_test(self, **kwargs):
        cmd = ['renode-test']
        if self.renode_robot_help is True:
            cmd.append('--help')
        else:
            if self.renode_robot_arg is not None:
                for arg in self.renode_robot_arg:
                    cmd.append(arg)
            if self.testsuite is not None:
                for suite in self.testsuite:
                    cmd.append(suite)
            else:
                self.logger.error("No Robot testsuite passed to renode-test! Use the `--testsuite` argument to provide one.")
        subprocess.run(cmd, check=True)
