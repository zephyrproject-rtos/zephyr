# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Rich I/O streams for devicetree shells.

- rich VT
- rich redirection streams for SVG and HTML

Rich I/O streams implementations are based on the rich.console module.
"""


from typing import Any, IO, Mapping, Optional, Sequence

from io import StringIO
import os

from rich.console import Console, PagerContext
from rich.measure import Measurement
from rich.theme import Theme
from rich.terminal_theme import (
    SVG_EXPORT_THEME,
    DEFAULT_TERMINAL_THEME,
    MONOKAI,
    DIMMED_MONOKAI,
    NIGHT_OWLISH,
    TerminalTheme,
)

from dtsh.config import DTShConfig
from dtsh.io import DTShVT, DTShInput, DTShOutput, DTShRedirect

from dtsh.rich.html import HtmlDocument, HtmlFormat
from dtsh.rich.theme import DTShTheme
from dtsh.rich.svg import SVGDocument, SVGFormat

_dtshconf: DTShConfig = DTShConfig.getinstance()
_theme: DTShTheme = DTShTheme.getinstance()


class DTShRichVT(DTShVT):
    """Rich terminal for devicetree shells."""

    _console: Console
    _pager: Optional[PagerContext]

    def __init__(self) -> None:
        """Initialize VT."""
        super().__init__()
        self._console = Console(
            theme=Theme(_theme.styles), highlight=False, markup=False
        )
        self._pager = None

    def write(self, *args: Any, **kwargs: Any) -> None:
        """Write to rich console.

        Overrides DTShOutput.write().

        Args:
            *args: Positional arguments, Console.print() semantic.
            **kwargs: Keyword arguments, Console.print() semantic.

        """
        self._console.print(*args, **kwargs)

    def flush(self) -> None:
        """Flush rich console output without sending LF.

        Overrides DTShOutput.flush().
        """
        self._console.print("", end="")

    def clear(self) -> None:
        """Overrides DTShVT.clear()."""
        self._console.clear()

    def pager_enter(self) -> None:
        """Overrides DTShOutput.pager_enter()."""
        if not self._pager:
            self._console.clear()
            self._pager = self._console.pager(styles=True, links=True)
            # We have to explicitly enter the context since we need
            # more control on it than the context manager would permit.
            self._pager.__enter__()

    def pager_exit(self) -> None:
        """Overrides DTShOutput.pager_exit()."""
        if self._pager:
            self._pager.__exit__(None, None, None)
            self._pager = None


class DTShBatchRichVT(DTShRichVT):
    """Rich terminal for devicetree shells with batch mode support.

    Batch mode support, will:
    - first read command lines from the batch input stream until EOF
    - then, if interactive, read command lines from VT input stream until EOF
    """

    # Batch input stream, reset to None on EOF.
    _batch_istream: Optional[DTShInput]

    # Whether to read from VT after batch.
    _interactive: bool

    def __init__(self, batch_is: DTShInput, interactive: bool) -> None:
        """Initialize VT.

        Args:
            batch: Batch input stream.
            interactive: Whether to read from VT after batch
              (interactive sessions).
        """
        super().__init__()
        self._batch_istream = batch_is
        self._interactive = interactive

    def is_tty(self) -> bool:
        """Overrides DTShInput.is_tty()."""
        return self._batch_istream is None

    def readline(self, multi_prompt: Optional[Sequence[Any]] = None) -> str:
        """Overrides DTShVT.readline()."""
        if self._batch_istream:
            try:
                return self._batch_istream.readline(multi_prompt)
            except EOFError:
                # Exhausted batch input stream.
                self._batch_istream = None

        if not self._interactive:
            # Signal EOF if we don't continue in interactive mode.
            raise EOFError()
        return super().readline(multi_prompt)


class DTShOutputFile(DTShOutput):
    """Base output stream for redirecting commands output.

    Provides a Console object initialized for supporting this use case:
    - set initial width to the configured maximum
    - record what would otherwise be printed to the TTY
    - capture TTY output so that it's not also echoed to the TTY
    """

    # Records/captures outputs.
    _console: Console

    # Redirection file.
    _path: str
    # Whether to append (True only when the file actually exist).
    _append: bool

    def __init__(self, path: str, append: bool) -> None:
        """Initialize console for commands output redirection.

        Args:
            path: Path to the file we're redirecting to.
            append: True when appending to an existing file.
        """
        self._console = Console(
            highlight=False,
            markup=False,
            theme=Theme(_theme.styles),
            record=True,
            # Set the console's width to the configured maximum,
            # sub-classes will strip the rich segments on flush().
            width=_dtshconf.pref_redir2_maxwidth,
            # Capture output: we don't want to echo the output to the TTY
            # when redirecting to files.
            file=StringIO(),
        )
        self._path = path
        self._append = append

    @property
    def path(self) -> str:
        """Path to the file we're redirecting to."""
        return self._path

    @property
    def append(self) -> bool:
        """True when appending to an existing file."""
        return self._append

    def write(self, *args: Any, **kwargs: Any) -> None:
        """Capture and record outputs.

        Overrides DTShOutput.write().

        Args:
            *args: Positional arguments, Console.print() semantic.
            **kwargs: Keyword arguments, Console.print() semantic.
        """
        self._console.print(*args, **kwargs)


class DTShOutputFileText(DTShOutputFile):
    """Text output file for commands output redirection."""

    # True until we call write() to actually capture something.
    # If flush() is called before that, e.g. because the DTSh command failed,
    # it will abort early.
    _pending: bool = True

    def write(self, *args: Any, **kwargs: Any) -> None:
        """Overrides DTShOutputFile.write()."""
        if self._pending:
            self._pending = False
        super().write(*args, **kwargs)

    def flush(self) -> None:
        """Format the captured text and write it to the redirection file.

        Overrides DTShOutput.flush().
        """
        if self._pending:
            return

        try:
            with open(
                self.path, "a" if self.append else "w", encoding="utf-8"
            ) as out:
                self._flush(out)
        except OSError as e:
            raise DTShRedirect.Error(e.strerror) from e

    def _flush(self, out: IO[str]) -> None:
        if self._append:
            # Insert blank line between command outputs.
            out.write(os.linesep)

        # Exported lines are padded up to the (maximum) console width:
        # strip these trailing whitespaces, which could make the text file
        # unreadable.
        contents = self._console.export_text()
        for line_nopad in (line.rstrip() for line in contents.splitlines()):
            print(line_nopad, file=out)


class DTShOutputFileHtml(DTShOutputFile):
    """HTML output file for commands output redirection."""

    # True until we call write() to actually capture something.
    # If flush() is called before that, e.g. because the DTSh command failed,
    # it will abort early.
    _pending: bool = True

    def write(self, *args: Any, **kwargs: Any) -> None:
        """Capture and record outputs.

        Overrides DTShOutputFile.write().

        Args:
            *args: Positional arguments, Console.print() semantic.
            **kwargs: Keyword arguments, Console.print() semantic.
        """
        if self._pending:
            if self._append:
                # NOTE: commands output are formatted as successive HTML <pre>
                # elements, and inserting an additional vertical space is
                # very unlikely what the user wants: compact mode is the
                # default for HTML output redirection.
                if not _dtshconf.pref_html_compact:
                    super().write()
            # Capture is starting.
            self._pending = False

        super().write(*args, **kwargs)

    def flush(self) -> None:
        """Format (HTML) the captured output and write it
        to the redirection file.

        Overrides DTShOutput.flush().
        """
        if self._pending:
            return

        try:
            with open(
                self.path, "r+" if self.append else "w", encoding="utf-8"
            ) as redir2io:
                self._flush(redir2io)
        except OSError as e:
            raise DTShRedirect.Error(f"{self.path}: {e.strerror}") from e
        except HtmlFormat.Error as e:
            raise DTShRedirect.Error(
                f"unexpected HTML format, redirection canceled: {e}"
            ) from e

    def _flush(self, redir2io: IO[str]) -> None:
        # Get the captured command output we're dealing with.
        theme = DTSH_EXPORT_THEMES.get(
            _dtshconf.pref_html_theme, DEFAULT_TERMINAL_THEME
        )
        html_capture: HtmlDocument = HtmlDocument.capture(
            self._console,
            theme=theme,
            font_family=_dtshconf.pref_html_font_family,
            font_size=_dtshconf.pref_html_font_size,
        )

        html_doc: HtmlDocument
        if self._append:
            # Load the HTML file we're appending to.
            html_doc = HtmlDocument.read(redir2io)
            # Append the last captured command output.
            html_doc.append(html_capture)
        else:
            # Creating new file.
            html_doc = html_capture

        print(html_doc.content, file=redir2io)


class DTShOutputFileSVG(DTShOutputFile):
    """SVG output file for commands output redirection."""

    _maxwidth: int
    _width: int

    # Width of the current console line.
    # Note: the current line ends when write() is called so that
    # the rich library will assume end="\n".
    # This is required for computing the correct width when the output
    # is written in successive steps:
    #   out.write(foo, end=None)
    #   out.write(bar, end=None)
    #   out.write()
    _line_width: int

    def __init__(self, path: str, append: bool) -> None:
        """Initialize output file.

        Args:
            path: The output file path.
            append: Whether to redirect the command's output in "append" mode.

        Raises:
             DTShRedirect.Error: Invalid path or permission errors.
        """
        super().__init__(path, append)
        self._line_width = 0

        # Maximum width allowed in preferences.
        self._maxwidth = _dtshconf.pref_redir2_maxwidth

        # Width required to output the command's output without wrapping or
        # cropping, up to the configured maximum.
        self._width = 0

    def write(self, *args: Any, **kwargs: Any) -> None:
        """Record/capture output.

        Overrides DTShOutput.write().

        Args:
            *args: Positional arguments, Console.print() semantic.
            **kwargs: Keyword arguments, Console.print() semantic.
        """
        if self._append and not self._width:
            # When appending to an existing content (output file),
            # insert a blank line before we start to actually
            # capture the last command output.
            #
            # NOTE: we can't do that on flush, it will be too late,
            # the capture starts right bellow (that's how we can compute
            # the actual width of the command output).
            if not _dtshconf.pref_svg_compact:
                super().write()

        # Write output to console using the maximum width.
        super().write(*args, **kwargs)

        # Update actually required width.
        #
        # 1st, add the renderable arguments widths to the current line width.
        for arg in args:
            if (
                isinstance(arg, str)
                # Aka RichCast.
                or hasattr(arg, "__rich__")
                # Aka ConsoleRenderable.
                or hasattr(arg, "__rich_console__")
            ):
                measure = Measurement.get(
                    self._console, self._console.options, arg
                )
                self._line_width += measure.maximum

        endl: bool = kwargs.get("end", "\n") == "\n"
        if endl:
            # We've reached the end of the current console line: update
            # the required width up to the allowed maximum.
            if self._line_width > self._width:
                self._width = min(self._line_width, self._maxwidth)
            # Starting a new console line.
            self._line_width = 0

    def flush(self) -> None:
        """Format (SVG) the captured output and write it
        to the redirection file.

        Overrides DTShOutput.flush().
        """
        if not self._width:
            # Calling write() without argument or with empty rich objects
            # would append an unwanted blank line.
            return

        try:
            with open(
                self.path, "r+" if self.append else "w", encoding="utf-8"
            ) as out:
                self._flush(out)
        except OSError as e:
            raise DTShRedirect.Error(e.strerror) from e
        except SVGFormat.Error as e:
            raise DTShRedirect.Error(
                f"Unexpected SVG format, redirection canceled: {e}"
            ) from e

    def _flush(self, out: IO[str]) -> None:
        # Text and background colors.
        theme = DTSH_EXPORT_THEMES.get(
            _dtshconf.pref_svg_theme, DEFAULT_TERMINAL_THEME
        )

        # Shrink the console's width, to prevent the SVG document from being
        # unnecessarily wide.
        self._console.width = self._width

        # Get captured command output as SVG.
        svg_capture: SVGDocument = SVGDocument.capture(
            self._console,
            font_family=_dtshconf.pref_svg_font_family,
            font_ratio=_dtshconf.pref_svg_font_ratio,
            theme=theme,
            title=_dtshconf.pref_svg_title,
            show_gcircles=_dtshconf.pref_svg_decorations,
        )

        svg_doc: SVGDocument
        if self._append:
            # Load the SVG content we're appending to.
            svg_doc = SVGDocument(
                out.read().splitlines(),
                len(_dtshconf.pref_svg_title) > 0,
                _dtshconf.pref_svg_decorations,
            )
            out.seek(0, os.SEEK_SET)
            # Append the last command output capture.
            svg_doc.append(svg_capture)
        else:
            svg_doc = svg_capture

        for line in svg_doc.content:
            print(line, file=out)

        if self._append:
            # Append mode, cleanup once we're done.
            offset: int = out.tell()
            out.truncate(offset)


DTSH_EXPORT_THEMES: Mapping[str, TerminalTheme] = {
    "svg": SVG_EXPORT_THEME,
    "html": DEFAULT_TERMINAL_THEME,
    "dark": DIMMED_MONOKAI,
    "light": NIGHT_OWLISH,
    "night": MONOKAI,
}
