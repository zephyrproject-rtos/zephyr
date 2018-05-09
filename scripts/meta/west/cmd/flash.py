# Copyright (c) 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''west "flash" command'''

from .run_common import desc_common, add_parser_common, do_run_common
from . import WestCommand


class Flash(WestCommand):

    def __init__(self):
        super(Flash, self).__init__(
            'flash',
            'Flash and run a binary onto a board.\n\n' +
            desc_common('flash'),
            accepts_unknown_args=True)

    def do_add_parser(self, parser_adder):
        return add_parser_common(parser_adder, self)

    def do_run(self, my_args, runner_args):
        do_run_common(self, my_args, runner_args,
                      'ZEPHYR_BOARD_FLASH_RUNNER')
