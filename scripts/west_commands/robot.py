# Copyright (c) 2024 Antmicro <www.antmicro.com>
#
# SPDX-License-Identifier: Apache-2.0

from west.commands import WestCommand
from run_common import add_parser_common, do_run_common

EXPORT_DESCRIPTION = '''\
Run RobotFramework test suites with a runner of choice.
'''


class Robot(WestCommand):

    def __init__(self):
        super(Robot, self).__init__(
            'robot',
            # Keep this in sync with the string in west-commands.yml.
            'run RobotFramework test suites',
            EXPORT_DESCRIPTION,
            accepts_unknown_args=True)

        self.runner_key = 'robot-runner'  # in runners.yaml

    def do_add_parser(self, parser_adder):
        return add_parser_common(self, parser_adder)

    def do_run(self, my_args, runner_args):
        do_run_common(self, my_args, runner_args)
