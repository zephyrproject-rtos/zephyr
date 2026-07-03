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
        list_parser.add_argument(
            "-l",
            "--long",
            action="store_true",
            help="show node reference and status",
        )
        list_parser.set_defaults(handler=self._run_list)

        return parser

    def do_run(self, args, _unknown):
        args.handler(args)

    def _run_list(self, args):
        dts = Path("build") / "zephyr" / "zephyr.dts"

        if not dts.is_file():
            self.die(f"{dts} does not exist. Run west build first.")

        try:
            dt = dtlib.DT(os.fspath(dts))
        except dtlib.DTError as e:
            self.die(f"failed to parse {dts}: {e}")
        except OSError as e:
            self.die(f"failed to open {dts}: {e}")

        nodes = list(self._walk(dt.root))

        if args.long:
            self._print_long(nodes)
        else:
            for node in nodes:
                self.inf(self._format_path(node.path))

    def _print_long(self, nodes):
        rows = [
            (
                self._node_ref(node),
                self._node_status(node),
                self._format_path(node.path),
            )
            for node in nodes
        ]

        ref_width = max(len(ref) for ref, _status, _path in rows)
        status_width = max(len(status) for _ref, status, _path in rows)

        for ref, status, path in rows:
            self.inf(f"{ref:<{ref_width}}  {status:<{status_width}}  {path}")

    def _walk(self, node):
        yield node

        for child in node.nodes.values():
            yield from self._walk(child)

    def _format_path(self, path):
        if path == "/":
            return "."

        return f".{path}"

    def _node_ref(self, node):
        if node.labels:
            return f"&{node.labels[0]}"

        return f"&{{{node.path}}}"

    def _node_status(self, node):
        prop = node.props.get("status")
        if prop is None:
            return "okay"

        try:
            status = prop.to_string()
        except dtlib.DTError:
            return "<invalid>"

        if status == "ok":
            return "okay"

        return status
