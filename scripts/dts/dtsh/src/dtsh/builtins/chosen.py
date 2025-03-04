# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell built-in "chosen".

List chosen nodes.

Unit tests and examples: tests/test_dtsh_builtin_chosen.py
"""


from typing import Sequence, Mapping

from dtsh.model import DTNode
from dtsh.io import DTShOutput
from dtsh.shell import DTSh
from dtsh.shellutils import DTShFlagEnabledOnly, DTShParamChosen

from dtsh.rich.shellutils import DTShCommandLongFmt
from dtsh.rich.modelview import ViewNodeAkaList


class DTShBuiltinChosen(DTShCommandLongFmt):
    """Devicetree shell built-in "chosen"."""

    def __init__(self) -> None:
        super().__init__(
            "chosen",
            "list chosen nodes",
            [
                DTShFlagEnabledOnly(),
            ],
            DTShParamChosen(),
        )

    def execute(self, argv: Sequence[str], sh: DTSh, out: DTShOutput) -> None:
        """Overrides DTShCommand.execute()."""
        super().execute(argv, sh, out)

        param_chosen = self.with_param(DTShParamChosen).chosen

        chosen2node: Mapping[str, DTNode]
        if param_chosen:
            # Chosen nodes that match the chosen parameter.
            chosen2node = {
                chosen: node
                for chosen, node in sh.dt.chosen_nodes.items()
                if chosen.find(param_chosen) != -1
            }
        else:
            # All chosen nodes.
            chosen2node = sh.dt.chosen_nodes

        if self.with_flag(DTShFlagEnabledOnly):
            # Filter out chosen nodes which are disabled.
            chosen2node = {
                chosen: node
                for chosen, node in chosen2node.items()
                if node.enabled
            }

        # Silently output nothing if no matched chosen nodes.
        if chosen2node:
            if self.has_longfmt:
                # Format output (unordered list view).
                # Default format string: "Path", "Binding".
                view = ViewNodeAkaList(chosen2node, self.get_longfmt("NC"))
                out.write(view)
            else:
                # POSIX-like symlinks (link -> file).
                for choice, node in chosen2node.items():
                    out.write(f"{choice} -> {node.path}")
