# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Rich console protocol.

Base view definitions compatible with the rich console protocol, __rich__().
"""


from typing import Optional, Union, Sequence

import rich.box
from rich.console import RenderableType
from rich.padding import PaddingDimensions, Padding
from rich.style import StyleType
from rich.table import Table
from rich.text import Text

from dtsh.config import DTShConfig, ActionableType
from dtsh.io import DTShOutput
from dtsh.shell import DTShCommand, DTShCommandError
from dtsh.shellutils import DTShFlagPager

from dtsh.rich.text import TextUtil
from dtsh.rich.theme import DTShTheme


class View:
    """View interface.

    DTSh user interface will print to kind of views:

    - rich API objects like Text
    - specialized views that implement this interface,
      typically named ViewXXX
    """

    PrintableType = Union[RenderableType | "View"]
    """Represent anything we may print() to the console,
    custom views, rich segments or ANSI strings.
    """

    SUB = Text()
    """View placecholder (empty Text)."""

    _ileft: int
    _itop: int

    def __init__(self) -> None:
        """New view."""
        self._ileft = 0
        self._itop = 0

    @property
    def renderable(self) -> RenderableType:
        """The rich console protocol object that represents this view.

        To be overridden by concrete views.
        """
        return View.SUB

    def left_indent(self, spaces: int) -> None:
        """Left-indent the view (left margin).

        Args:
            spaces: Indentation in number of characters.
        """
        self._ileft = spaces

    def top_indent(self, spaces: int) -> None:
        """Top-indent the view (top margin).

        Args:
            spaces: Indentation in number of characters.
        """
        self._itop = spaces

    def __rich__(self) -> RenderableType:
        """Rich console protocol."""
        view = self.renderable
        if self._ileft or self._itop:
            view = Padding(view, (self._itop, 0, 0, self._ileft))
        return view


class GridLayout(View):
    """Base for grid layouts.

    This is a simple vertical grid layout without header.
    """

    _grid: Table

    def __init__(
        self,
        ncols: int = 1,
        /,
        padding: PaddingDimensions = 0,
        no_wrap: bool = False,
        expand: bool = False,
    ) -> None:
        """Initialize layout.

        Args:
            ncols: Number of columns (default to 1).
            padding: Cells padding (CSS style), as one integer (TPLR),
              or a pair of integers (TB, LR), or 3 integers (T, LR, B),
              or 4 integers (T, R, B, L).
              Defaults to 0.
            no_wrap: Disable wrapping entirely for this layout.
            expand: Whether to expand the table to fit the available space.
        """
        super().__init__()
        self._grid = Table.grid(
            padding=padding,
            expand=expand,
        )

        for _ in range(0, ncols):
            self._grid.add_column(no_wrap=no_wrap)

    @property
    def renderable(self) -> RenderableType:
        """Rich Grid.

        Overrides View.layout().
        """
        return self._grid

    def add_row(self, *views: Optional[RenderableType]) -> "GridLayout":
        """Add a row to this grid layout.

        Args:
            views: The row's render-able columns.

        Returns:
            Self to allow chaining.
        """
        if len(self._grid.columns) != len(views):
            raise ValueError(
                f"Expected {len(self._grid.columns)} views, got {len(views)}"
            )
        self._grid.add_row(*views)
        return self


class TableLayout(View):
    """Base for table layouts.

    This is a simple vertical grid layout with optional header row.
    """

    _table: Table

    def __init__(
        self,
        headers: Sequence[str],
        /,
        *,
        padding: PaddingDimensions = 0,
        show_header: bool = True,
        no_wrap: bool = True,
        expand: bool = False,
    ) -> None:
        """Initialize layout.

        Args:
            headers: The table column headers.
            padding: Cells padding (CSS style), as one integer (TPLR),
              or a pair of integers (TB, LR), or 3 integers (T, LR, B),
              or 4 integers (T, R, B, L). Defaults to 0.
            show_header: Whether to show the table's header row.
            no_wrap: Disable wrapping entirely for this layout.
            expand: Whether to expand the table to fit the available space.
        """
        super().__init__()
        self._table = Table(
            box=rich.box.SIMPLE_HEAD,
            padding=padding,
            collapse_padding=True,
            pad_edge=False,
            show_edge=False,
            show_header=show_header,
            header_style=DTShTheme.STYLE_LIST_HEADER,
            expand=expand,
        )
        for header in headers:
            self._table.add_column(header, no_wrap=no_wrap)

    @property
    def renderable(self) -> RenderableType:
        """Rich Table.

        Overrides View.layout().
        """
        return self._table

    def add_row(self, *views: Optional[RenderableType]) -> None:
        """Add a row to this table layout.

        Args:
            views: The row's render-able columns.
        """
        if len(self._table.columns) != len(views):
            raise ValueError(
                f"Expected {len(self._table.columns)} views, got {len(views)}"
            )
        self._table.add_row(*views)


class StatusBar(GridLayout):
    """Status bar view.

    Single row expanded layout, with 3 equally sized columns (left, center,
    and right), suitable for top or bottom status bars.
    """

    def __init__(
        self,
        text_left: Optional[Union[str, Text]],
        text_center: Optional[Union[str, Text]],
        text_right: Optional[Union[str, Text]],
    ) -> None:
        """Initialize status bar.

        Args:
            text_left: Left side text content.
            text_center: Center text content.
            text_right: Right side text content.
        """
        super().__init__(3, expand=True)
        self._grid.columns[0].justify = "left"
        self._grid.columns[1].justify = "center"
        self._grid.columns[2].justify = "right"
        for col in self._grid.columns:
            col.ratio = 1
        self._grid.add_row(text_left, text_center, text_right)


class FormLayout(GridLayout):
    """Base view for forms.

    A form is a table with "Label: <content>" rows,
    where <content> may be any renderable type.
    """

    _placeholder: Optional[Union[str, Text]]
    _style: Optional[StyleType]
    _linktype: ActionableType

    def __init__(
        self,
        /,
        *,
        placeholder: Optional[Union[str, Text]] = None,
        style: Optional[StyleType] = None,
    ) -> None:
        """Initialize form.

        Args:
            placeholder: Placeholder for missing contents.
            style: Style to use for the form's labels.
              Defaults to configured preference.
            linktype: Link type to use for contents.
              Defaults to configured preference.
        """
        super().__init__(2, padding=(0, 1, 0, 0), no_wrap=False)
        self._placeholder = placeholder
        self._style = style or DTShTheme.STYLE_FORM_LABEL
        self._linktype = DTShConfig.getinstance().pref_form_actionable_type
        self._grid.columns[0].justify = "right"

    def add_content(
        self, label: str, content: Optional[Union[View, RenderableType]]
    ) -> None:
        """Add en entry to this form.

        Args:
            label: The entry's label.
            content: The entry's content as renderable.
              Empty contents are allowed and replaced by the form's placeholder.

        """
        self.add_row(
            self._mk_label(label),
            # Empty strings allow to override the placeholder.
            content if content is not None else self._placeholder,
        )

    def _mk_label(self, label: str) -> Text:
        return TextUtil.assemble(
            TextUtil.mk_text(label, self._style),
            TextUtil.mk_text(":"),
        )


class HeadingsContentWriter:
    """Helper for views with headings an contents.

    This permits to write contents one-by-one instead of building
    a single complete view before we print something to the console.
    """

    ContentType = Union[RenderableType, View]

    class Section:
        """Contents."""

        title: str
        level: int
        content: "HeadingsContentWriter.ContentType"

        def __init__(
            self,
            title: str,
            content: "HeadingsContentWriter.ContentType",
            level: int = 1,
        ) -> None:
            self.title = title
            self.level = level
            self.content = content

    # The TAB size in characters.
    _tab: int

    # Whether we need to insert a new line before we write a new heading.
    _newl: bool

    def __init__(self, tab: int = 4) -> None:
        self._tab = tab
        self._newl = False

    def write(
        self,
        title: str,
        level: int,
        content: "HeadingsContentWriter.ContentType",
        out: DTShOutput,
    ) -> None:
        """Write content.

        Args:
            title: Content's title.
            level: Headings level.
            content: Renderable content.
            out: Where to write the content.
        """
        if self._newl:
            out.write()
        else:
            # Once this heading is written,
            # we'll need to insert new lines between subsequent ones.
            self._newl = True

        i_left: int = self._tab + (self._tab // 2) * (level - 1)
        if isinstance(content, View):
            content.left_indent(i_left)
        else:
            content = Padding(content, (0, 0, 0, i_left))

        out.write(TextUtil.bold(title.upper()))
        out.write(content)

    def write_section(
        self,
        section: "HeadingsContentWriter.Section",
        out: DTShOutput,
    ) -> None:
        """Write section.

        Args:
            section: What to write.
            out: Where to write.
        """
        self.write(section.title, section.level, section.content, out)

    def write_sections(
        self,
        sections: Sequence["HeadingsContentWriter.Section"],
        out: DTShOutput,
    ) -> None:
        """Write sections.

        Args:
            sections: What to write.
            out: Where to write.
        """
        for section in sections:
            self.write_section(section, out)


class RenderableError(BaseException):
    """Base for something we can raise and that has a view.

    - allows view factories to prepare a custom fallback detailing
      an expected condition we couldn't describe with a simple error message
    - commands are responsible for i) writing the view to the output stream,
      ii) forward the error as a DTShCommandException with a simple message
    """

    _grid: GridLayout

    def __init__(self, title: str, label: str, cause: Exception) -> None:
        """Initialize the error view.

        Args:
            title: Heading text.
            label: Error context (e.g. file path)
            cause: The cause of the rendering error.
        """
        strerror = cause.strerror if isinstance(cause, OSError) else str(cause)
        super().__init__(strerror)
        self._grid = GridLayout()

        # Title.
        self._grid.add_row(TextUtil.italic(f"{title}:"))
        # Body.
        body = GridLayout()
        body.add_row(TextUtil.mk_text(f"{label}"))
        body.add_row(TextUtil.mk_warning(strerror))
        body.left_indent(2)
        self._grid.add_row(body)

    @property
    def view(self) -> View:
        """The error view."""
        return self._grid

    def warn_and_forward(
        self, cmd: DTShCommand, msg: str, out: DTShOutput
    ) -> None:
        """Warn user and forward this error.

        Convenience for:
        - exiting the pager if needed
        - warning the user with the error view
        - forwarding (raising) the rendering error as a command error

        Args:
            cmd: Failed command.
            msg: Command error message.
            out: Where to warn user of the rendering error.
        """
        if cmd.with_flag(DTShFlagPager):
            out.pager_exit()

        out.write(self.view)
        raise DTShCommandError(cmd, msg)
