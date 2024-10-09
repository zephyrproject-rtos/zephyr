# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Base devicetree shell session.

A session binds a shell and I/O streams, then enters a loop:
- read command lines from input stream
- execute shell commands with the appropriate output stream
- exit on EOF or "quit"
"""

from types import FrameType
from typing import Any, Optional, Sequence, List

import errno
import signal
import sys

from devicetree import edtlib

from dtsh.model import DTModel
from dtsh.rl import DTShReadline
from dtsh.config import DTShConfig
from dtsh.autocomp import DTShAutocomp
from dtsh.io import DTShOutput, DTShOutputFile, DTShRedirect, DTShVT
from dtsh.shell import (
    DTSh,
    DTShCommand,
    DTShFlagHelp,
    DTShError,
    DTShUsageError,
    DTShCommandError,
    DTShCommandNotFoundError,
)
from dtsh.builtins.pwd import DTShBuiltinPwd
from dtsh.builtins.cd import DTShBuiltinCd
from dtsh.builtins.ls import DTShBuiltinLs
from dtsh.builtins.tree import DTShBuiltinTree
from dtsh.builtins.find import DTShBuiltinFind
from dtsh.builtins.alias import DTShBuiltinAlias
from dtsh.builtins.chosen import DTShBuiltinChosen
from dtsh.builtins.cat import DTShBuiltinCat
from dtsh.builtins.uname import DTShBuiltinUname


_dtshconf: DTShConfig = DTShConfig.getinstance()


class DTShSession:
    """Base for devicetree shell sessions."""

    _dtsh: DTSh
    _vt: DTShVT

    _rl: DTShReadline
    _autocomp: DTShAutocomp

    _last_err: Optional[BaseException]

    @classmethod
    def create(
        cls, dts_path: str, binding_dirs: Optional[Sequence[str]] = None
    ) -> "DTShSession":
        """Create a new devicetree shell session.

        Args:
            dts_path: Path to the Devicetree source to open the session with.
            binding_dirs: List of directories to search for
              the YAML binding files this Devicetree depends on.

        Returns:
            An initialized session.

        Raises:
            DTShError: Typically DTS file not found or invalid,
              or missing or invalid bindings.
        """
        dt = cls._create_dtmodel(dts_path, binding_dirs)
        sh = cls._create_dtsh(dt)
        return cls(sh)

    def __init__(
        self,
        sh: DTSh,
        vt: Optional[DTShVT] = None,
        autocomp: Optional[DTShAutocomp] = None,
    ) -> None:
        """Initialize a session.

        Will initialize the VT and auto-completion support.

        Won't start the loop until run().

        Args:
            sh: The session's shell.
            vt: The session's VT. Defaults to DTShVT.
            autocomp: GNU readline callbacks for completion and matches display.
              Defaults to DTShAutocomp().
        """
        self._dtsh = sh
        self._last_err = None

        self._vt = vt or DTShVT()
        self._autocomp = autocomp or DTShAutocomp(self._dtsh)

        self._rl = DTShReadline(
            self._vt,
            self._autocomp.complete,
            self._autocomp.display,
        )

    def run(
        self, interactive: bool = True
    ) -> None:  # pylint: disable=too-many-branches
        """Enter session loop.

        If interactive, first run the preamble hook:
        - handle SIGINT to work-around TTY issues
        - clear VT and print banner

        Enter actual loop, until EOF or "quit" command:
        - read next command line from input stream
        - parse and execute command line, with the session's VT
          or a redirection file as output stream

        Eventually run the pre-exit hook if interactive,
        and exit the devicetree shell.
        """
        if interactive:
            self._preamble_hook()

        # Session error state.
        self._last_err = None

        while True:
            cmdline: Optional[str] = None
            try:
                cmdline = self._vt.readline(self.mk_prompt())
            except EOFError:
                # Exit DTSh on EOF.
                self.close(interactive)

            if cmdline:
                if cmdline.strip() in ["q", "quit", "exit"]:
                    # Exit DTSh process.
                    self.close(interactive)

                cmd: DTShCommand
                argv: List[str]
                redir2: Optional[str]
                try:
                    # Parse command line into the command to execute,
                    # its arguments, and the redirection directive, if any.
                    # Won't parse the command arguments yet,
                    # but will fault if the command is undefined.
                    cmd, argv, redir2 = self._dtsh.parse_cmdline(cmdline)

                    out: DTShOutput = (
                        self.open_redir2(redir2) if redir2 else self._vt
                    )

                except DTShRedirect.Error as e:
                    # Failed to initialize redirection stream.
                    self._last_err = e
                    self.on_redir2_error(e)

                except DTShCommandNotFoundError as e:
                    self._last_err = e
                    self.on_cmd_not_found_error(e)

                else:
                    try:
                        cmd.execute(argv, self._dtsh, out)

                        # Last command line succeeded, reset error state.
                        self._last_err = None

                    except DTShUsageError as e:
                        if e.cmd.with_flag(DTShFlagHelp):
                            # The user asked for help (-h or --h).
                            self.on_cmd_help(e.cmd)
                        else:
                            # Invalid command arguments.
                            self._last_err = e
                            self.on_cmd_usage_error(e)

                    except DTShCommandError as e:
                        # Command execution failed.
                        self._last_err = e
                        self.on_cmd_failed_error(e)

                    finally:
                        if out is not self._vt:
                            # Flush the file the command output
                            # was redirected to, even on error.
                            # Note that the shell (error) messages themselves
                            # are always written to the session VT,
                            # and never redirected.
                            out.flush()

            # NOTE: Be sure to set prompt_sparse in preferences
            # when running batch sessions.
            if _dtshconf.prompt_sparse and self._vt.is_tty():
                self._vt.write()

    def close(self, interactive: bool) -> None:
        """Terminate session.

        Will first run the pre-exit hook if interactive,
        saving commands history, and exit the session's process.

        Exits with status code -EINVAL if the last command line failed.
        """
        if interactive:
            self._pre_exit_hook()
        sys.exit(-errno.EINVAL if self._last_err else 0)

    def open_redir2(self, redir2: str) -> DTShOutput:
        """Open DTSh redirection output stream.

        Args:
            redir2: Command line redirection.

        Returns:
            The redirection output stream.

        Raises:
            DTShRedirect.Error: Failed to setup redirection output.
        """
        redirect = DTShRedirect(redir2)
        return DTShOutputFile(redirect.path, redirect.append)

    def on_cmd_help(self, cmd: DTShCommand) -> None:
        """Called when the user's asked for a command's help.

        Args:
            cmd: The command to help with.
        """
        self._vt.write(f"usage: {cmd.synopsis}")
        self._vt.write()
        self._vt.write(f"{cmd.brief.capitalize()}.")

    def on_cmd_not_found_error(self, e: DTShCommandNotFoundError) -> None:
        """Called when the user's asked for an unknown command.

        Args:
            e: The error event.
        """
        self._vt.write(f"dtsh: command not found: {e.name}")

    def on_cmd_usage_error(self, e: DTShUsageError) -> None:
        """Called when the user's misused a command.

        Args:
            e: The error event.
        """
        self._vt.write(f"{e.cmd.name}: {e.msg}")

    def on_cmd_failed_error(self, e: DTShCommandError) -> None:
        """Called when the last command execution has failed.

        Args:
            e: The error event.
        """
        self._vt.write(f"{e.cmd.name}: {e.msg}")

    def on_redir2_error(self, e: DTShRedirect.Error) -> None:
        """Called when failed to setup redirection stream.

        Args:
            e: The error event.
        """
        self._vt.write(f"dtsh: {e}")

    def mk_prompt(self) -> Sequence[Any]:
        """Make multiple-line prompt to use for the next command.

        Returns:
            A valid multiple-line prompt: optional state lines of any type,
            followed by the actual ANSI prompt.
        """
        return [
            self._dtsh.pwd,
            _dtshconf.prompt_alt
            if self._last_err
            else _dtshconf.prompt_default,
        ]

    def mk_prologue(self) -> Sequence[Any]:
        """Shell prologue.

        Returns:
            The shell multiple-line banner.
        """
        return ["dtsh: A Devicetree Shell"]

    def mk_epilogue(self) -> Sequence[Any]:
        """Shell epilogue.

        Returns:
            A goodbye message.
        """
        return ["bye."]

    def _preamble_hook(self) -> None:
        # Hook called when an interactive session starts.

        # Closing with SIGINT when the pager is active breaks the TTY.
        # As a work-around, we ignore SIGINT in interactive sessions.
        signal.signal(signal.SIGINT, self._sig_handler)

        self._vt.clear()
        for line in self.mk_prologue():
            self._vt.write(line)
        self._vt.write()

    def _pre_exit_hook(self) -> None:
        # Hook called when an interactive session terminates.
        self._rl.save_history()
        for line in self.mk_epilogue():
            self._vt.write(line)

    @classmethod
    def _create_dtmodel(
        cls, dts_path: str, binding_dirs: Optional[Sequence[str]]
    ) -> DTModel:
        try:
            return DTModel.create(dts_path, binding_dirs)
        except (OSError, edtlib.EDTError) as e:
            raise DTShError(f"DTS error: {e}") from e

    @classmethod
    def _create_dtsh(cls, dt: DTModel) -> DTSh:
        return DTSh(
            dt,
            [
                DTShBuiltinPwd(),
                DTShBuiltinCd(),
                DTShBuiltinLs(),
                DTShBuiltinTree(),
                DTShBuiltinFind(),
                DTShBuiltinAlias(),
                DTShBuiltinChosen(),
                DTShBuiltinCat(),
                DTShBuiltinUname(),
            ],
        )

    def _sig_handler(self, signum: int, frame: Optional[FrameType]) -> Any:
        # closing() the session when the pager is active breaks the TTY.
        # As a work-around, we ignore SIGINT.
        del frame  # Unused.
        if signum == signal.SIGINT:
            pass
