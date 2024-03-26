# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""West command to open the devicetree shell (dtsh)."""


import argparse
import os
import sys

from west.commands import WestCommand
from west import log

from zephyr_ext_common import ZEPHYR_SCRIPTS


# DTSh dependencies that are not pip-installed when
# initializing the West workspace.
sys.path.insert(0, str(ZEPHYR_SCRIPTS / "dts" / "python-devicetree" / "src"))
sys.path.insert(1, str(ZEPHYR_SCRIPTS / "dts" / "dtsh" / "src"))

from dtsh.shell import DTShError
from dtsh.config import DTShConfig
from dtsh.rich.theme import DTShTheme
from dtsh.rich.session import DTShRichSession


DTSH_DESCRIPTION = """\
Shell-like interface with Devicetree:

- walk a devicetree through a hierarchical file-system metaphor
- search for nodes with flexible criteria
- filter, sort and format commands output
- generate simple documentation artifacts (text, HTML, SVG)
  by redirecting commands output to files
- rich Textual User Interface, command line auto-completion,
  command history, actionable text, user themes
"""


class DTShell(WestCommand):
    """Devicetree shell West command."""

    def __init__(self) -> None:
        super().__init__(
            "dtsh",
            "devicetree shell",
            DTSH_DESCRIPTION,
            accepts_unknown_args=False,
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description,
        )

        args_grp = parser.add_argument_group("open a DTS file")
        args_grp.add_argument(
            "-b",
            "--bindings",
            help="directory to search for binding files",
            action="append",
            metavar="DIR",
        )
        args_grp.add_argument(
            "dts", help="path to the DTS file", nargs="?", metavar="DTS"
        )

        args_grp = parser.add_argument_group("user files")
        args_grp.add_argument(
            "-u",
            "--user-files",
            help="initialize per-user configuration files and exit",
            action="store_true",
        )
        args_grp.add_argument(
            "--preferences",
            help="load additional preferences file",
            nargs=1,
            metavar="FILE",
        )
        args_grp.add_argument(
            "--theme",
            help="load additional theme file",
            nargs=1,
            metavar="FILE",
        )

        return parser

    def do_run(self, args, unknown) -> None:
        del unknown  # Unused.

        if args.user_files:
            # Initialize per-user configuration files and exit.
            ret = DTShConfig.getinstance().init_user_files()
            sys.exit(ret)

        if args.preferences:
            # Load additional preferences file.
            path = args.preferences[0]
            try:
                DTShConfig.getinstance().load_ini_file(path)
            except DTShConfig.Error as e:
                log.err(str(e))
                log.die(f"Failed to load preferences file: {path}")

        if args.theme:
            # Load additional theme file.
            path = args.theme[0]
            try:
                DTShTheme.getinstance().load_theme_file(path)
            except DTShTheme.Error as e:
                log.err(str(e))
                log.die(f"Failed to load styles file: {path}")

        dts: str = args.dts or os.path.join(
            os.path.abspath("build"), "zephyr", "zephyr.dts"
        )
        binding_dirs = args.bindings

        try:
            # Initialize DT model, shell, etc.
            session = DTShRichSession.create(dts, binding_dirs)
        except DTShError as e:
            log.err(e.msg)
            log.die("Failed to initialize devicetree.")

        # Enter the Devicetree shell interactive session loop.
        session.run()
