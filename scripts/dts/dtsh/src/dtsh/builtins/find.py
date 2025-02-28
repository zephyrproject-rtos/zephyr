# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell built-in "find".

Search for nodes with multiple criteria.

Unit tests and examples: tests/test_dtsh_builtin_find.py
"""


from typing import List, Sequence, Dict, Mapping, Tuple

from dtsh.model import DTWalkable, DTNode, DTNodeCriterion, DTNodeCriteria
from dtsh.modelutils import DTWalkableComb
from dtsh.io import DTShOutput
from dtsh.shell import DTSh, DTShError, DTShCommandError
from dtsh.shellutils import (
    DTShFlagReverse,
    DTShFlagEnabledOnly,
    DTShFlagPager,
    DTShFlagRegex,
    DTShFlagIgnoreCase,
    DTShFlagCount,
    DTShFlagTreeLike,
    DTShFlagLogicalOr,
    DTShFlagLogicalNot,
    DTShArgOrderBy,
    DTShArgCriterion,
    DTSH_ARG_NODE_CRITERIA,
    DTShParamDTPaths,
)

from dtsh.rich.shellutils import DTShCommandLongFmt
from dtsh.rich.modelview import (
    SketchMV,
    ViewNodeList,
    ViewNodeTreePOSIX,
    ViewNodeTwoSided,
)
from dtsh.rich.text import TextUtil


class DTShBuiltinFind(DTShCommandLongFmt):
    """Devicetree shell built-in "find"."""

    def __init__(self) -> None:
        super().__init__(
            "find",
            "search branches for nodes",
            [
                DTShFlagLogicalOr(),
                DTShFlagLogicalNot(),
                DTShFlagRegex(),
                DTShFlagIgnoreCase(),
                DTShFlagEnabledOnly(),
                DTShFlagCount(),
                DTShFlagReverse(),
                DTShFlagTreeLike(),
                DTShFlagPager(),
                DTShArgOrderBy(),
                *DTSH_ARG_NODE_CRITERIA,
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

        # What to do here depends on the appropriate model kind:
        # - either a simple list of found nodes
        # - or a virtual subtree defined with the found nodes as its leaves
        if self.with_flag(DTShFlagTreeLike):
            self._find_tree(path_expansions, sh, out)
        else:
            self._find_nodes(path_expansions, sh, out)

        if self.with_flag(DTShFlagPager):
            out.pager_exit()

    def _find_nodes(
        self,
        path_expansions: Sequence[DTSh.PathExpansion],
        sh: DTSh,
        out: DTShOutput,
    ) -> None:
        # Get model, mapping pathways to the nodes found there.
        path2node: Mapping[str, DTNode] = self._get_path2node(
            path_expansions, sh
        )
        if not path2node:
            return
        count = len(path2node)

        if self.has_longfmt:
            # Formatted output (list view).
            self._output_nodes_longfmt(path2node, count, out)
        else:
            # POSIX-like.
            self._output_nodes_raw(path2node, count, out)

    def _get_path2node(
        self,
        path_expansions: Sequence[DTSh.PathExpansion],
        sh: DTSh,
    ) -> Mapping[str, DTNode]:
        # Collect criterion chain.
        criteria = self._get_criteria()

        path2node: Dict[str, DTNode] = {}
        for expansion in path_expansions:
            for branch in self.sort(expansion.nodes):
                for node in branch.find(
                    criteria,
                    order_by=self.arg_sorter,
                    reverse=self.flag_reverse,
                    enabled_only=self.flag_enabled_only,
                ):
                    path = sh.pathway(node, expansion.prefix)
                    path2node[path] = node

        return path2node

    def _output_nodes_raw(
        self, path2node: Mapping[str, DTNode], count: int, out: DTShOutput
    ) -> None:
        # Output paths of found nodes.
        for path in path2node:
            out.write(path)
        if self.with_flag(DTShFlagCount):
            out.write()
            self._output_count_raw(count, out)

    def _output_nodes_longfmt(
        self, path2node: Mapping[str, DTNode], count: int, out: DTShOutput
    ) -> None:
        # Output found nodes as formatted list.
        sketch = self.get_sketch(SketchMV.Layout.LIST_VIEW)
        cols = self.get_longfmt(sketch.default_fmt)
        view = ViewNodeList(cols, sketch)
        view.extend(path2node.values())
        out.write(view)
        if self.with_flag(DTShFlagCount):
            out.write()
            self._output_count_longftm(count, out)

    def _find_tree(
        self,
        path_expansions: Sequence[DTSh.PathExpansion],
        sh: DTSh,
        out: DTShOutput,
    ) -> None:
        # Get model, mapping pathways to a subtree containing
        # the nodes found there as its leaves.
        count: int
        path2walkable: Mapping[str, DTWalkable]
        count, path2walkable = self._get_path2walkable(path_expansions, sh)
        if not path2walkable:
            return

        if self.has_longfmt:
            # Formatted output.
            self._output_treelike_longfmt(path2walkable, count, out)
        else:
            # POSIX-like.
            self._output_treelike_raw(path2walkable, count, out)

    def _output_treelike_raw(
        self,
        path2walkable: Mapping[str, DTWalkable],
        count: int,
        out: DTShOutput,
    ) -> None:
        # Tree-like views (POSIX output).
        N = len(path2walkable)
        for i, (path, comb) in enumerate(path2walkable.items()):
            view = ViewNodeTreePOSIX(path, comb)
            view.do_layout(
                self.arg_sorter,
                self.flag_reverse,
                self.flag_enabled_only,
            )
            out.write(view)

            if i != N - 1:
                out.write()

        if self.with_flag(DTShFlagCount):
            out.write()
            self._output_count_raw(count, out)

    def _output_treelike_longfmt(
        self,
        path2walkable: Mapping[str, DTWalkable],
        count: int,
        out: DTShOutput,
    ) -> None:
        # Tree-like views (2-sided).
        sketch = self.get_sketch(SketchMV.Layout.TWO_SIDED)
        cols = self.get_longfmt(sketch.default_fmt)

        N = len(path2walkable)
        for i, (_, comb) in enumerate(path2walkable.items()):
            view = ViewNodeTwoSided(comb, cols)
            view.do_layout(
                self.arg_sorter,
                self.flag_reverse,
                self.flag_enabled_only,
            )
            out.write(view)

            if i != N - 1:
                out.write()

        if self.with_flag(DTShFlagCount):
            out.write()
            self._output_count_longftm(count, out)

    def _get_path2walkable(
        self,
        path_expansions: Sequence[DTSh.PathExpansion],
        sh: DTSh,
    ) -> Tuple[int, Mapping[str, DTWalkable]]:
        # Collect criterion chain.
        criteria = self._get_criteria()

        # One tree per root: for each expanded path, map its pathway
        # to a subtree containing the nodes found there.
        path2walkable: Dict[str, DTWalkable] = {}
        count: int = 0
        for expansion in path_expansions:
            for branch in self.sort(expansion.nodes):
                path = sh.pathway(branch, expansion.prefix)
                nodes = list(
                    branch.find(
                        criteria,
                        order_by=self.arg_sorter,
                        reverse=self.flag_reverse,
                        enabled_only=self.flag_enabled_only,
                    )
                )
                if nodes:
                    count += len(nodes)
                    comb = DTWalkableComb(branch, nodes)
                    path2walkable[path] = comb

        return (count, path2walkable)

    def _output_count_longftm(self, count: int, out: DTShOutput) -> None:
        out.write(
            TextUtil.assemble("Found ", TextUtil.bold(str(count))),
            "nodes.",
        )

    def _output_count_raw(self, count: int, out: DTShOutput) -> None:
        out.write(f"Found: {count}")

    def _get_criteria(self) -> DTNodeCriteria:
        # All defined command arguments that may participate in the search.
        args_criterion: List[DTShArgCriterion] = [
            self.with_arg(type(option))
            for option in self.options
            if isinstance(option, DTShArgCriterion)
        ]

        try:
            arg_criteria: List[DTNodeCriterion] = [
                criterion
                for criterion in (
                    arg_criterion.get_criterion(
                        re_strict=self.with_flag(DTShFlagRegex),
                        ignore_case=self.with_flag(DTShFlagIgnoreCase),
                    )
                    for arg_criterion in args_criterion
                )
                if criterion
            ]

            return DTNodeCriteria(
                arg_criteria,
                ored_chain=self.with_flag(DTShFlagLogicalOr),
                negative_chain=self.with_flag(DTShFlagLogicalNot),
            )

        except DTShError as e:
            # Invalid criterion pattern or expression.
            raise DTShCommandError(self, e.msg) from e
