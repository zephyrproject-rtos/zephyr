# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Rich display callback for GNU readline integration."""


from typing import Optional, Sequence, Set

import os

from rich.text import Text

from dtsh.io import DTShOutput
from dtsh.rl import DTShReadline
from dtsh.autocomp import (
    DTShAutocomp,
    RlStateDTShCommand,
    RlStateDTShOption,
    RlStateDTPath,
    RlStateCompatStr,
    RlStateDTVendor,
    RlStateDTBus,
    RlStateDTAlias,
    RlStateDTChosen,
    RlStateDTLabel,
    RlStateFsEntry,
    RlStateEnum,
)

from dtsh.rich.theme import DTShTheme
from dtsh.rich.text import TextUtil
from dtsh.rich.tui import GridLayout


class DTShRichAutocomp(DTShAutocomp):
    """Rich display callbacks for GNU readline integration."""

    def display(
        self,
        out: DTShOutput,
        states: Sequence[DTShReadline.CompleterState],
    ) -> None:
        """Rich display callback.

        Style completions based on their semantic.

        Implements DTShReadline.DisplayCallback.
        Overrides DTShAutocomp.display().

        Args:
            out: Where to display these completions.
            states: The completer states to display.
        """
        grid = GridLayout(2, padding=(0, 4, 0, 0))

        for state in states:
            if isinstance(state, RlStateDTShCommand):
                self._rlstates_view_add_dtshcmd(grid, state)

            elif isinstance(state, RlStateDTShOption):
                self._rlstates_view_add_dtshopt(grid, state)

            elif isinstance(state, RlStateDTPath):
                self._rlstates_view_add_dtpath(grid, state)

            elif isinstance(state, RlStateCompatStr):
                self._rlstates_view_add_compatstr(grid, state)

            elif isinstance(state, RlStateDTVendor):
                self._rlstates_view_add_vendor(grid, state)

            elif isinstance(state, RlStateDTBus):
                self._rlstates_view_add_bus(grid, state)

            elif isinstance(state, RlStateDTAlias):
                self._rlstates_view_add_alias(grid, state)

            elif isinstance(state, RlStateDTChosen):
                self._rlstates_view_add_chosen(grid, state)

            elif isinstance(state, RlStateDTLabel):
                self._rlstates_view_add_label(grid, state)

            elif isinstance(state, RlStateFsEntry):
                self._rlstates_view_add_fspath(grid, state)

            elif isinstance(state, RlStateEnum):
                self._rlstates_view_add_enum(grid, state)

            else:
                grid.add_row(state.rlstr, None)

        out.write(grid)

    def _rlstates_view_add_dtshcmd(
        self, grid: GridLayout, state: RlStateDTShCommand
    ) -> None:
        grid.add_row(TextUtil.bold(state.cmd.name), state.cmd.brief)

    def _rlstates_view_add_dtshopt(
        self, grid: GridLayout, state: RlStateDTShOption
    ) -> None:
        grid.add_row(TextUtil.bold(state.opt.usage), state.opt.brief)

    def _rlstates_view_add_dtpath(
        self, grid: GridLayout, state: RlStateDTPath
    ) -> None:
        txt = TextUtil.mk_text(state.node.name, DTShTheme.STYLE_DT_NODE_NAME)
        if not state.node.enabled:
            TextUtil.dim(txt)
        grid.add_row(txt, None)

    def _rlstates_view_add_compatstr(
        self, grid: GridLayout, state: RlStateCompatStr
    ) -> None:
        txt_desc: Optional[Text] = None
        if state.bindings:
            # The compatible string associates bindings,
            # look for description.
            if len(state.bindings) == 1:
                # Single binding, use its description if any.
                binding = state.bindings.pop()
                if binding.description:
                    txt_desc = TextUtil.mk_headline(
                        binding.description, DTShTheme.STYLE_DT_BINDING_DESC
                    )
            else:
                headlines: Set[str] = set()
                buses: Set[str] = set()

                for binding in state.bindings:
                    if binding.on_bus:
                        buses.add(binding.on_bus)
                    headline = binding.get_headline()
                    if headline:
                        headlines.add(headline)

                if len(headlines) == 1:
                    # All associated bindings have the same description
                    # headline, use it.
                    txt_desc = TextUtil.mk_headline(
                        headlines.pop(), DTShTheme.STYLE_DT_BINDING_DESC
                    )
                elif buses:
                    # Tell user about different buses of appearance.
                    txt_desc = TextUtil.assemble(
                        TextUtil.italic("Available for different buses: "),
                        TextUtil.mk_text(
                            ", ".join(buses), DTShTheme.STYLE_DT_BUS
                        ),
                    )

        txt_compat = TextUtil.mk_text(
            state.compatstr, DTShTheme.STYLE_DT_COMPAT_STR
        )

        grid.add_row(txt_compat, txt_desc)

    def _rlstates_view_add_vendor(
        self, grid: GridLayout, state: RlStateDTVendor
    ) -> None:
        grid.add_row(
            TextUtil.mk_text(state.prefix, DTShTheme.STYLE_DT_COMPAT_STR),
            TextUtil.mk_text(state.vendor, DTShTheme.STYLE_DT_VENDOR_NAME),
        )

    def _rlstates_view_add_bus(
        self, grid: GridLayout, state: RlStateDTBus
    ) -> None:
        grid.add_row(
            TextUtil.mk_text(state.proto, DTShTheme.STYLE_DT_BUS),
            None,
        )

    def _rlstates_view_add_alias(
        self, grid: GridLayout, state: RlStateDTAlias
    ) -> None:
        txt = TextUtil.mk_text(state.alias, DTShTheme.STYLE_DT_ALIAS)
        if not state.node.enabled:
            TextUtil.dim(txt)
        grid.add_row(txt, None)

    def _rlstates_view_add_chosen(
        self, grid: GridLayout, state: RlStateDTChosen
    ) -> None:
        txt = TextUtil.mk_text(state.chosen, DTShTheme.STYLE_DT_CHOSEN)
        if not state.node.enabled:
            TextUtil.dim(txt)
        grid.add_row(txt, None)

    def _rlstates_view_add_label(
        self, grid: GridLayout, state: RlStateDTLabel
    ) -> None:
        txt_label = TextUtil.mk_text(state.label, DTShTheme.STYLE_DT_NODE_LABEL)
        if not state.node.enabled:
            TextUtil.dim(txt_label)
        if state.node.description:
            txt_desc = TextUtil.mk_headline(
                state.node.description, DTShTheme.STYLE_DT_BINDING_DESC
            )
            if not state.node.enabled:
                TextUtil.dim(txt_desc)
        else:
            txt_desc = None
        grid.add_row(txt_label, txt_desc)

    def _rlstates_view_add_fspath(
        self, layout: GridLayout, state: RlStateFsEntry
    ) -> None:
        if state.dirent.is_dir():
            txt = TextUtil.mk_text(
                f"{state.dirent.name}{os.sep}",
                style=DTShTheme.STYLE_FS_DIR,
            )
        else:
            txt = TextUtil.mk_text(
                state.dirent.name,
                style=DTShTheme.STYLE_FS_FILE,
            )
        layout.add_row(txt, None)

    def _rlstates_view_add_enum(
        self, grid: GridLayout, state: RlStateEnum
    ) -> None:
        grid.add_row(TextUtil.bold(state.value), state.brief)
