# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""GNU Readline integration.

Rationale:

- provide DTSh client code with a single entry-point API for
  GNU Readline integration
- isolate the completion logic (find matches) and view providers
  (display matches) from the readline hooks machinery

The GNU Readline integration with Python is initialized:

- with the standalone Python module gnureadline if installed: this
  is likely necessary on macOS, where readline is typically backed by libedit
  instead of libreadline
- with the readline module of the Python standard library on other POSIX systems

On windows, the readline support will likely be disabled.
"""


from typing import Callable, Sequence, List, Optional

import os
import sys

from dtsh.config import DTShConfig
from dtsh.io import DTShOutput

_has_readline = False

try:
    # Allow all POSIX-like systems to load the gnureadline stand-alone module.
    # https://pypi.org/project/gnureadline/
    import gnureadline as readline  # type: ignore

    _has_readline = True
except ImportError:
    try:
        import readline

        _has_readline = True
    except ImportError:
        print("GNU readline support disabled.", file=sys.stderr)


_dtshconf: DTShConfig = DTShConfig.getinstance()


class DTShReadline:
    """GNU Readline integration.

    Callback-based API that isolates the completion (find matches)
    and view (display matches) providers from the GNU Readline machinery.
    """

    class CompleterState:
        """A completer state is a completion candidate.

        States are initialized by the RL completion hook callback.
        """

        _rlstr: str
        _item: Optional[object]

        def __init__(self, rlstr: str, item: Optional[object]) -> None:
            """Initialize completer state.

            Args:
                rlstr: The string to substitute the RL completion scope with,
                  e.g. a command  name or a Devicetree path.
                  Must have at least the length of the completion scope.
                item: The corresponding model object, if any,
                  e.g. a command option or a Devicetree node.
            """
            self._rlstr = rlstr
            self._item = item

        @property
        def rlstr(self) -> str:
            """The string to substitute the RL completion scope with."""
            return self._rlstr

        @property
        def item(self) -> Optional[object]:
            """The corresponding model object, if any."""
            return self._item

        def __eq__(self, other: object) -> bool:
            """States equal when their RL substitution strings equal."""
            if isinstance(other, DTShReadline.CompleterState):
                return self._rlstr == other._rlstr
            return False

        def __lt__(self, other: object) -> bool:
            """Alphabetical order of RL substitution strings."""
            if isinstance(other, DTShReadline.CompleterState):
                return self._rlstr < other._rlstr
            raise ValueError(other)

        def __repr__(self) -> str:
            return self._rlstr

    CompletionCallback = Callable[
        [str, str, int, int], List["DTShReadline.CompleterState"]
    ]
    """Completer states provider callback prototype.

    That's where the completion logic is implemented.

    Args:
        cs_txt: The text to complete,
          i.e. the content of the Readline completion scope.
        rlbuf: The current command line,
          i.e. the Readline buffer's content.
        cs_begin: The index within the Readline buffer where
          the completion scope starts.
        cs_end: The index within the Readline buffer's where
          the completion scope ends.

    Returns:
        The list of completer states.
    """

    DisplayCallback = Callable[
        [DTShOutput, List["DTShReadline.CompleterState"]], None
    ]
    """Completer states display callback prototype.

    That's where the completion matches are displayed.

    Args:
        out: Where to display the completion matches.
        states: The completer states to display.
    """

    # Completer states provider.
    _completion_callback: CompletionCallback

    # Optional completion views provider.
    # If unset, defaults to the readline module's implementation.
    _display_callback: Optional[DisplayCallback]

    # Completer states, See rl_complete().
    _completer_states: List["DTShReadline.CompleterState"]

    # Where to display completion matches and restore the command line.
    _stdout: DTShOutput

    # Path to the command history file.
    _histfile: str

    def __init__(
        self,
        stdout: DTShOutput,
        completion_callback: "DTShReadline.CompletionCallback",
        display_callback: Optional["DTShReadline.DisplayCallback"] = None,
    ) -> None:
        """Initialize GNU readline integration.

        If readline support is enabled, read history file and set RL hooks.

        Args:
            stdout: Bound terminal to restore the command line after the
              completion matches display callback has been called.
            completion_callback: The completions provider callback.
            display_callback: The completion matches display callback.
        """
        self._stdout = stdout
        self._completer_states = []
        self._completion_callback = completion_callback
        self._display_callback = display_callback

        self._histfile = _dtshconf.get_user_file("history")
        if _has_readline:
            self._rl_init()
            self.read_history()

    def read_history(self) -> Optional[str]:
        """Load command history file.

        Returns:
            The history file path, or None if the history file is unavailable.
        """
        if _has_readline:
            if os.path.isfile(self._histfile):
                try:
                    readline.read_history_file(self._histfile)
                    return self._histfile
                except OSError as e:
                    print(
                        f"Failed to read command history: {e}", file=sys.stderr
                    )
        return None

    def save_history(self) -> Optional[str]:
        """Write command history file.

        Returns:
            The history file path, or None if failed to save history.
        """
        if _has_readline:
            try:
                readline.write_history_file(self._histfile)
                return self._histfile
            except OSError as e:
                print(f"Failed to write command history: {e}", file=sys.stderr)
        return None

    def rl_complete(self, cs_txt: str, state: int) -> Optional[str]:
        """Setup the GNU readline's completion hook.

        Actual completion is delegated to the completions provider callback.

        Args:
            cs_txt: The readline completion scope content.
            state: The state integer that iterates on the completion
              candidates.
        """
        if state == 0:
            self._completer_states.clear()

            rlbuf = readline.get_line_buffer()
            begin = readline.get_begidx()
            end = readline.get_endidx()

            self._completer_states = self._completion_callback(
                cs_txt, rlbuf, begin, end
            )

        try:
            return self._completer_states[state].rlstr
        except IndexError:
            # No completion.
            pass
        return None

    def rl_display_matches_hook(
        self,
        substitution: str,
        matches: Sequence[str],
        longest_match_length: int,
    ) -> None:
        """Setup the GNU readline's display matches hook.

        Actual display is delegated to the completions display callback.

        Command line content and cursor position are eventually restored.
        """
        if not self._display_callback:
            raise NotImplementedError()

        del substitution  # Unused.
        del longest_match_length  # Unused.
        if not matches:
            return

        rlbuf = readline.get_line_buffer()
        end = readline.get_endidx()
        # Backward steps needed to restore the cursor position.
        n_back = len(rlbuf) - end

        # Move cursor bellow input line.
        self._stdout.write()

        # Display completions.
        self._display_callback(self._stdout, self._completer_states)

        # Restore command line.
        self._stdout.write(_dtshconf.prompt_default, end="")
        self._stdout.write(rlbuf, end="")
        if n_back > 0:
            # Move cursor back to insertion point.
            # ANSI Cursor Back: CSI n D
            self._stdout.write(f"\033[{n_back}D", end="")
        # Update stdout without sending LF.
        self._stdout.flush()

    def _rl_init(self) -> None:
        readline.set_completer(self.rl_complete)
        readline.set_completer_delims(f" \t{os.linesep}")
        readline.parse_and_bind("tab: complete")

        if self._display_callback:
            # Enable custom display callback if asked to.
            readline.set_completion_display_matches_hook(
                self.rl_display_matches_hook
            )
