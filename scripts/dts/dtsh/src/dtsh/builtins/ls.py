# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell built-in "ls".

List branch contents.

Unit tests and examples: tests/test_dtsh_builtin_ls.py
"""


from typing import Sequence, Dict, Mapping

from dtsh.model import DTNode
from dtsh.io import DTShOutput
from dtsh.shell import DTSh
from dtsh.shellutils import (
    DTShFlagReverse,
    DTShFlagEnabledOnly,
    DTShFlagPager,
    DTShFlagRecursive,
    DTShFlagNoChildren,
    DTShArgOrderBy,
    DTShArgFixedDepth,
    DTShParamDTPaths,
)

from dtsh.rich.shellutils import DTShCommandLongFmt
from dtsh.rich.text import TextUtil
from dtsh.rich.modelview import DTModelView, SketchMV, ViewNodeList


class DTShBuiltinLs(DTShCommandLongFmt):
    """Devicetree shell built-in "ls"."""

    def __init__(self) -> None:
        super().__init__(
            "ls",
            "list branch contents",
            [
                DTShFlagNoChildren(),
                DTShFlagReverse(),
                DTShFlagRecursive(),
                DTShFlagEnabledOnly(),
                DTShFlagPager(),
                DTShArgOrderBy(),
                DTShArgFixedDepth(),
            ],
            DTShParamDTPaths(),
        )

    def execute(self, argv: Sequence[str], sh: DTSh, out: DTShOutput) -> None:
        """Overrides DTShCommand.execute()."""
        super().execute(argv, sh, out)

        # Expand path parameter.
        path_expansions: Sequence[DTSh.PathExpansion] = self.with_param(
            DTShParamDTPaths
        ).expand(self, sh)

        if self.with_flag(DTShFlagPager):
            out.pager_enter()

        # What to do here depends on the appropriate model kind,
        # "files" (nodes) or "directories" (branches).
        if self.with_flag(DTShFlagNoChildren):
            # List nodes, not branche(s) contents ("-d").
            self._ls_nodes(path_expansions, sh, out)
        else:
            # List branches as directories.
            self._ls_contents(path_expansions, sh, out)

        if self.with_flag(DTShFlagPager):
            out.pager_exit()

    def _ls_nodes(
        self,
        path_expansions: Sequence[DTSh.PathExpansion],
        sh: DTSh,
        out: DTShOutput,
    ) -> None:
        # Get the nodes to list as "files".
        path2node: Mapping[str, DTNode] = self._get_path2node(
            path_expansions, sh
        )
        if not path2node:
            return

        if self.has_longfmt:
            # Formatted output.
            self._output_nodes_longfmt(path2node, out)
        else:
            # POSIX-like.
            self._output_nodes_raw(path2node, out)

    def _get_path2node(
        self,
        path_expansions: Sequence[DTSh.PathExpansion],
        sh: DTSh,
    ) -> Mapping[str, DTNode]:
        path2node: Dict[str, DTNode] = {}
        for expansion in path_expansions:
            for node in self.sort(expansion.nodes):
                path = sh.pathway(node, expansion.prefix)
                path2node[path] = node
        return path2node

    def _output_nodes_raw(
        self, path2node: Mapping[str, DTNode], out: DTShOutput
    ) -> None:
        for path in path2node:
            out.write(path)

    def _output_nodes_longfmt(
        self, path2node: Mapping[str, DTNode], out: DTShOutput
    ) -> None:
        sketch = self.get_sketch(SketchMV.Layout.LIST_VIEW)
        cols = self.get_longfmt(sketch.default_fmt)

        lv_nodes = ViewNodeList(cols, sketch)
        lv_nodes.extend(path2node.values())
        out.write(lv_nodes)

    def _ls_contents(
        self,
        path_expansions: Sequence[DTSh.PathExpansion],
        sh: DTSh,
        out: DTShOutput,
    ) -> None:
        # Get the branches to list the contents of as "directories".
        path2contents: Mapping[str, Sequence[DTNode]] = self._get_path2contents(
            path_expansions, sh
        )
        if not path2contents:
            return

        if self.has_longfmt:
            # Formatted output.
            self._output_contents_longfmt(path2contents, sh, out)
        else:
            # POSIX-like.
            self._output_contents_raw(path2contents, out)

    def _get_path2contents(
        self,
        path_expansions: Sequence[DTSh.PathExpansion],
        sh: DTSh,
    ) -> Mapping[str, Sequence[DTNode]]:
        mode_recursive = (
            self.with_flag(DTShFlagRecursive)
            or self.with_arg(DTShArgFixedDepth).isset
        )

        path2contents: Dict[str, Sequence[DTNode]] = {}
        for expansion in path_expansions:
            for node in self.sort(expansion.nodes):
                if mode_recursive:
                    for branch in node.walk(
                        order_by=self.arg_sorter,
                        reverse=self.flag_reverse,
                        enabled_only=self.flag_enabled_only,
                        fixed_depth=self.with_arg(DTShArgFixedDepth).depth,
                    ):
                        path = sh.pathway(branch, expansion.prefix)
                        path2contents[path] = self.sort(branch.children)
                else:
                    path = sh.pathway(node, expansion.prefix)
                    # Filter out disabled nodes if asked to.
                    contents = self.prune(node.children)
                    # Sort contents if asked to.
                    path2contents[path] = self.sort(contents)

        return path2contents

    def _output_contents_raw(
        self, path2contents: Mapping[str, Sequence[DTNode]], out: DTShOutput
    ) -> None:
        N = len(path2contents)
        for i, (dirpath, contents) in enumerate(path2contents.items()):
            if N > 1:
                out.write(f"{dirpath}:")

            for node in contents:
                out.write(node.name)

            if i != N - 1:
                # Insert empty line between "directories".
                out.write()

    def _output_contents_longfmt(
        self,
        path2contents: Mapping[str, Sequence[DTNode]],
        sh: DTSh,
        out: DTShOutput,
    ) -> None:
        N = len(path2contents)
        sketch = self.get_sketch(SketchMV.Layout.LIST_VIEW)
        cols = self.get_longfmt(sketch.default_fmt)

        for i, (dirpath, contents) in enumerate(path2contents.items()):
            if N > 1:
                tv_dirpath = DTModelView.mk_path(dirpath)
                if not sh.node_at(dirpath).enabled:
                    TextUtil.disabled(tv_dirpath)
                out.write(tv_dirpath, ":", sep="")

            if contents:
                # Output branch contents as list view.
                lv_nodes = ViewNodeList(cols, sketch)
                lv_nodes.left_indent(1)
                lv_nodes.extend(contents)
                out.write(lv_nodes)

            if i != N - 1:
                # Insert empty line between "directories".
                out.write()
