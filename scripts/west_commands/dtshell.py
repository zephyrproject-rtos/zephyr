# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""West command to open the devicetree shell (dtsh)."""

import argparse
import os
import sys

from west.commands import WestCommand
from zephyr_ext_common import ZEPHYR_SCRIPTS

# DTSh dependencies that are not pip-installed when
# initializing the West workspace.
sys.path.append(os.fspath(ZEPHYR_SCRIPTS / "dts" / "python-devicetree" / "src"))
sys.path.append(os.fspath(ZEPHYR_SCRIPTS / "dts" / "dtsh" / "src"))

from dtsh.cli import DTShArgvParser, DTShCli
from dtsh.shell import DTShError

DTSH_DESCRIPTION = """\
Shell-like interface with Devicetree:

- navigate and visualize the devicetree through a hierarchical
  file-system metaphor
- search for devices, bindings, buses or interrupts with flexible criteria
- redirect command output to files (text, HTML, SVG) to document
  hardware configurations or illustrate notes
- rich CLI, command line auto-completion, command history, user themes
"""


class DTShell(WestCommand):
    """Devicetree shell West command."""

    _parser: argparse.ArgumentParser

    def __init__(self) -> None:
        super().__init__(
            "dtsh",
            "Devicetree shell",
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

    def do_run(self, args: argparse.Namespace, unknown: list[str]) -> None:
        del unknown  # Unused.

        cli = DTShCli(self._parser)
        try:
            cli.run(args)
        except DTShError as e:
            self.err(e.msg)
            self.die("Failed to initialize DTSh session, try 'west dtsh -h'.")
