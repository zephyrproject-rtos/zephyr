# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell built-in "aliases".

List aliased nodes.

Unit tests and examples: tests/test_dtsh_builtin_alias.py
"""


from typing import Sequence, Mapping

from dtsh.model import DTNode
from dtsh.io import DTShOutput
from dtsh.shell import DTSh
from dtsh.shellutils import DTShFlagEnabledOnly, DTShParamAlias

from dtsh.rich.shellutils import DTShCommandLongFmt
from dtsh.rich.modelview import ViewNodeAkaList


class DTShBuiltinAlias(DTShCommandLongFmt):
    """Devicetree shell built-in 'alias'."""

    def __init__(self) -> None:
        super().__init__(
            "alias",
            "list aliased nodes",
            [
                DTShFlagEnabledOnly(),
            ],
            DTShParamAlias(),
        )

    def execute(self, argv: Sequence[str], sh: DTSh, out: DTShOutput) -> None:
        """Overrides DTShCommand.execute()."""
        super().execute(argv, sh, out)

        param_alias = self.with_param(DTShParamAlias).alias

        alias2node: Mapping[str, DTNode]
        if param_alias:
            # Aliased nodes that match the alias parameter.
            alias2node = {
                alias: node
                for alias, node in sh.dt.aliased_nodes.items()
                if alias.find(param_alias) != -1
            }
        else:
            # All aliased nodes.
            alias2node = sh.dt.aliased_nodes

        if self.with_flag(DTShFlagEnabledOnly):
            # Filter out aliased nodes which are disabled.
            alias2node = {
                alias: node
                for alias, node in alias2node.items()
                if node.enabled
            }

        # Silently output nothing if no matched aliased nodes.
        if alias2node:
            if self.has_longfmt:
                # Format output (unordered list view).
                # Default format string: "Path", "Binding".
                view = ViewNodeAkaList(alias2node, self.get_longfmt("pC"))
                out.write(view)
            else:
                # POSIX-like symlinks (link -> file).
                for alias, node in alias2node.items():
                    out.write(f"{alias} -> {node.path}")
