# Copyright (c) 2023 Google
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import sys

from west.commands import Verbosity, WestCommand

from zephyr_ext_common import ZEPHYR_SCRIPTS

# Resolve path to twister libs and add imports
twister_path = ZEPHYR_SCRIPTS
os.environ["ZEPHYR_BASE"] = str(twister_path.parent)

sys.path.insert(0, str(twister_path))
sys.path.insert(0, str(twister_path / "pylib" / "twister"))

from twisterlib.environment import add_parse_arguments, parse_arguments, python_version_guard
from twisterlib.twister_main import main

TWISTER_DESCRIPTION = """\
Convenience wrapper for twister. The below options are shared with the twister
script and have the same effects as if you ran twister directly. Refer to the
twister documentation for more information.
"""


class Twister(WestCommand):
    def __init__(self):
        super(Twister, self).__init__(
            "twister",
            # Keep this in sync with the string in west-commands.yml.
            "west twister wrapper",
            TWISTER_DESCRIPTION,
            accepts_unknown_args=True,
        )
        python_version_guard()

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description,
            allow_abbrev=False
        )

        parser = add_parse_arguments(parser)

        return parser

    def do_run(self, args, remainder):
        self.dbg(
            "args: {} remainder: {}".format(args, remainder), level=Verbosity.DBG_EXTREME
        )

        options = parse_arguments(self.parser, args=remainder, options=args)
        default_options = parse_arguments(self.parser, args=[], on_init=False)
        ret = main(options, default_options)
        sys.exit(ret)

    def _parse_arguments(self, args, options):
        """Helper function for testing purposes"""
        return parse_arguments(self.parser, args, options)
