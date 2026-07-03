# Copyright (c) 2026 Space Cubics Inc.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import sys
from pathlib import Path

from west.commands import WestCommand
from zephyr_ext_common import ZEPHYR_SCRIPTS

PYTHON_DEVICETREE = ZEPHYR_SCRIPTS / "dts" / "python-devicetree" / "src"

# Use Zephyr's in-tree python-devicetree. This import must come after
# updating sys.path because python-devicetree is not installed as a package.
sys.path.insert(0, str(PYTHON_DEVICETREE))

from devicetree import dtlib


class Dt(WestCommand):
    def __init__(self):
        super().__init__(
            "dt",
            "",
            description="manage Zephyr devicetree nodes",
            accepts_unknown_args=False,
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description,
        )

        subparsers = parser.add_subparsers(
            dest="subcommand",
            metavar="<subcommand>",
            required=True,
        )

        list_parser = subparsers.add_parser(
            "list",
            help="list devicetree nodes",
            description="List nodes from build/zephyr/zephyr.dts.",
        )
        list_parser.set_defaults(handler=self._run_list)

        return parser

    def do_run(self, args, _unknown):
        args.handler(args)

    def _run_list(self, _args):
        dts = Path("build") / "zephyr" / "zephyr.dts"

        if not dts.is_file():
            self.die(f"{dts} does not exist. Run west build first.")

        try:
            dt = dtlib.DT(os.fspath(dts))
        except dtlib.DTError as e:
            self.die(f"failed to parse {dts}: {e}")
        except OSError as e:
            self.die(f"failed to open {dts}: {e}")

        for node in self._walk(dt.root):
            self.inf(self._format_path(node.path))

    def _walk(self, node):
        yield node

        for child in node.nodes.values():
            yield from self._walk(child)

    def _format_path(self, path):
        if path == "/":
            return "."

        return f".{path}"
