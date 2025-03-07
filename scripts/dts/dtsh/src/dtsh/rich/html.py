# Copyright (c) 2024 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Helpers for redirecting commands output to HTML files.

Provides a simple model layer between the rich library Console.export_html() API
and DTSh commands output redirection to HTML files.

Unit tests and examples: tests/test_dtsh_rich_html.py
"""

from typing import List, Tuple, IO

import os
import re

from rich.console import Console
from rich.terminal_theme import TerminalTheme


class HtmlFormat:
    """DTSh HTML format."""

    class Error(BaseException):
        """Unexpected or invalid SVG content."""

        def __init__(self, cause: str) -> None:
            super().__init__(cause)

    RE_STYLE_BEGIN = re.compile(r"^\s*<style>\s*$")
    RE_STYLE_END = re.compile(r"^\s*</style>\s*$")
    RE_STYLE_RCLASS = re.compile(r"\.r(?P<n>[\d]+) {")

    RE_CODE_BEGIN = re.compile(r"^\s*<pre")
    # Not always on its own line (may end on the same line it begins).
    RE_CODE_END = re.compile(r"</code></pre>\s*$")

    @staticmethod
    def ifind(
        txt: List[str], pattern: re.Pattern[str], start: int
    ) -> Tuple[int, re.Match[str]]:
        """Find match in a multi-line text.

        Stops on the first matching line.

        Args:
            txt: Multi-line HTML text to search for.
            pattern: The pattern to match.
            start: Start the search from here.

        Returns: A tuple containing the index of the matching text line
            and the RE match.

        Raises:
            HtmlFormat.Error: Pattern not found.
        """
        for i, line in enumerate(txt[start:]):
            m = pattern.match(line)
            if m:
                return (i + start, m)
        raise HtmlFormat.Error(pattern.pattern)

    @staticmethod
    def mk_format(font_family: str, font_size: str) -> str:
        """Initialize HTML format string for given font.

        Args:
            font_family: CSS font family (see pref_html_font_family).
            font_size: CSS font size (see pref_html_font_size).

        Returns:
            A format string compatible with the Console.export_html() API.
        """
        font_family = f"{font_family},monospace" if font_family else "monospace"
        fmt: str = DTSH_HTML_META_FORMAT.replace("|font_family|", font_family)
        fmt = fmt.replace("|font_size|", font_size or "medium")
        return fmt


class HtmlFragment:
    """Base for HTML document parts we're interested in."""

    _endl: int

    # Initialized with the parsed content lines.
    # May be overwritten by sub-classes.
    _txt: List[str]

    def __init__(self, endl: int, txt: List[str]) -> None:
        self._endl = endl
        self._txt = txt

    @property
    def i_end(self) -> int:
        """Index of the last HTML line parsed within this element."""
        return self._endl

    @property
    def content(self) -> List[str]:
        """HTML formatted content (multi-line)."""
        return self._txt


class HtmlStyle(HtmlFragment):
    """Contents of the <style> element in the HTML format.

    <style>
    .r1 {font-weight: bold}
    .r2 {...}
    .rN {...}
    body {
        color: {foreground};
        background-color: {background};
    }
    </style>
    """

    @staticmethod
    def ifind(txt: List[str], start: int = 0) -> "HtmlStyle":
        """Find HTML <style> element.

        Args:
            txt: Multi-line HTML text to search for.
            start: Start the search from here.
        """
        i_begin, _ = HtmlFormat.ifind(txt, HtmlFormat.RE_STYLE_BEGIN, start)
        i_end, _ = HtmlFormat.ifind(txt, HtmlFormat.RE_STYLE_END, i_begin + 1)
        return HtmlStyle(i_end, txt[i_begin : i_end + 1])

    # One per line:
    # .r(?P<n>[\d]+)
    _rclasses: List[str]

    # body {
    #     color: {foreground};
    #     background-color: {background};
    # }
    _body: List[str]

    def __init__(self, endl: int, txt: List[str]) -> None:
        super().__init__(endl, txt)

        # Skip the opening <style> line.
        offset: int = 1

        self._rclasses = []
        for line in txt[offset:]:
            if HtmlFormat.RE_STYLE_RCLASS.match(line):
                self._rclasses.append(line)
                offset += 1
            else:
                # Last .rN class definition.
                break

        # Strip the closing </style> line.
        self._body = txt[offset:-1]

    @property
    def rclasses(self) -> List[str]:
        """Style classes definitions."""
        return self._rclasses

    @property
    def nclasses(self) -> int:
        """Number of style classes."""
        return len(self._rclasses)

    @property
    def body(self) -> List[str]:
        """Style <body>."""
        return self._body

    @property
    def content(self) -> List[str]:
        # Content in self._txt may have been obsoleted
        # by calling shift_rclasses().
        return ["<style>", *self._rclasses, *self._body, "</style>"]

    def shift_rclasses(self, shift: int) -> None:
        """Shift the class identifiers.

        When these CSS styles belong to an HTML formatted capture
        that is appended to an existing HTML file,
        class identifiers must be shifted according to the classes
        defined by the file we're appending to.

        Args:
            shift: Class identifiers shift. This is the number of CSS classes
                already defined by the file we're appending to.
        """
        self._rclasses = [
            # CSS class identifiers we get are 1-indexed.
            rclass.replace(f".r{i+1} {{", f".r{i+1+shift} {{", 1)
            for i, rclass in enumerate(self._rclasses)
        ]


class HtmlPreformatted(HtmlFragment):
    """Contents of the <pre> element in the HTML format.

    <pre style="..."><code style="..."><span class="r1">...</span>
    <span class="r2">...</span>
    ...
    </code></pre>
    """

    @staticmethod
    def ifind(txt: List[str], start: int = 0) -> "HtmlPreformatted":
        """Find HTML <style> element."""
        i_begin, _ = HtmlFormat.ifind(txt, HtmlFormat.RE_CODE_BEGIN, start)
        # May be on the same line.
        i_end, _ = HtmlFormat.ifind(txt, HtmlFormat.RE_CODE_END, i_begin)
        return HtmlPreformatted(i_end, txt[i_begin : i_end + 1])

    @staticmethod
    def ifind_list(txt: List[str], start: int = 0) -> List["HtmlPreformatted"]:
        """Find successive <pre><code>s in a file we're appending to.

        Args:
            txt: The HTML text to search.
            start: Start the search from here.

        Returns: A tuple containing the index of the line matching
            the end of the last <pre><code>, and the matched fragments.
        """
        pre_code: HtmlPreformatted = HtmlPreformatted.ifind(txt, start)
        pre_codes: List[HtmlPreformatted] = [pre_code]
        try:
            while True:
                pre_code = HtmlPreformatted.ifind(txt, pre_code.i_end + 1)
                pre_codes.append(pre_code)
        except HtmlFormat.Error:
            # Last <pre><code>.
            pass
        return pre_codes

    def shift_rclasses(self, nclasses: int, shift: int) -> None:
        """Shift the class identifiers.

        When we're appending this <pre><code> to an existing HTML file:
        - the class identifiers defined in this last captured output
          are shifted to not overlap with the ones already defined in
          the file we're appending to: see HtmlStyle.shift_rclasses()
        - the class identifiers referenced in the appended contents
          should also be shifted accordingly,
          which is the purpose of this function

        Args:
            nclasses: Number of CSS classes defined by the last captured output.
            shift: Class identifiers shift. This is the number of CSS classes
                already defined by the file we're appending to.
        """
        # Rather than working line by line, then classes by classes,
        # i.e. something like O(lines X classes), build one single string
        # for the whole content, then for each class compile and apply an RE
        # on this string, and eventually split back the string into lines.
        txt: str = "\n".join(self._txt)

        # Reverse the substitution order so as not to shoot yourself
        # in the foot, e.g.:
        # r1 -> r4
        # ....
        # r4 -> r7 ;-)
        for classid in reversed(range(1, nclasses + 1)):
            re_class: re.Pattern[str] = re.compile(f'class="r{classid}"')
            repl: str = f'class="r{classid+shift}"'

            txt = re.sub(re_class, repl, txt)

        self._txt = txt.splitlines()


class HtmlDocument:
    """HTML document.

    Can represent either the last command output,
    or an existing HTML file we're appending to.
    """

    @staticmethod
    def capture(
        console: Console,
        /,
        *,
        theme: TerminalTheme,
        font_family: str,
        font_size: str,
    ) -> "HtmlDocument":
        """Capture SVG document from console.

        Args:
            console: The capture console.
            theme: Terminal theme to apply (see pref.svg.theme).
            font_family: CSS font family (see pref.svg.font_family).
            font_size: CSS font size (see pref.svg.font_size).

        Raises:
            HtmlFormat.Error: Unexpected HTML format.
                Did the HTML text generated by the rich library change ?
        """
        fmt = HtmlFormat.mk_format(font_family, font_size)
        html = console.export_html(
            theme=theme, code_format=fmt, inline_styles=False
        )
        return HtmlDocument(html)

    @staticmethod
    def read(fhtml: IO[str]) -> "HtmlDocument":
        """Read HTML document from input stream (file).

        The OS cursor (aka seek) is returned to the beginning
        of the input stream after reading.

        Args:
            fhtml: HTML input stream.

        Raises:
            HtmlFormat.Error: Unexpected HTML format.
                May be appending to an HTML file generated by a different
                version of DTSh ?
            OSError: File access error.
        """
        html = fhtml.read()
        fhtml.seek(0, os.SEEK_SET)
        return HtmlDocument(html)

    _style: HtmlStyle
    _codes: List[HtmlPreformatted]

    def __init__(self, html: str) -> None:
        """Initialize HTML document.

        Args:
            html: HTML text.

        Raises:
            HtmlFormat.Error: Unexpected HTML format.
        """
        # The generated HTML pads lines with withe spaces
        # up to the console's width, which is ugly if you
        # want to edit the HTML source: clean this up.
        html_lines: List[str] = [line.rstrip() for line in html.splitlines()]

        # Parse HTML text into our fragments of interest.
        self._style = HtmlStyle.ifind(html_lines)
        self._codes = HtmlPreformatted.ifind_list(
            html_lines, self._style.i_end + 1
        )

    @property
    def style(self) -> HtmlStyle:
        """HTML <style> for this document."""
        return self._style

    @property
    def codes(self) -> List[HtmlPreformatted]:
        """HTML <pre><code>s in this document."""
        return self._codes

    @property
    def content(self) -> str:
        """HTML formatted document."""
        return DTSH_HTML_OUTPUT_FORMAT.format(
            style_rclasses="\n".join(self.style.rclasses),
            style_body="\n".join(self.style.body),
            pre_codes="\n".join(
                line for pre in self._codes for line in pre.content
            ),
        )

    def append(self, other_doc: "HtmlDocument") -> None:
        """Append other HTML document to this one.

        Append shifted CSS classes.
        Append last captured <pre><code> with shifted class identifiers.

        Args:
            other_doc: The document we're appending to this one.
        """
        # Shift class identifiers of the last captured command output.
        other_doc.shift_rclasses(self._style.nclasses)
        # And add them to this document.
        self._style.rclasses.extend(other_doc.style.rclasses)
        # Then append last captured command output
        # with shifted class identifiers.
        self._codes.append(other_doc.codes[0])

    def shift_rclasses(self, shift: int) -> None:
        """Shift CSS class identifiers document-wise.

        - shift CSS class identifiers to not overlap with the classes
          already defined by the HTML document we're appending to
        - shift accordingly the CSS class identifiers referenced
          in the <pre><code> we're appending to the exiting HTML file

        Args:
            shift: Class identifiers shift. This is the number of CSS classes
                already defined by the file we're appending to.
        """
        self._style.shift_rclasses(shift)
        if self._codes:
            # We're appending one single <pre><code>.
            pre_code = self._codes[0]
            pre_code.shift_rclasses(self._style.nclasses, shift)


# Format for generated files.
DTSH_HTML_OUTPUT_FORMAT = """\
<!DOCTYPE html>
<html>

<head>
<meta charset="UTF-8">

<style>
{style_rclasses}
{style_body}
</style>
</head>

<body>

    {pre_codes}

</body>

</html>
"""

# Meta-format for Console.export_html().
DTSH_HTML_META_FORMAT = """\
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

    <pre style="font-family:|font_family|; font-size:|font_size|"><code style="font-family:inherit">{code}</code></pre>

</body>

</html>
"""
