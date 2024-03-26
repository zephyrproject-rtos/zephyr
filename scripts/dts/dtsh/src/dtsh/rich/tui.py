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
from rich.table import Table
from rich.text import Text

from dtsh.rich.theme import DTShTheme


class View:
    """View interface.

    DTSh user interface will print to kind of views:

    - rich API objects like Text
    - specialized views that implement this interface,
      typically named ViewXXX
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

    def add_row(self, *views: Optional[RenderableType]) -> None:
        """Add a row to this grid layout.

        Args:
            views: The row's render-able columns.
        """
        if len(self._grid.columns) != len(views):
            raise ValueError(
                f"Expected {len(self._grid.columns)} views, got {len(views)}"
            )
        self._grid.add_row(*views)


class TableLayout(View):
    """Base for table layouts.

    This is a simple vertical grid layout with optional header row.
    """

    _table: Table

    def __init__(
        self,
        headers: Sequence[str],
        /,
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
