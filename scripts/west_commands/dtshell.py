# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""West command to open the devicetree shell (dtsh)."""

from typing import List
import argparse
import sys

from west.commands import WestCommand

from zephyr_ext_common import ZEPHYR_SCRIPTS

# DTSh dependencies that are not pip-installed when
# initializing the West workspace.
sys.path.insert(0, str(ZEPHYR_SCRIPTS / "dts" / "python-devicetree" / "src"))
sys.path.insert(1, str(ZEPHYR_SCRIPTS / "dts" / "dtsh" / "src"))

from dtsh.shell import DTShError  # noqa
from dtsh.cli import DTShArgvParser, DTShCli  # noqa


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

    _parser: argparse.ArgumentParser

    def __init__(self) -> None:
        super().__init__(
            "dtsh",
            "devicetree shell",
            DTSH_DESCRIPTION,
            accepts_unknown_args=False,
        )

    def do_add_parser(self, parser_adder) -> argparse.ArgumentParser:
        self._parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description,
        )
        DTShArgvParser.init(self._parser)
        return self._parser

    def do_run(self, args: argparse.Namespace, unknown: List[str]) -> None:
        del unknown  # Unused.

        cli = DTShCli(self._parser)
        try:
            cli.run(args)
        except DTShError as e:
            self.err(e.msg)
            self.die("Failed to initialize DTSh session, try 'west dtsh -h'.")
