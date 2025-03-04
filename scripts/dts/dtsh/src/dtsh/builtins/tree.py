# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell built-in "tree".

List nodes in tree-like format.

Unit tests and examples: tests/test_dtsh_builtin_tree.py
"""


from typing import Sequence, Mapping, Dict

from dtsh.model import DTNode
from dtsh.io import DTShOutput
from dtsh.shell import DTSh
from dtsh.shellutils import (
    DTShFlagReverse,
    DTShFlagEnabledOnly,
    DTShFlagPager,
    DTShArgOrderBy,
    DTShArgFixedDepth,
    DTShParamDTPaths,
)

from dtsh.rich.modelview import (
    SketchMV,
    ViewNodeTreePOSIX,
    ViewNodeTwoSided,
)
from dtsh.rich.shellutils import DTShCommandLongFmt


class DTShBuiltinTree(DTShCommandLongFmt):
    """Devicetree shell built-in "tree"."""

    def __init__(self) -> None:
        super().__init__(
            "tree",
            "list branch contents in tree-like format",
            [
                DTShFlagReverse(),
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

        # Expand path parameter: can't be empty.
        path_expansions: Sequence[DTSh.PathExpansion] = self.with_param(
            DTShParamDTPaths
        ).expand(self, sh)
        # Get the model, mapping the branches to list to their expected pathways.
        # pathway -> node.
        path2branch: Mapping[str, DTNode] = self._get_path2branch(
            path_expansions, sh
        )

        if self.with_flag(DTShFlagPager):
            out.pager_enter()

        N = len(path2branch)
        for i, (path, branch) in enumerate(path2branch.items()):
            if self.has_longfmt:
                # Formatted output (2sided view).
                self._output_longfmt(branch, out)
            else:
                # POSIX-like simple tree.
                self._output_raw(path, branch, out)

            if i != N - 1:
                # Insert empty line between trees.
                out.write()

        if self.with_flag(DTShFlagPager):
            out.pager_exit()

    def _get_path2branch(
        self,
        path_expansions: Sequence[DTSh.PathExpansion],
        sh: DTSh,
    ) -> Mapping[str, DTNode]:
        path2branch: Dict[str, DTNode] = {}
        for expansion in path_expansions:
            for branch in self.sort(expansion.nodes):
                path = sh.pathway(branch, expansion.prefix)
                path2branch[path] = branch

        return path2branch

    def _output_longfmt(self, branch: DTNode, out: DTShOutput) -> None:
        # Formatted branch output (2-sided view).
        sketch = self.get_sketch(SketchMV.Layout.TWO_SIDED)
        cells = self.get_longfmt(sketch.default_fmt)
        view_2sided = ViewNodeTwoSided(branch, cells)
        view_2sided.do_layout(
            self.arg_sorter,
            self.flag_reverse,
            self.flag_enabled_only,
            self.with_arg(DTShArgFixedDepth).depth,
        )
        out.write(view_2sided)

    def _output_raw(self, path: str, branch: DTNode, out: DTShOutput) -> None:
        # Branch as tree (POSIX output).
        tree = ViewNodeTreePOSIX(path, branch)
        tree.do_layout(
            self.arg_sorter,
            self.flag_reverse,
            self.flag_enabled_only,
            self.with_arg(DTShArgFixedDepth).depth,
        )
        out.write(tree)
