# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""I/O streams for devicetree shells.

- "/dev/null" I/O semantic: DTShOutput, DTShInput
- base terminal API: DTShVT
- parsed command redirection: DTShRedirection
- command redirection to file: DTShOutputFile

Unit tests and examples: tests/test_dtsh_io.py
"""

from typing import Any, IO, Tuple, Optional, List, Sequence

import os
import sys

from dtsh.config import DTShConfig

_dtshconf: DTShConfig = DTShConfig.getinstance()


class DTShOutput:
    """Base for shell output streams.

    This base implementation behaves like "/dev/null".
    """

    def write(self, *args: Any, **kwargs: Any) -> None:
        """Write to output stream.

        Args:
            *args: Positional arguments.
              Semantic depends on the actual concrete stream.
            **kwargs: Keyword arguments.
              Semantic depends on the actual concrete stream.
        """

    def flush(self) -> None:
        """Flush output stream.

        Semantic depends on the actual concrete stream.
        """

    def pager_enter(self) -> None:
        """Page output until a call to pager_exit()."""

    def pager_exit(self) -> None:
        """Stop paging output."""


class DTShInput:
    """Base for shell input streams.

    This base implementation immediately signals EOF.
    """

    def readline(self, multi_prompt: Optional[Sequence[Any]] = None) -> str:
        """Print the prompt and read a command line from input stream.

        Args:
            multi_prompt: The multiple-line prompt to use.

        Returns:
            The next command line.

        Raises:
            EOFError: End of input stream.
        """
        raise EOFError()

    def is_tty(self) -> bool:
        """Whether the command lines come from a TTY (interactive mode).

        In batch mode, the command lines come from either a file input stream
        or the CLI arguments.
        """
        return True


class DTShVT(DTShInput, DTShOutput):
    """Base terminal for devicetree shells.

    This base implementation writes to stdout, reads from stdin
    and ignores paging.
    """

    def readline(self, multi_prompt: Optional[Sequence[Any]] = None) -> str:
        r"""Print the prompt and read a command line.

        To use ANSI escape codes in the prompt without breaking
        the GNU readline cursor position, please protect SGR parameters
        with RL_PROMPT_{START,STOP}_IGNORE markers:

            <SGR> := <CSI><n1, n2, ...>m
            <PROMPT> := <START_IGNORE><SGR><END_IGNORE>

            <START_IGNORE> := \001
            <END_IGNORE> := \002

        Overrides DTShInput.readline().
        """
        multi_prompt = multi_prompt or ["> "]
        preamble: Sequence[Any] = multi_prompt[:-1]
        prompt: str = multi_prompt[-1]

        # Print multiple-line prompt's preamble.
        for line in preamble:
            self.write(line)

        # Read user input or EOF.
        try:
            return input(prompt)
        except EOFError as e:
            # Dismiss dead interactive prompt line.
            self.write()
            raise e

    def write(self, *args: Any, **kwargs: Any) -> None:
        """Write to stdout.

        Overrides DTShOutput.write().

        Args:
            *args: Positional arguments, standard Python print() semantic.
            **kwargs: Keyword arguments, standard Python print() semantic.
        """
        print(*args, **kwargs)

    def flush(self) -> None:
        """Flush stdout.

        Overrides DTShOutput.flush().
        """
        sys.stdout.flush()

    def clear(self) -> None:
        """Clear VT.

        Ignored by base VT.

        NOTE: duplicate implementation from rich.Console.clear() ?
        """


class DTShOutputFile(DTShOutput):
    """Output file for command output redirection."""

    _out: IO[str]

    def __init__(self, path: str, append: bool) -> None:
        """Initialize output file.

        Args:
            path: The output file path.
            append: Whether to redirect the command's output in "append" mode.

        Raises:
             DTShRedirect.Error: Invalid path or permission errors.
        """
        try:
            # We can't use a context manager here, we just want to open
            # the file for later subsequent writes.
            self._out = open(  # pylint: disable=consider-using-with
                path,
                "a" if append else "w",
                encoding="utf-8",
            )
            if append:
                # Insert blank line between command outputs.
                self._out.write(os.linesep)
        except OSError as e:
            raise DTShRedirect.Error(e.strerror) from e

    def write(self, *args: Any, **kwargs: Any) -> None:
        """Write to output file.

        Overrides DTShOutput.write().

        Args:
            *args: Positional arguments, standard Python print(file=) semantic.
            **kwargs: Keyword arguments, standard Python print(file=) semantic.
        """
        print(*args, **kwargs, file=self._out)

    def flush(self) -> None:
        """Close output file.

        Overrides DTShOutput.flush().
        """
        self._out.close()


class DTShRedirect(DTShOutput):
    """Setup command output redirection."""

    class Error(BaseException):
        """Failed to setup redirection stream."""

    @classmethod
    def parse_redir2(cls, redir2: str) -> Tuple[str, bool]:
        """Parse redirection stream into path and mode.

        Does not validate the path.

        Args:
            redir2: The redirection expression, starting with ">".

        Returns:
            Path and mode.

        Raises:
            DTShRedirect.Error: Invalid redirection string.
        """
        if not redir2.startswith(">"):
            raise DTShRedirect.Error(f"invalid redirection: '{redir2}'")

        if redir2.startswith(">>"):
            append = True
            path = redir2[2:].strip()
        else:
            append = False
            path = redir2[1:].strip()

        if not path:
            raise DTShRedirect.Error(
                "don't know where to redirect the output to ?"
            )

        return (path, append)

    _path: str
    _append: bool
    _out: IO[str]

    def __init__(self, redir2: str) -> None:
        """New redirection.

        Args:
            redir2: The redirection expression, starting with ">" or ">>",
              followed by a file path.

        Raises:
            DTShRedirect.Error: Forbidden spaces or overwrite.
        """
        path, append = self.parse_redir2(redir2)

        if _dtshconf.pref_fs_no_spaces and " " in path:
            raise DTShRedirect.Error(
                f"spaces not allowed in redirection: '{path}'"
            )

        if path.startswith("~"):
            # abspath() won't expand a leading "~".
            path = path.replace("~", os.path.expanduser("~"), 1)
        path = os.path.abspath(path)

        if os.path.isfile(path):
            err_exists = False
            if _dtshconf.pref_fs_no_overwrite:
                if append:
                    if _dtshconf.pref_fs_no_overwrite_strict:
                        err_exists = True
                else:
                    err_exists = True

            if err_exists:
                raise DTShRedirect.Error(f"file exists: '{path}'")
        else:
            # We won't actually append if the file does not exist,
            # checking this here will help actual DTShOutput implementations
            # to do the right thing.
            append = False

        self._append = append
        self._path = path

    @property
    def append(self) -> bool:
        """Whether to redirect output in append-mode.

        Can be true only when the output file actually already exists.
        """
        return self._append

    @property
    def path(self) -> str:
        """Redirection real path."""
        return self._path


class DTShInputFile(DTShInput):
    """Shell input file.

    Text file containing shell command lines, one per line.
    """

    class Error(BaseException):
        """Failed to open input file for reading."""

    _in: IO[str]

    def __init__(self, path: str) -> None:
        """Initialize input stream.

        Args:
            path: Path to input file.

        Raises:
            DTShInputFile.Error: Failed to open input file.
        """
        try:
            self._in = open(  # pylint: disable=consider-using-with
                path, "r", encoding="utf-8"
            )
        except OSError as e:
            raise DTShInputFile.Error(str(e)) from e

    def readline(self, multi_prompt: Optional[Sequence[Any]] = None) -> str:
        """Overrides DTShInput.readline()."""
        line: str = self._in.readline()

        # Skip empty and comment lines.
        while line and not self._is_cmdline(line):
            line = self._in.readline()

        if line:
            # Strip trailing "\n".
            return line.rstrip()

        self._in.close()
        raise EOFError()

    def _is_cmdline(self, line: str) -> bool:
        # A non-empty line should be a command line if not only spaces,
        # and not a comment.
        return not (line.isspace() or line.startswith("#"))


class DTShInputCmds(DTShInput):
    """Command-line argument source.

    Returns commands as they are provided on the tool command line.
    """

    _cmds: List[str]

    def __init__(self, cmds: List[str]) -> None:
        """Initialize object.

        Args:
            cmds: List of commands to execute.
        """
        self._cmds = cmds

    def readline(self, multi_prompt: Optional[Sequence[Any]] = None) -> str:
        """Overrides DTShInput.readline()."""
        if self._cmds:
            line: str = self._cmds.pop(0)
            return line

        raise EOFError()
