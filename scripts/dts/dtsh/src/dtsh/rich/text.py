# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Text view factories and other text related helpers.

Factories for styled and actionable (aka linked) rich text views,
and miscellaneous text related helpers.
"""

from typing import Optional, Union, Iterable

from urllib.parse import urlparse
import os
import pathlib

from rich.console import JustifyMethod
from rich.style import Style, StyleType
from rich.text import Text

from dtsh.config import DTShConfig, ActionableType
from dtsh.rich.theme import DTShTheme


_dtshconf: DTShConfig = DTShConfig.getinstance()


class TextUtil:
    """Text view factories."""

    @classmethod
    def mk_text(
        cls,
        content: str,
        style: Optional[StyleType] = None,
        justify: Optional[JustifyMethod] = None,
    ) -> Text:
        """Text view factory.

        Args:
            content:
            style:
            justify:

        Returns:
            A new text view.
        """
        return Text(
            content,
            style=style or DTShTheme.STYLE_DEFAULT,
            justify=justify,
        )

    @classmethod
    def dim(cls, txt: Union[str, Text]) -> Text:
        """Dim an existing text view or create a new "dim" text.

        Args:
            txt: A text view or a string.

        Returns:
            A dim text view.
        """
        if isinstance(txt, str):
            return cls.mk_text(txt, "dim")
        txt.stylize("dim")
        return txt

    @classmethod
    def bold(cls, text: Union[str, Text]) -> Text:
        """Bold an existing text view or create a new "bold" text.

        Args:
            text: A text view or a string.

        Returns:
            A bold text view.
        """
        if isinstance(text, str):
            return cls.mk_text(text, "bold")
        text.stylize("bold")
        return text

    @classmethod
    def italic(cls, text: Union[str, Text]) -> Text:
        """Emphasize an existing text view or create a "italic" text.

        Args:
            text: A text view or a string.

        Returns:
            An italic text view.
        """
        if isinstance(text, str):
            return cls.mk_text(text, "italic")
        text.stylize("italic")
        return text

    @classmethod
    def underline(cls, text: Union[str, Text]) -> Text:
        """Underline an existing text view or create a new "underline" text.

        Args:
            text: A text view or a string.

        Returns:
            An underlined text view.
        """
        if isinstance(text, str):
            return cls.mk_text(text, "underline")
        text.stylize("underline")
        return text

    @classmethod
    def strike(cls, text: Union[str, Text]) -> Text:
        """Strike an existing text view or create a new "strike" text.

        Args:
            text: A text view or a string.

        Returns:
            An stroked text view.
        """
        if isinstance(text, str):
            return cls.mk_text(text, "strike")
        text.stylize("strike")
        return text

    @classmethod
    def disabled(cls, text: Union[str, Text]) -> Text:
        """Style text item as disabled.

        Args:
            text: A text view or a string.

        Returns:
            An text view styled as disabled.
        """
        if isinstance(text, str):
            return cls.mk_text(text, DTShTheme.STYLE_DISABLED)
        text.stylize(DTShTheme.STYLE_DISABLED)
        return text

    @classmethod
    def link(
        cls,
        text: Union[str, Text],
        uri: str,
        linktype: Optional[ActionableType] = None,
    ) -> Text:
        """Link text to file or URI.

        Actual links rendering, and whether they are actually actionable,
        will eventually depend on the host terminal.

        Args:
            text: A text view or the string to create a view of.
            uri: The destination URI.
            linktype: How to represent actionable text.

        Returns:
            An actionable text view.
        """
        if isinstance(text, str):
            text = TextUtil.mk_text(text)

        linktype = linktype or _dtshconf.pref_actionable_type
        if linktype is ActionableType.NONE:
            return text

        scheme = urlparse(uri).scheme
        if not scheme:
            # Assume "file" URI scheme when missing.
            uri = pathlib.Path(os.path.abspath(uri)).as_uri()

        if linktype is ActionableType.ALT:
            # Append actionable text.
            actionable = TextUtil.mk_text(
                _dtshconf.pref_actionable_text, style=text.style
            )
            actionable.stylize(Style(link=uri))
            text = TextUtil.join(" ", (text, actionable))
        else:
            # Make text itself actionable (ActionableType.LINK).
            text.stylize(Style(link=uri))

        return text

    @classmethod
    def mk_headline(
        cls, content: str, style: Optional[Union[str, Style]] = None
    ) -> Text:
        """Extract headline of a multi-line content.

        Args:
            content: Multi-line content.
            style: Style or style name.

        Returns:
            A text view.
        """
        headline, _, rest = content.lstrip().partition("\n")
        is_multiline = bool(rest)

        if is_multiline:
            if headline.endswith("."):
                # Remove trailing dot if ellipsis.
                headline = headline[:-1]
            headline = f"{headline}{_dtshconf.wchar_ellipsis}"

        return cls.mk_text(headline, style)

    @classmethod
    def join(cls, sep: Union[str, Text], parts: Iterable[Text]) -> Text:
        """Join rich text elements."""
        if isinstance(sep, str):
            sep = cls.mk_text(sep)
        return sep.join(parts)

    @classmethod
    def assemble(cls, *parts: Union[Text, str]) -> Text:
        """Assemble Text views into one.

        Args:
            parts: The views to assemble.
        """
        # Note: Text.append_tokens(), Text.append_text() and such
        # would "merge" the Text styles.
        return Text.assemble(*parts)
