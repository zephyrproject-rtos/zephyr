# Copyright (c) 2026 Space Cubics Inc.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import sys
from pathlib import Path

from west.commands import WestCommand
from zcmake import CMakeCache
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

        enable_parser = subparsers.add_parser(
            "enable",
            help="enable a devicetree node",
            description="Enable an existing devicetree node by appending an overlay fragment.",
        )
        enable_parser.add_argument(
            "node",
            help="node label, node reference, or node path",
        )
        enable_parser.add_argument(
            "-o",
            "--overlay",
            default=None,
            help=(
                "overlay file to update, relative to the application "
                "source directory, default: app.overlay"
            ),
        )
        enable_parser.set_defaults(handler=self._run_enable)

        return parser

    def do_run(self, args, _unknown):
        args.handler(args)

    def _run_list(self, args):
        dt = self._load_dt()
        nodes = list(self._walk(dt.root))

        if args.long:
            self._print_long(nodes)
        else:
            for node in nodes:
                self.inf(self._format_path(node.path))

    def _run_enable(self, args):
        dt = self._load_dt()
        node = self._resolve_node(dt, args.node)

        if node is None:
            self.die(f"devicetree node not found: {args.node}")

        ref = self._node_ref(node)
        status = self._node_status(node)

        if status == "okay":
            self.inf(f"{ref} is already okay")
            return

        overlay = self._overlay_path(args.overlay)
        fragment = self._status_fragment(ref, "okay")

        self._append_overlay_fragment(overlay, fragment)

        self.inf(f"enabled {ref} in {overlay}")
        self.inf("Run west build to regenerate build/zephyr/zephyr.dts.")

    def _build_dir(self):
        return Path("build")

    def _load_dt(self):
        dts = self._build_dir() / "zephyr" / "zephyr.dts"

        if not dts.is_file():
            self.die(f"{dts} does not exist. Run west build first.")

        try:
            return dtlib.DT(os.fspath(dts))
        except dtlib.DTError as e:
            self.die(f"failed to parse {dts}: {e}")
        except OSError as e:
            self.die(f"failed to open {dts}: {e}")

    def _load_cmake_cache(self):
        cache_file = self._build_dir() / "CMakeCache.txt"

        if not cache_file.is_file():
            self.die(f"{cache_file} does not exist. Run west build first.")

        return CMakeCache.from_build_dir(os.fspath(self._build_dir()))

    def _application_source_dir(self):
        cache = self._load_cmake_cache()
        app_dir = cache.get("APP_DIR")

        if not app_dir:
            app_dir = cache.get("APPLICATION_SOURCE_DIR")

        if not app_dir:
            app_dir = cache.get("CMAKE_HOME_DIRECTORY")

        if not app_dir:
            self.die("CMake cache does not contain the application source directory.")

        return Path(app_dir)

    def _overlay_path(self, overlay):
        app_dir = self._application_source_dir()

        if overlay is None:
            return app_dir / "app.overlay"

        overlay = Path(overlay)
        if overlay.is_absolute():
            return overlay

        return app_dir / overlay

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

    def _resolve_node(self, dt, node_id):
        label = None
        path = None
        node_id = node_id.strip()

        if node_id == ".":
            path = "/"
        elif node_id.startswith("./"):
            path = "/" + node_id[2:]
        elif node_id.startswith("&{") and node_id.endswith("}"):
            path = node_id[2:-1]
        elif node_id.startswith("&"):
            label = node_id[1:]
        elif node_id.startswith("/"):
            path = node_id
        else:
            label = node_id

        for node in self._walk(dt.root):
            if path is not None and node.path == path:
                return node

            if label is not None and label in node.labels:
                return node

        return None

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

    def _status_fragment(self, ref, status):
        return f'{ref} {{\n\tstatus = "{status}";\n}};\n'

    def _append_overlay_fragment(self, overlay, fragment):
        old = ""

        if overlay.exists():
            old = overlay.read_text(encoding="utf-8")

        if fragment in old:
            self.inf(f"{overlay} already contains the requested fragment")
            return

        overlay.parent.mkdir(parents=True, exist_ok=True)

        with overlay.open("a", encoding="utf-8") as f:
            if old and not old.endswith("\n"):
                f.write("\n")

            if old:
                f.write("\n")

            f.write(fragment)
