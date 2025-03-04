# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Rich devicetree shell sessions.

Extend the base session with:
- rich TUI and auto-completion display hook
- SVG and HTML command output redirection formats
- batch commands to execute on start-up, before or instead of user input
"""

from typing import Any, Sequence, Optional, List, Union

import os

from dtsh.config import DTShConfig
from dtsh.shell import (
    DTSh,
    DTShError,
    DTShCommandNotFoundError,
    DTShUsageError,
    DTShCommandError,
)
from dtsh.session import DTShSession
from dtsh.io import DTShInput, DTShOutput, DTShRedirect, DTShVT, DTShInputFile, DTShInputCmds

from dtsh.rich.io import (
    DTShRichVT,
    DTShBatchRichVT,
    DTShOutputFileText,
    DTShOutputFileHtml,
    DTShOutputFileSVG,
)
from dtsh.rich.autocomp import DTShRichAutocomp
from dtsh.rich.theme import DTShTheme
from dtsh.rich.text import TextUtil
from dtsh.rich.modelview import DTModelView


_dtshconf: DTShConfig = DTShConfig.getinstance()


class DTShRichSession(DTShSession):
    """Rich devicetree shell session."""

    @classmethod
    def create_batch(
        cls,
        dts_path: str,
        binding_dirs: Optional[List[str]],
        batch: Union[str, List[str]],
        interactive: bool,
    ) -> DTShSession:
        """Create batch session.

        Args:
            dts_path: Path to the Devicetree source to open the session with.
            binding_dirs: List of directories to search for
              the YAML binding files this Devicetree depends on.
            batch: Batch input specification.
              Commands to execute on start-up, either:
              - string: path to a batch file
              - list: a list of commands lines
            interactive: Whether to continue with interactive session
              after batch.

        Returns:
            An initialized batch session.

        Raises:
            DTShError: Invalid DTS or batch input file.
        """
        vt: Optional[DTShVT] = None
        batch_is: DTShInput
        if isinstance(batch, str):
            # Batch input file.
            try:
                batch_is = DTShInputFile(batch)
            except DTShInputFile.Error as e:
                raise DTShError(str(e)) from e
        elif isinstance(batch, List):
            # Batch commands from cli arguments.
            batch_is = DTShInputCmds(batch)
        else:
            raise NotImplementedError(f"create_batch({repr(type(batch))})")

        dt = cls._create_dtmodel(dts_path, binding_dirs)
        vt = DTShBatchRichVT(batch_is, interactive)
        sh = cls._create_dtsh(dt)
        return cls(sh, vt)

    def __init__(
        self,
        sh: DTSh,
        vt: Optional[DTShVT] = None,
    ) -> None:
        """Initialize rich session.

        Extend the base session with:
        - rich TUI and auto-completion display hook
        - SVG and HTML command output redirection formats
        - batch commands to execute on start-up, before or instead of user input

        Args:
            sh: The session's shell.
            vt: The session's VT, default to DTShRichVT.
        """
        super().__init__(sh, vt or DTShRichVT(), DTShRichAutocomp(sh))

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

    def mk_prompt(self) -> Sequence[Any]:
        """Overrides DTShVT.mk_prompt()."""
        return [
            DTModelView.mk_path_name(self._dtsh.pwd),
            _dtshconf.prompt_alt
            if self._last_err
            else _dtshconf.prompt_default,
        ]

    def mk_prologue(self) -> Sequence[Any]:
        """Overrides DTShSession.mk_prologue()."""
        txt_banner = TextUtil.join(
            " ",
            [
                TextUtil.bold("dtsh"),
                TextUtil.mk_text(f"({DTSh.VERSION_STRING}):"),
                TextUtil.italic("A Devicetree Shell"),
            ],
        )
        txt_howto = TextUtil.assemble(
            TextUtil.mk_text("How to exit: "),
            TextUtil.bold("q"),
            TextUtil.mk_text(", or "),
            TextUtil.bold("quit"),
            TextUtil.mk_text(", or "),
            TextUtil.bold("exit"),
            TextUtil.mk_text(", or press "),
            TextUtil.bold("Ctrl-D"),
        )
        return [txt_banner, txt_howto]

    def mk_epilogue(self) -> Sequence[Any]:
        """Overrides DTShSession.mk_epilogue()."""
        return [TextUtil.italic("bye.")]
