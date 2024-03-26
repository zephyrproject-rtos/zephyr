# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Rich I/O streams for devicetree shells.

- rich VT
- rich redirection streams for SVG and HTML

Rich I/O streams implementations are based on the rich.console module.
"""


from typing import Any, IO, List, Mapping, Optional

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
from dtsh.io import DTShVT, DTShOutput, DTShRedirect

from dtsh.rich.theme import DTShTheme
from dtsh.rich.svg import SVGContentsFmt, SVGContents

_dtshconf: DTShConfig = DTShConfig.getinstance()
_theme: DTShTheme = DTShTheme.getinstance()


class DTShRichVT(DTShVT):
    """Rich terminal for devicetree shells."""

    _console: Console
    _pager: Optional[PagerContext]

    def __init__(self) -> None:
        """Initialize VT."""
        super().__init__()
        self._console = Console(theme=Theme(_theme.styles), highlight=False)
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
            self._pager.__enter__()  # pylint: disable=unnecessary-dunder-call

    def pager_exit(self) -> None:
        """Overrides DTShOutput.pager_exit()."""
        if self._pager:
            self._pager.__exit__(None, None, None)
            self._pager = None


class DTShOutputFileText(DTShOutput):
    """Text output file for commands output redirection."""

    _out: IO[str]
    _console: Console

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

        self._console = Console(
            highlight=False,
            theme=Theme(_theme.styles),
            record=True,
            # Set the console's width to the configured maximum,
            # we'll strip the rich segments on flush.
            width=_dtshconf.pref_redir2_maxwidth,
        )

    def write(self, *args: Any, **kwargs: Any) -> None:
        """Capture command's output.

        Overrides DTShOutput.write().

        Args:
            *args: Positional arguments, Console.print() semantic.
            **kwargs: Keyword arguments, Console.print() semantic.
        """
        with self._console.capture():
            self._console.print(*args, **kwargs)

    def flush(self) -> None:
        """Format (HTML) the captured output and write it
        to the redirection file.

        Overrides DTShOutput.flush().
        """
        contents = self._console.export_text()
        # Exported lines are padded up to the (maximum) console width:
        # strip these trailing whitespaces, which could make the text file
        # unreadable.
        for line_nopad in (line.rstrip() for line in contents.splitlines()):
            print(line_nopad, file=self._out)
        self._out.close()


class DTShOutputFileHtml(DTShOutput):
    """HTML output file for commands output redirection."""

    _out: IO[str]
    _append: bool
    _console: Console

    def __init__(self, path: str, append: bool) -> None:
        """Initialize output file.

        Args:
            path: The output file path.
            append: Whether to redirect the command's output in "append" mode.

        Raises:
             DTShRedirect.Error: Invalid path or permission errors.
        """
        try:
            self._append = append
            self._out = open(  # pylint: disable=consider-using-with
                path,
                "r+" if append else "w",
                encoding="utf-8",
            )
        except OSError as e:
            raise DTShRedirect.Error(e.strerror) from e

        self._console = Console(
            highlight=False,
            theme=Theme(_theme.styles),
            record=True,
            # Set the console's width to the configured maximum,
            # we'll post-process the generated HTML document on flush.
            width=_dtshconf.pref_redir2_maxwidth,
        )

        if self._append:
            # Write a blank line to the captured output
            # as a commands separator when we append.
            self.write()

    def write(self, *args: Any, **kwargs: Any) -> None:
        """Capture command's output.

        Overrides DTShOutput.write().

        Args:
            *args: Positional arguments, Console.print() semantic.
            **kwargs: Keyword arguments, Console.print() semantic.
        """
        with self._console.capture():
            self._console.print(*args, **kwargs)

    def flush(self) -> None:
        """Format (HTML) the captured output and write it
        to the redirection file.

        Overrides DTShOutput.flush().
        """
        # Text and bakcround colors.
        theme = DTSH_EXPORT_THEMES.get(
            _dtshconf.pref_html_theme, DEFAULT_TERMINAL_THEME
        )

        html_fmt = DTSH_HTML_FORMAT.replace(
            "|font_family|", _dtshconf.pref_html_font_family
        )

        html = self._console.export_html(
            theme=theme,
            code_format=html_fmt,
            # Use inline CSS styles in "append" mode.
            inline_styles=self._append,
        )

        # The generated HTML pad lines with withe spaces
        # up to the console's width, which is ugly if you
        # want to re-use the HTML source: clean this up.
        html_lines: List[str] = [line.rstrip() for line in html.splitlines()]

        # Index of the first line we'll write to the HTML output file:
        # - either 0, pointing to the first line for the current redirection
        #   contents, if we're creating a new file
        # - or, in "append" mode, the index of the line containing
        #   the <pre> tag that represents the actual command's output
        #
        # Since this <pre> tag appears immediately before the HTML epilog,
        # we'll then just have to write the current re-direction's contents
        # starting from this index.
        i_output: int = 0

        if self._append:
            # Appending to an existing file: seek to the appropriate
            # point of insertion.
            self._seek_last_content()
            self._out.write(os.linesep)

            # Find command's output contents.
            for i, line in enumerate(html_lines):
                if line.find("<pre") != -1:
                    i_output = i
                    break

        for line in html_lines[i_output:]:
            print(line, file=self._out)
        self._out.close()

    def _seek_last_content(self) -> None:
        # Offset for the point of insertion, just before the HTML epilog.
        offset: int = self._out.tell()
        line = self._out.readline()
        while line and not line.startswith("</body>"):
            offset = self._out.tell()
            line = self._out.readline()
        self._out.seek(offset, os.SEEK_SET)


class DTShOutputFileSVG(DTShOutput):
    """SVG output file for commands output redirection."""

    _out: IO[str]
    _append: bool
    _console: Console

    _maxwidth: int
    _width: int

    def __init__(self, path: str, append: bool) -> None:
        """Initialize output file.

        Args:
            path: The output file path.
            append: Whether to redirect the command's output in "append" mode.

        Raises:
             DTShRedirect.Error: Invalid path or permission errors.
        """
        try:
            self._append = append
            self._out = open(  # pylint: disable=consider-using-with
                path,
                "r+" if append else "w",
                encoding="utf-8",
            )
        except OSError as e:
            raise DTShRedirect.Error(e.strerror) from e

        # Maximum width allowed in preferences.
        self._maxwidth = _dtshconf.pref_redir2_maxwidth

        # Width required to output the command's output without wrapping or
        # cropping, up to the configured maximum.
        self._width = 0

        self._console = Console(
            highlight=False,
            theme=Theme(_theme.styles),
            record=True,
            # Set the console's width to the configured maximum,
            # we'll shrink it on flush.
            width=self._maxwidth,
        )

        if self._append:
            # Write a blank line to the captured output
            # as a commands separator when we append.
            self.write()

    def write(self, *args: Any, **kwargs: Any) -> None:
        """Capture command's output.

        Overrides DTShOutput.write().

        Args:
            *args: Positional arguments, Console.print() semantic.
            **kwargs: Keyword arguments, Console.print() semantic.
        """
        # Update required width.
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
                if (measure.maximum > self._width) and not (
                    measure.maximum > self._maxwidth
                ):
                    self._width = measure.maximum

        with self._console.capture():
            self._console.print(*args, **kwargs)

    def flush(self) -> None:
        """Format (SVG) the captured output and write it
        to the redirection file.

        Overrides DTShOutput.flush().
        """
        # Text and bakcround colors.
        theme = DTSH_EXPORT_THEMES.get(
            _dtshconf.pref_svg_theme, DEFAULT_TERMINAL_THEME
        )

        # Shrink the console's width, to prevent the SVG document from being
        # unnecessarily wide.
        self._console.width = self._width

        # Get captured command output as SVG contents lines.
        contents: List[str] = self._console.export_svg(
            theme=theme,
            title="",
            code_format=SVGContentsFmt.get_format(
                _dtshconf.pref_svg_font_family
            ),
            font_aspect_ratio=_dtshconf.pref_svg_font_ratio,
        ).splitlines()

        svg: SVGContents
        try:
            if self._append:
                svg = self._svg_append(contents)
            else:
                svg = self._svg_create(contents)

        except SVGContentsFmt.Error as e:
            raise DTShRedirect.Error(str(e)) from e

        self._svg_write(svg)
        self._out.close()

    def _svg_create(self, contents: List[str]) -> SVGContents:
        svg: SVGContents = SVGContents(contents)
        svg.top_padding_correction()
        return svg

    def _svg_append(self, contents: List[str]) -> SVGContents:
        svgnew: SVGContents = SVGContents(contents)
        svgnew.top_padding_correction()

        svg: SVGContents = self._svg_read()
        svg.append(svgnew)
        return svg

    def _svg_read(self) -> SVGContents:
        contents: List[str] = self._out.read().splitlines()
        self._out.seek(0, os.SEEK_SET)

        # Padding correction was already done.
        svg = SVGContents(contents)
        return svg

    def _svg_write(self, svg: SVGContents) -> None:
        self._svg_write_prolog()
        self._svg_writeln()

        self._svg_write_styles(svg)
        self._svg_writeln()

        self._svg_write_defs(svg)
        self._svg_writeln()

        self._svg_write_chrome(svg)
        self._svg_writeln()

        self._svg_write_gterms(svg)
        self._svg_write_epilog()

    def _svg_write_prolog(self) -> None:
        print(SVGContentsFmt.PROLOG, file=self._out)

    def _svg_write_styles(self, svg: SVGContents) -> None:
        print(SVGContentsFmt.CSS_STYLES_BEGIN, file=self._out)
        for line in svg.styles:
            print(line, file=self._out)
        print(SVGContentsFmt.CSS_STYLES_END, file=self._out)

    def _svg_write_defs(self, svg: SVGContents) -> None:
        print(SVGContentsFmt.SVG_DEFS_BEGIN, file=self._out)
        for line in svg.defs:
            print(line, file=self._out)
        print(SVGContentsFmt.SVG_DEFS_END, file=self._out)

    def _svg_write_chrome(self, svg: SVGContents) -> None:
        print(SVGContentsFmt.MARK_CHROME, file=self._out)
        print(svg.rect, file=self._out)

    def _svg_write_gterms(self, svg: SVGContents) -> None:
        for gterm in svg.gterms:
            print(SVGContentsFmt.MARK_GTERM_BEGIN, file=self._out)
            for line in gterm.contents:
                print(line, file=self._out)
            print(SVGContentsFmt.MARK_GTERM_END, file=self._out)
            self._svg_writeln()

    def _svg_write_epilog(self) -> None:
        print(SVGContentsFmt.EPILOG, file=self._out)

    def _svg_writeln(self) -> None:
        print(file=self._out)


DTSH_HTML_FORMAT = """\
<!DOCTYPE html>
<html>

<head>
<meta charset="UTF-8">

<style>
{stylesheet}
body {{
    color: {foreground};
    background-color: {background};
}}
</style>
</head>

<body>

    <pre style="font-family:|font_family|, monospace"><code>{code}</code></pre>

</body>

</html>
"""

DTSH_EXPORT_THEMES: Mapping[str, TerminalTheme] = {
    "svg": SVG_EXPORT_THEME,
    "html": DEFAULT_TERMINAL_THEME,
    "dark": DIMMED_MONOKAI,
    "light": NIGHT_OWLISH,
    "night": MONOKAI,
}
