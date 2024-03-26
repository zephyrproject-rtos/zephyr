# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Rich interactive devicetree shell session.

Extend the base session with a few rich TUI elements
and support for SVG and HTML command output redirection formats.
"""

import os

from dtsh.shell import (
    DTSh,
    DTShCommandNotFoundError,
    DTShUsageError,
    DTShCommandError,
)
from dtsh.session import DTShSession
from dtsh.io import DTShOutput, DTShRedirect

from dtsh.rich.io import (
    DTShRichVT,
    DTShOutputFileText,
    DTShOutputFileHtml,
    DTShOutputFileSVG,
)
from dtsh.rich.autocomp import DTShRichAutocomp
from dtsh.rich.theme import DTShTheme
from dtsh.rich.text import TextUtil


_theme: DTShTheme = DTShTheme.getinstance()


class DTShRichSession(DTShSession):
    """Rich interactive devicetree shell session."""

    def __init__(self, sh: DTSh) -> None:
        """Initialize rich session.

        Extend the base session with a few rich TUI elements
        and support for SVG and HTML command output redirection formats.

        Initialized with:

        - a rich VT based on Textualize's rich.Console (DTShRichVT)
        - a rich auto-completion display hook (DTShRichAutocomp)

        Args:
            sh: The session's shell.
        """
        super().__init__(sh, DTShRichVT(), DTShRichAutocomp(sh))

    def preamble_hook(self) -> None:
        """Overrides DTShSession.preamble_hook()."""
        self._vt.write(
            TextUtil.join(
                " ",
                [
                    TextUtil.bold("dtsh"),
                    TextUtil.mk_text(f"({DTSh.VERSION_STRING}):"),
                    TextUtil.italic("Shell-like interface with Devicetree"),
                ],
            )
        )
        self._vt.write(
            TextUtil.assemble(
                TextUtil.mk_text("How to exit: "),
                TextUtil.bold("q"),
                TextUtil.mk_text(", or "),
                TextUtil.bold("quit"),
                TextUtil.mk_text(", or "),
                TextUtil.bold("exit"),
                TextUtil.mk_text(", or press "),
                TextUtil.bold("Ctrl-D"),
            )
        )
        self._vt.write()

    def open_redir2(self, redir2: str) -> DTShOutput:
        """Overrides DTShSession.open_redir2().

        This redirection supports SVG and HTML exports.
        """
        redirect = DTShRedirect(redir2)
        ext: str = os.path.splitext(redirect.path)[1].lower()

        if ext == ".html":
            return DTShOutputFileHtml(redirect.path, redirect.append)
        if ext == ".svg":
            return DTShOutputFileSVG(redirect.path, redirect.append)

        return DTShOutputFileText(redirect.path, redirect.append)

    def on_cmd_not_found_error(self, e: DTShCommandNotFoundError) -> None:
        """Overrides DTShSession.on_cmd_not_found_error()."""
        self._vt.write(
            TextUtil.mk_text("command not found: ").join(
                (
                    TextUtil.mk_text("dtsh: "),
                    TextUtil.mk_text(e.name, DTShTheme.STYLE_ERROR),
                )
            )
        )

    def on_cmd_usage_error(self, e: DTShUsageError) -> None:
        """Overrides DTShSession.on_cmd_usage_error()."""
        self._vt.write(
            TextUtil.mk_text(": ").join(
                (
                    TextUtil.mk_text(e.cmd.name),
                    TextUtil.mk_text(e.msg, DTShTheme.STYLE_ERROR),
                )
            )
        )

    def on_cmd_failed_error(self, e: DTShCommandError) -> None:
        """Overrides DTShSession.on_cmd_failed_error()."""
        self._vt.write(
            TextUtil.mk_text(": ").join(
                (
                    TextUtil.mk_text(e.cmd.name),
                    TextUtil.mk_text(e.msg, DTShTheme.STYLE_ERROR),
                )
            )
        )

    def on_redir2_error(self, e: DTShRedirect.Error) -> None:
        """Overrides DTShSession.on_redir2_error()."""
        self._vt.write(
            TextUtil.assemble(
                TextUtil.mk_text("dtsh: "),
                TextUtil.mk_text(str(e), DTShTheme.STYLE_WARNING),
            )
        )

    def pre_exit_hook(self, status: int) -> None:
        """Overrides DTShSession.pre_exit_hook()."""
        style = DTShTheme.STYLE_ERROR if status else DTShTheme.STYLE_DEFAULT
        txt = TextUtil.mk_text("bye.", style)
        self._vt.write(TextUtil.italic(txt))
