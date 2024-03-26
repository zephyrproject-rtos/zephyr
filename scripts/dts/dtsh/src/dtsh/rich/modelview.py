# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree model to views.

Stateless factories of base Devicetree model elements.

Context-aware views of DT nodes.
"""

from typing import (
    Type,
    Optional,
    Union,
    Iterable,
    Sequence,
    List,
    Generator,
    Mapping,
    Dict,
)

import enum

from rich import box
from rich.console import RenderableType
from rich.padding import PaddingDimensions
from rich.style import StyleType
from rich.text import Text
from rich.tree import Tree

from dtsh.model import (
    DTPath,
    DTWalkable,
    DTNode,
    DTNodeRegister,
    DTNodeInterrupt,
    DTNodeSorter,
)
from dtsh.modelutils import (
    DTNodeSortByNodeLabel,
    DTNodeSortByAlias,
    DTNodeSortByCompatible,
    DTNodeSortByRegSize,
    DTNodeSortByRegAddr,
    DTNodeSortByIrqNumber,
    DTNodeSortByIrqPriority,
    DTNodeSortByBus,
)
from dtsh.config import DTShConfig

from dtsh.rich.tui import View, TableLayout, GridLayout
from dtsh.rich.text import TextUtil
from dtsh.rich.theme import DTShTheme


_dtshconf: DTShConfig = DTShConfig.getinstance()


class DTModelView:
    """Stateless factory of base Devicetree model elements."""

    SI_UNITS: Mapping[int, str] = {
        0: "bytes",
        1: "kB",
        2: "MB",
        3: "GB",
    }
    SI_KB: int = 1024

    @classmethod
    def mk_path_name(cls, pathname: str) -> Text:
        """Text view factory for path names.

        Args:
            pathname: A DT path name.
        """
        DTPath.check_path_name(pathname)

        # Starts with "/" since pathname is absolute,
        # equals (at least) to "/" for both the root node
        # and its immediate children.
        dirname: str = DTPath.dirname(pathname)
        # The devicetree root basename will be empty.
        basename: str = DTPath.basename(pathname)

        tv_branch: Optional[Text]
        if basename:
            if dirname != "/":
                # Append the path separator to the branch path
                # if not the devicetree root.
                dirname += "/"
            tv_branch = TextUtil.mk_text(
                f"{dirname}", DTShTheme.STYLE_DT_PATH_BRANCH
            )
        else:
            # Empty base name (devicetree root): promote "/" to base name.
            basename = "/"
            # And skip the branch part.
            tv_branch = None

        tv_node = TextUtil.mk_text(basename, DTShTheme.STYLE_DT_PATH_NODE)
        if not tv_branch:
            return tv_node

        return TextUtil.assemble(tv_branch, tv_node)

    @classmethod
    def mk_path(cls, path: str) -> Text:
        """Generic text view factory for DT paths.

        Args:
            path: An absolute or relative DT path.
        """
        names = DTPath.split(path)
        branch_names: Sequence[str] = names[:-1]
        node_name: str = names[-1]

        if branch_names:
            if branch_names[0] == "/":
                branch = "/" + "/".join(branch_names[1:])
            else:
                branch = "/".join(branch_names)
            tv_branch = TextUtil.mk_text(
                f"{branch}/", DTShTheme.STYLE_DT_PATH_BRANCH
            )
        else:
            tv_branch = None

        tv_node = TextUtil.mk_text(node_name, DTShTheme.STYLE_DT_PATH_NODE)
        if not tv_branch:
            return tv_node

        return TextUtil.assemble(tv_branch, tv_node)

    @classmethod
    def mk_addr(cls, addr: int, style: Optional[StyleType] = None) -> Text:
        """Base text view factory for addresses.

        Address representation is an hexadecimal number,
        uppercase if "pref_hex_upper" is set.

        Args:
            addr: Address value.
            style: Text style.
        """
        straddr = hex(addr)
        if _dtshconf.pref_hex_upper:
            straddr = straddr.upper()
        return TextUtil.mk_text(straddr, style)

    @classmethod
    def mk_size(cls, size: int, style: Optional[StyleType] = None) -> Text:
        """Base text view factory for memory sizes.

        Size representation is either:

        - if "pref_sizes_si" is set, a floating point number,
          in SI units (bytes, kB, MB, GB)
        - or an hexadecimal number, uppercase if "pref_hex_upper" is set

        Args:
            size: Size in bytes.
            style: Text style.
        """
        # String representation of the size.
        strsize: str

        if _dtshconf.pref_sizes_si:
            # Retrieve appropriate SI unit.
            left: int = size
            pow_of_k = 0
            while left >= cls.SI_KB:
                left >>= 10
                pow_of_k += 1
            si_unit = cls.SI_UNITS[pow_of_k]
            si_unit_size = cls.SI_KB**pow_of_k

            # Format as integer or float depending on the remainder
            # once we've subtracted the size that can be written
            # as a multiple of the SI unit size.
            si_quotient: int = size >> (10 * pow_of_k)
            remainder: int = size - (si_quotient * si_unit_size)
            if remainder:
                si_size: float = si_quotient + (remainder / si_unit_size)
                strsize = f"{si_size} {si_unit}"
            else:
                strsize = f"{si_quotient} {si_unit}"
        else:
            # Show size as hex.
            strsize = hex(size)
            if _dtshconf.pref_hex_upper:
                strsize = strsize.upper()

        return TextUtil.mk_text(strsize, style)

    @classmethod
    def mk_node_name(cls, name: str) -> Text:
        """Text view factory for node names."""
        return TextUtil.mk_text(name, DTShTheme.STYLE_DT_NODE_NAME)

    @classmethod
    def mk_unit_name(cls, unit_name: str) -> Text:
        """Text view factory for unit names."""
        return TextUtil.mk_text(unit_name, DTShTheme.STYLE_DT_UNIT_NAME)

    @classmethod
    def mk_unit_addr(cls, unit_addr: int) -> Text:
        """Text view factory for unit addresses."""
        return cls.mk_addr(unit_addr, DTShTheme.STYLE_DT_UNIT_ADDR)

    @classmethod
    def mk_device_label(cls, label: str) -> Text:
        """Text view factory for "label" property values."""
        return TextUtil.mk_text(label, DTShTheme.STYLE_DT_DEVICE_LABEL)

    @classmethod
    def mk_dts_label(cls, label: str) -> Text:
        """Text view factory for DTS labels."""
        return TextUtil.mk_text(label, DTShTheme.STYLE_DT_NODE_LABEL)

    @classmethod
    def mk_compat_str(cls, compat: str) -> Text:
        """Text view factory for compatible strings."""
        return TextUtil.mk_text(compat, DTShTheme.STYLE_DT_COMPAT_STR)

    @classmethod
    def mk_binding_compat(cls, compat: str) -> Text:
        """Text view factory for compatible strings (from bindings)."""
        return TextUtil.mk_text(compat, DTShTheme.STYLE_DT_BINDING_COMPAT)

    @classmethod
    def mk_binding_headline(cls, desc: str) -> Text:
        """Text view factory for description headlines."""
        return TextUtil.mk_headline(desc, DTShTheme.STYLE_DT_BINDING_DESC)

    @classmethod
    def mk_binding_depth(cls, cb_depth: int) -> Text:
        """Text view factory child-binding depth."""
        return TextUtil.mk_text(
            str(cb_depth),
            DTShTheme.STYLE_DT_IS_CHILD_BINDING
            if (cb_depth > 0)
            else DTShTheme.STYLE_DT_CB_ORDER,
        )

    @classmethod
    def mk_vendor_name(cls, vendor: str) -> Text:
        """Text view factory vendors."""
        return TextUtil.mk_text(vendor, DTShTheme.STYLE_DT_VENDOR_NAME)

    @classmethod
    def mk_status(cls, status: str) -> Text:
        """Text view factory status strings."""
        return TextUtil.mk_text(
            status,
            DTShTheme.STYLE_DT_STATUS_ENABLED
            if status == "okay"
            else DTShTheme.STYLE_DT_STATUS_DISABLED,
        )

    @classmethod
    def mk_alias(cls, alias: str) -> Text:
        """Text view factory for alias names."""
        return TextUtil.mk_text(alias, DTShTheme.STYLE_DT_ALIAS)

    @classmethod
    def mk_bus(cls, bus: str) -> Text:
        """Text view factory for bus protocols."""
        return TextUtil.mk_text(bus, DTShTheme.STYLE_DT_BUS)

    @classmethod
    def mk_interrupt(cls, irq: DTNodeInterrupt) -> Text:
        """Text view factory for interrupts."""
        tv_irq = TextUtil.join(
            ":",
            (
                TextUtil.mk_text(
                    str(irq.number), DTShTheme.STYLE_DT_IRQ_NUMBER
                ),
                TextUtil.mk_text(
                    str(irq.priority), DTShTheme.STYLE_DT_IRQ_PRIORITY
                ),
            ),
        )
        if irq.name:
            tv_irq = TextUtil.join(
                " ", (tv_irq, TextUtil.mk_text(f"({irq.name})"))
            )

        return tv_irq

    @classmethod
    def mk_reg_addr(cls, addr: int) -> Text:
        """Text view factory for register addresses."""
        return cls.mk_addr(addr, DTShTheme.STYLE_DT_REG_ADDR)

    @classmethod
    def mk_reg_size(cls, size: int) -> Text:
        """Text view factory for register sizes."""
        return cls.mk_size(size, DTShTheme.STYLE_DT_REG_SIZE)

    @classmethod
    def mk_register(cls, reg: DTNodeRegister) -> Text:
        """Text view factory for registers (base address, size)."""
        tv_addr = cls.mk_reg_addr(reg.address)
        if reg.size:
            tv_size = TextUtil.assemble("(", cls.mk_reg_size(reg.size), ")")
            tv_reg = TextUtil.mk_text(" ").join((tv_addr, tv_size))
        else:
            tv_reg = tv_addr
        return tv_reg

    @classmethod
    def mk_register_range(cls, reg: DTNodeRegister) -> Text:
        """Text view factory for registers (address range)."""
        tv_reg = cls.mk_reg_addr(reg.address)
        if reg.size:
            tv_reg = TextUtil.join(
                f" {_dtshconf.wchar_arrow_right} ",
                (tv_reg, cls.mk_reg_addr(reg.tail)),
            )
        return tv_reg

    @classmethod
    def mk_depends_on(cls, depends_on: str, dep_failed: bool) -> Text:
        """Text view factory for node dependencies."""
        return TextUtil.mk_text(
            depends_on,
            DTShTheme.STYLE_DT_DEP_FAILED
            if dep_failed
            else DTShTheme.STYLE_DT_DEP_ON,
        )

    @classmethod
    def mk_requiredy_by(cls, req_by: str) -> Text:
        """Text view factory for dependent nodes."""
        return TextUtil.mk_text(req_by, DTShTheme.STYLE_DT_REQ_BY)


class SketchMV:
    """Rendering context for view factories."""

    class Layout(enum.Enum):
        """Rendering layout."""

        LIST_VIEW = 1
        """Rendering happends in the context of a list view.

        This is the widest rendering context.
        """

        TREE_VIEW = 2
        """Rendering happends in the context of a tree view.

        This is the rendering context for tree anchors.
        """

        TWO_SIDED = 3
        """Rendering happends in the context of a 2-sided view.

        A 2-sided view holds a tree (left) and a table (right)
        where each node in the tree matches a table row."""

        LIST_MULTI = 4
        """Rendering happends in the context of a list view.

        Same as LIST_VIEW but allows multiple-line cells.
        """

    _layout: "SketchMV.Layout"
    _reversed: bool
    _sorter: Optional[DTNodeSorter]

    def __init__(
        self,
        layout: "SketchMV.Layout",
        sorter: Optional[DTNodeSorter] = None,
        reverse: bool = False,
    ) -> None:
        """Initialize sketch.

        args:
            layout: Rendering layout.
            sorter: The order-by relationship applied to sort the nodes
              we're rendering.
            reverse: Whether to reverse output.
        """
        self._layout = layout
        self._sorter = sorter
        self._reversed = reverse

    @property
    def layout(self) -> "SketchMV.Layout":
        """Rendering layout."""
        return self._layout

    @property
    def default_fmt(self) -> str:
        """The preferred default format string in this context.

        Depends on the rendering layout and user preferences.
        """
        if self._layout == SketchMV.Layout.LIST_VIEW:
            fmt = _dtshconf.pref_list_fmt
        elif self._layout == SketchMV.Layout.TREE_VIEW:
            fmt = _dtshconf.pref_tree_fmt
        elif self._layout == SketchMV.Layout.TWO_SIDED:
            # NOTE: should we add a preference for this ?
            fmt = _dtshconf.pref_tree_fmt
        elif self._layout == SketchMV.Layout.LIST_MULTI:
            # NOTE: should we add a preference for this ?
            fmt = _dtshconf.pref_list_fmt
        else:
            raise ValueError(self._layout)
        return fmt

    def mk_placeholder(self) -> Optional[Text]:
        """Make a placeholder.

        The actual placeholder character depends on the rendering layout
        and user preferences.
        """
        placeholder: Optional[str] = None
        if self._layout == SketchMV.Layout.LIST_VIEW:
            placeholder = _dtshconf.pref_list_placeholder
        elif self._layout == SketchMV.Layout.TREE_VIEW:
            placeholder = _dtshconf.pref_tree_placeholder
        elif self._layout == SketchMV.Layout.TWO_SIDED:
            # NOTE: should we add a preference for this ?
            placeholder = _dtshconf.pref_tree_placeholder
        elif self._layout == SketchMV.Layout.LIST_MULTI:
            # NOTE: should we add a preference for this ?
            placeholder = _dtshconf.pref_list_placeholder
        else:
            raise ValueError(self._layout)

        return TextUtil().mk_text(placeholder) if placeholder else None

    def link(self, text: Union[str, Text], uri: str) -> Text:
        """Link text.

        The actual actionable view link depends on
        the rendering context and user preferences.

        Args:
            text: The text to link.
            uri: The link destination.

        Return:
            An actionable text.
        """
        if self._layout == SketchMV.Layout.LIST_VIEW:
            action_type = _dtshconf.pref_list_actionable_type
        elif self._layout == SketchMV.Layout.TREE_VIEW:
            action_type = _dtshconf.pref_tree_actionable_type
        elif self._layout == SketchMV.Layout.TWO_SIDED:
            action_type = _dtshconf.pref_2Sided_actionable_type
        elif self._layout == SketchMV.Layout.LIST_MULTI:
            # Use default.
            action_type = _dtshconf.pref_actionable_type
        else:
            raise ValueError(self._layout)
        return TextUtil.link(text, uri, action_type)

    def with_sorter(self, sorter_t: Type[DTNodeSorter]) -> bool:
        """Check if the rendering context includes a sorter.

        Args:
            sorter_t: The order-by relationship to test.

        Returns:
            True if we're rendering nodes sorted by this order-by relationship.
        """
        return isinstance(self._sorter, sorter_t)

    def with_reverse(self) -> bool:
        """Whether the rendering should reverse output."""
        return self._reversed


class NodeMV:
    """Stateless view factory for some node aspect.

    A node aspect is some information about the node that can be represented
    by a list of text views: this is the raw representation returned
    by mk_text().

    This representation is post-processed by mk_view() to factorize
    boilerplate code:

    - possibly create a placeholder when the aspect does not have a value
      (e.g. a node without unit address)
    - dim text views for disabled nodes
    - build the final view depending on the rendering layout
    """

    T = Type["NodeMV"]
    """Type to represent a concrete stateless factory."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Make the raw representation.

        Args:
            node: The node for which the rendering happens.
            sketch: The rendering context.

        Returns:
            A list of text views.
        """
        del node  # Unused in base class.
        del sketch  # Unused in base class.
        return []

    @classmethod
    def mk_view(
        cls, node: DTNode, sketch: SketchMV
    ) -> Optional[RenderableType]:
        """Make the view that represents this node aspect.

        Args:
            node: The node for which the rendering happens.
            sketch: The rendering context.

        returns:
            A view.
        """
        tvs = cls.mk_text(node, sketch)

        if not tvs:
            placeholder = sketch.mk_placeholder()
            if placeholder:
                tvs = [placeholder]
            else:
                # The aspect does not evaluate for the node, and no placeholder.
                return None

        if not node.enabled:
            for tv in tvs:
                TextUtil.disabled(tv)

        if len(tvs) == 1:
            # Single value: answer the text view, even if layout supports
            # multiple-line cells.
            return tvs[0]

        # Multiple value, we assume mk_text() was called with LIST_MULTI.
        view = GridLayout()
        for tv in tvs:
            view.add_row(tv)
        return view


class NodeColumnMV:
    """A node column is a view factory to be used in node tables."""

    _header: str
    _modelview: NodeMV.T

    def __init__(self, header: str, modelview: NodeMV.T) -> None:
        """Define column.

        Args:
            header: The column header.
            modelview: The view factory.
        """
        self._header = header
        self._modelview = modelview

    @property
    def header(self) -> str:
        """The column header."""
        return self._header

    @property
    def modelview(self) -> NodeMV.T:
        """The view factory."""
        return self._modelview

    def mk_view(
        self, node: DTNode, sketch: SketchMV
    ) -> Optional[RenderableType]:
        """Shortcut to call view factory.

        Args:
            node: The node for which the rendering happens.
            sketch: The rendering context.
        """
        return self._modelview.mk_view(node, sketch)


class ViewNodeTable(TableLayout):
    """Table view for DT nodes."""

    _cols: Sequence[NodeColumnMV]
    _sketch: SketchMV

    def __init__(
        self,
        cols: Sequence[NodeColumnMV],
        sketch: SketchMV,
        /,
        padding: PaddingDimensions = (0, 0, 0, 0),
        show_header: bool = True,
        no_wrap: bool = True,
        expand: bool = False,
    ) -> None:
        """Initialize table view.

        Args:
            cols: The table columns.
            sketch: Rendering context.
            padding: Cells padding (CSS style), as one integer (TPLR),
              or a pair of integers (TB, LR), or 3 integers (T, LR, B),
              or 4 integers (T, R, B, L). Defaults to 0.
            show_header: Whether to show the table's header row.
            no_wrap: Disable wrapping entirely for this layout.
            expand: Whether to expand the table to fit the available space.
        """
        super().__init__(
            [col.header for col in cols],
            padding=padding,
            show_header=show_header,
            no_wrap=no_wrap,
            expand=expand,
        )
        self._cols = cols
        self._sketch = sketch

    def append(self, node: DTNode) -> None:
        """Append a DT node to this table.

        NOTE: Won't check for duplicates.
        """
        cols: List[Optional[RenderableType]] = [
            col.mk_view(node, self._sketch) for col in self._cols
        ]
        self.add_row(*cols)

    def extend(self, nodes: Iterable[DTNode]) -> None:
        """Append DT nodes to this list.

        NOTE: Won't check for duplicates.
        """
        for node in nodes:
            self.append(node)


class ViewNodeList(ViewNodeTable):
    """List view for DT nodes."""

    def __init__(
        self,
        cols: Sequence[NodeColumnMV],
        sketch: SketchMV,
    ) -> None:
        """Initialize view.

        Args:
            cols: The table columns.
            sketch: Rendering context.
        """
        super().__init__(
            cols,
            sketch,
            padding=(0, 1, 0, 1),
            show_header=_dtshconf.pref_list_headers,
            no_wrap=True,
        )
        if self._sketch.layout == SketchMV.Layout.LIST_MULTI:
            # When we allow multiple-line cells, draw lines to distinguish rows.
            self._table.show_lines = True
            self._table.box = box.HORIZONTALS


class ViewDTWalkable(View):
    """Base view for DT walk-able."""

    _tree: Optional[Tree]
    _walkable: DTWalkable

    def __init__(self, walkable: DTWalkable) -> None:
        """New view.

        The view will remain an empty placeholder until walk_layout() is called.

        Args:
            walkable: The virtual devicetree to render.
        """
        super().__init__()
        self._walkable = walkable
        # Initialized on walk_layout().
        self._tree = None

    @property
    def renderable(self) -> RenderableType:
        """A rich Tree."""
        return self._tree or super().renderable

    def walk_layout(
        self,
        order_by: Optional[DTNodeSorter] = None,
        reverse: bool = False,
        enabled_only: bool = False,
        fixed_depth: int = 0,
    ) -> Generator[DTNode, None, None]:
        """Layout this tree in order of traversal.

        Derived classes may override this method to insert additional
        initialization before mk_anchor() and mk_label() are called.

        Args:
            order_by: Children ordering while walking branches.
              None will preserve the DTS order.
            reverse: Whether to reverse children order.
              If set and no order_by is given, means reversed DTS order.
            enabled_only: Whether to stop at disabled branches.
            fixed_depth: The depth limit, defaults to 0,
              walking through to leaf nodes, according to enabled_only.

        Yields:
            The added nodes in order of traversal.
        """
        # Map devicetree branches (nodes) to their Tree representation.
        branch2tree: Dict[DTNode, Tree] = {}

        walker = self._walkable.walk(
            order_by=order_by,
            reverse=reverse,
            enabled_only=enabled_only,
            fixed_depth=fixed_depth,
        )

        # Initialize tree view with the walk-able root.
        try:
            root: DTNode = next(walker)
        except StopIteration:
            # Empty walk-able.
            return
        self._tree = Tree(self.mk_anchor(root))
        branch2tree[root] = self._tree
        yield root

        for node in walker:
            anchor = branch2tree[node.parent]
            branch2tree[node] = anchor.add(self.mk_label(node))
            yield node

    def do_layout(
        self,
        order_by: Optional[DTNodeSorter] = None,
        reverse: bool = False,
        enabled_only: bool = False,
        fixed_depth: int = 0,
    ) -> None:
        """Same as walk_layout(), but consuming the generator."""
        for _ in self.walk_layout(order_by, reverse, enabled_only, fixed_depth):
            pass

    def mk_anchor(self, root: DTNode) -> RenderableType:
        """View factory for the Tree anchor (root).

        Derived classes may override this method.

        Args:
            root: The walk-able root.

        Returns:
            The Tree view's anchor (default to the node name).
        """
        return root.name

    def mk_label(self, node: DTNode) -> RenderableType:
        """View factory for the Tree labels (nodes).

        Derived classes may override this method.

        Args:
            node: The node to make the label of.

        Returns:
            The Tree label (default to the node name).
        """
        return node.name


class ViewNodeTreePOSIX(ViewDTWalkable):
    """POSIX-like tree view of a DT walk-able."""

    _path: str

    def __init__(
        self,
        path: str,
        walkable: DTWalkable,
    ) -> None:
        """New view.

        Args:
            path: The DT path expression to use as anchor.
              Like with the POSIX tree command,
              "." will be substituted for an empty path.
        """
        super().__init__(walkable)
        self._path = path or "."

    def mk_anchor(self, root: DTNode) -> RenderableType:
        """Overrides ViewDTWalkable.mk_anchor()."""
        return self._path


class ViewDTWalkableMV(ViewDTWalkable):
    """Tree view for DT walk-able with formatted node labels.

    Typically embedded in a parent layout.
    """

    # Initialized on walk_layout().
    _sketch: Optional[SketchMV]

    def __init__(
        self,
        walkable: DTWalkable,
        mv: NodeMV.T,
    ) -> None:
        """Initialize view.

        Args:
            walkable: The virtual devicetree to render.
            mv: The node format to render tree labels with.
        """
        super().__init__(walkable)
        self._mv = mv
        self._sketch = None

    def walk_layout(
        self,
        order_by: Optional[DTNodeSorter] = None,
        reverse: bool = False,
        enabled_only: bool = False,
        fixed_depth: int = 0,
    ) -> Generator[DTNode, None, None]:
        """Overrides ViewDTWalkable.walk_layout()."""
        self._sketch = SketchMV(SketchMV.Layout.TREE_VIEW, order_by, reverse)
        return super().walk_layout(order_by, reverse, enabled_only, fixed_depth)

    def mk_anchor(self, root: DTNode) -> RenderableType:
        """Overrides ViewDTWalkable.mk_anchor()."""
        return self.mk_label(root)

    def mk_label(self, node: DTNode) -> RenderableType:
        """Overrides ViewDTWalkable.mk_label()."""
        label = self._mv.mk_view(node, self._sketch) if self._sketch else None
        return label or View.SUB


class ViewNodeTwoSided(GridLayout):
    """Two-sided view for DT nodes."""

    _walkable: DTWalkable
    _cols: Sequence[NodeColumnMV]

    def __init__(
        self, walkable: DTWalkable, cols: Sequence[NodeColumnMV]
    ) -> None:
        """Initialize view.

        No rendering happens until do_layout() is called.

        Args:
            walkable: The virtual devicetree to render (left-side).
            cols: The table columns (right-side).
        """
        super().__init__(2)
        self._walkable = walkable
        self._cols = cols

    def do_layout(
        self,
        order_by: Optional[DTNodeSorter] = None,
        reverse: bool = False,
        enabled_only: bool = False,
        fixed_depth: int = 0,
    ) -> None:
        """Layout this view.

        Args:
            order_by: Children ordering while walking branches.
              None will preserve the DTS order.
            reverse: Whether to reverse children order.
              If set and no order_by is given, means reversed DTS order.
            enabled_only: Whether to stop at disabled branches.
            fixed_depth: The depth limit, defaults to 0,
              walking through to leaf nodes, according to enabled_only.
        """
        # Left side: tree view
        left_treeview = ViewDTWalkableMV(
            self._walkable, self._cols[0].modelview
        )
        if _dtshconf.pref_tree_headers and self._cols[1:]:
            # If a non empty right-side list-view shows its headers,
            # move the left-side tree-view two lines bellow.
            left_treeview.top_indent(2)

        # Right side: tree view
        right_listview = ViewNodeList(
            self._cols[1:],
            SketchMV(SketchMV.Layout.TWO_SIDED, order_by, reverse),
        )
        # Left-indented with two spaces relative to the tree at left-side.
        right_listview.left_indent(2)

        # Layout both sides in sync.
        for node in left_treeview.walk_layout(
            order_by, reverse, enabled_only, fixed_depth
        ):
            right_listview.append(node)

        self.add_row(left_treeview, right_listview)


class PathNameNodeMV(NodeMV):
    """View factory for node paths."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        return (DTModelView.mk_path_name(node.path),)


class NodeNameNodeMV(NodeMV):
    """View factory for node names."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        return (DTModelView.mk_node_name(node.name),)


class UnitNameNodeMV(NodeMV):
    """View factory for unit names."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        return (DTModelView.mk_unit_name(node.unit_name),)


class UnitAddrNodeMV(NodeMV):
    """View factory for unit names."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if node.unit_addr is None:
            return []
        return (DTModelView.mk_unit_addr(node.unit_addr),)


class DepOrdinalNodeMV(NodeMV):
    """View factory for dependency ordinals."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        return (
            TextUtil.mk_text(str(node.dep_ordinal), DTShTheme.STYLE_DT_ORDINAL),
        )


class DeviceLabelNodeMV(NodeMV):
    """View factory for device labels.

    Note: it seems the "label" property is now deprecated by Zephyr.
    """

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.label:
            return []
        return (DTModelView.mk_device_label(node.label),)


class NodeLabelsNodeMV(NodeMV):
    """View factory for DTS labels."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.labels:
            return []

        # In-cell sort.
        labels: Sequence[str]
        if sketch.with_sorter(DTNodeSortByNodeLabel):
            labels = sorted(node.labels, reverse=sketch.with_reverse())
        else:
            labels = node.labels

        tvs_labels = (DTModelView.mk_dts_label(label) for label in labels)

        if sketch.layout == SketchMV.Layout.LIST_MULTI:
            return list(tvs_labels)

        return (TextUtil.join(", ", tvs_labels),)


class CompatibleNodeMV(NodeMV):
    """View factory for "compatible" property values."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.compatibles:
            return []

        # In-cell sort.
        compats: Sequence[str]
        if sketch.with_sorter(DTNodeSortByCompatible):
            compats = sorted(node.compatibles, reverse=sketch.with_reverse())
        else:
            compats = node.compatibles

        tvs_compats: List[Text] = []
        for compat in compats:
            binding_path: Optional[str] = None
            if compat == node.compatible:
                txt_compat = DTModelView.mk_binding_compat(compat)
                binding_path = node.binding_path
            else:
                txt_compat = DTModelView.mk_compat_str(compat)
                binding = node.dt.get_compatible_binding(compat, node.on_bus)
                if binding:
                    binding_path = binding.path

            if binding_path:
                txt_compat = sketch.link(txt_compat, binding_path)

            tvs_compats.append(txt_compat)

        if sketch.layout == SketchMV.Layout.LIST_MULTI:
            return tvs_compats

        return (TextUtil.join(" ", tvs_compats),)


class BindingNodeMV(NodeMV):
    """View factory for "compatible" property values."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.binding:
            return []

        binding = node.binding
        if not (binding.compatible or binding.description):
            return []

        tv_binding: Optional[Text] = None
        if binding.compatible:
            tv_binding = DTModelView.mk_binding_compat(binding.compatible)
        elif binding.description:
            tv_binding = DTModelView.mk_binding_headline(binding.description)

        if not tv_binding:
            # Binding has no representation.
            return []

        # Child-bindings layout.
        cb_depth: int = binding.cb_depth
        # Should we anchor child-bindings to their parent node's binding ?
        cb_anchor: Optional[str] = _dtshconf.pref_tree_cb_anchor
        cb_anchored = (
            sketch.layout == SketchMV.Layout.TWO_SIDED
            and cb_depth
            and cb_anchor
        )

        if cb_anchored:
            spc_indent = 2 * (cb_depth - 1) * " "
            tv_cb = TextUtil.mk_text(
                f"{spc_indent}{cb_anchor} ",
                DTShTheme.STYLE_DT_CB_ANCHOR,
            )
            tv_binding = TextUtil.assemble(tv_cb, tv_binding)
        else:
            # Link only to top-level bindings,
            # child-bindings are defined by the same binding file.
            tv_binding = sketch.link(tv_binding, binding.path)

        return (tv_binding,)


class BindingDepthNodeMV(NodeMV):
    """View factory for child-binding depths."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.binding:
            return []
        tv_depth = DTModelView.mk_binding_depth(node.binding.cb_depth)
        tv_depth.justify = "center"
        return (tv_depth,)


class DescriptionNodeMV(NodeMV):
    """View factory for descriptions."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.description:
            return []

        tv_desc = DTModelView.mk_binding_headline(node.description)
        if node.binding_path:
            tv_desc = sketch.link(tv_desc, node.binding_path)

        return (tv_desc,)


class VendorNodeMV(NodeMV):
    """View factory for vendor names."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.vendor:
            return []
        return (DTModelView.mk_vendor_name(node.vendor.name),)


class StatusNodeMV(NodeMV):
    """View factory for status strings."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        return (DTModelView.mk_status(node.status),)


class AliasesNodeMV(NodeMV):
    """View factory for node aliases."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.aliases:
            return []

        # In-cell sort.
        aliases: Sequence[str]
        if sketch.with_sorter(DTNodeSortByAlias):
            aliases = sorted(node.aliases, reverse=sketch.with_reverse())
        else:
            aliases = node.aliases

        tvs_aliases = (DTModelView.mk_alias(alias) for alias in aliases)

        if sketch.layout == SketchMV.Layout.LIST_MULTI:
            return list(tvs_aliases)

        return (TextUtil.join(", ", tvs_aliases),)


class AlsoKnownAsNodeMV(NodeMV):
    """View factory for all labels aliases."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not (node.aliases or node.label or node.labels):
            return []

        tvs_aka = (
            *DeviceLabelNodeMV.mk_text(node, sketch),
            *NodeLabelsNodeMV.mk_text(node, sketch),
            *AliasesNodeMV.mk_text(node, sketch),
        )

        if sketch.layout == SketchMV.Layout.LIST_MULTI:
            return list(tvs_aka)

        return (TextUtil.join(", ", tvs_aka),)


class BusesNodeMV(NodeMV):
    """View factory for bus protocols."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.buses:
            return []

        # In-cell sort.
        buses: Sequence[str]
        if sketch.with_sorter(DTNodeSortByBus):
            buses = sorted(node.buses, reverse=sketch.with_reverse())
        else:
            buses = node.buses

        tvs_buses = (DTModelView.mk_bus(bus) for bus in buses)

        if sketch.layout == SketchMV.Layout.LIST_MULTI:
            return list(tvs_buses)

        return (TextUtil.join(" ", tvs_buses),)


class OnBusNodeMV(NodeMV):
    """View factory for buses of appearance."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.on_bus:
            return []
        return (DTModelView.mk_bus(node.on_bus),)


class BusNodeMV(NodeMV):
    """View factory for bus info."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        tv_businfo: Optional[Text] = None
        tvs_buses = BusesNodeMV.mk_text(node, sketch)
        tvs_on_bus = OnBusNodeMV.mk_text(node, sketch)

        if tvs_buses:
            tv_businfo = TextUtil.join(", ", tvs_buses)

        if tvs_on_bus:
            if tv_businfo:
                tv_businfo = TextUtil.join(" on ", (tv_businfo, tvs_on_bus[0]))
            else:
                tv_businfo = TextUtil.assemble("on ", tvs_on_bus[0])

        return (tv_businfo,) if tv_businfo else []


class InterruptsNodeMV(NodeMV):
    """View factory for generated interrupts."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.interrupts:
            return []

        # In-cell sort.
        irqs: Sequence[DTNodeInterrupt]
        if sketch.with_sorter(DTNodeSortByIrqNumber):
            irqs = DTNodeInterrupt.sort_by_number(
                node.interrupts, sketch.with_reverse()
            )
        elif sketch.with_sorter(DTNodeSortByIrqPriority):
            irqs = DTNodeInterrupt.sort_by_priority(
                node.interrupts, sketch.with_reverse()
            )
        else:
            irqs = node.interrupts

        tvs_irqs = (DTModelView.mk_interrupt(irq) for irq in irqs)

        if sketch.layout == SketchMV.Layout.LIST_MULTI:
            return list(tvs_irqs)

        return (TextUtil.join(", ", tvs_irqs),)


class RegistersNodeMV(NodeMV):
    """View factory for node registers (base address, size)."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.registers:
            return []

        # In-cell sort.
        regs: Sequence[DTNodeRegister]
        if sketch.with_sorter(DTNodeSortByRegAddr):
            regs = DTNodeRegister.sort_by_addr(
                node.registers, sketch.with_reverse()
            )
        elif sketch.with_sorter(DTNodeSortByRegSize):
            regs = DTNodeRegister.sort_by_size(
                node.registers, sketch.with_reverse()
            )
        else:
            regs = node.registers

        tvs_regs = (DTModelView.mk_register(reg) for reg in regs)

        if sketch.layout == SketchMV.Layout.LIST_MULTI:
            return list(tvs_regs)

        return (TextUtil.join(", ", tvs_regs),)


class RegisterRangesNodeMV(NodeMV):
    """View factory for node registers (address range)."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.registers:
            return []

        # In-cell sort.
        regs: Sequence[DTNodeRegister]
        if sketch.with_sorter(DTNodeSortByRegAddr):
            regs = DTNodeRegister.sort_by_addr(
                node.registers, sketch.with_reverse()
            )
        elif sketch.with_sorter(DTNodeSortByRegSize):
            regs = DTNodeRegister.sort_by_size(
                node.registers, sketch.with_reverse()
            )
        else:
            regs = node.registers

        tvs_regs = (DTModelView.mk_register_range(reg) for reg in regs)

        if sketch.layout == SketchMV.Layout.LIST_MULTI:
            return list(tvs_regs)

        return (TextUtil.join(", ", tvs_regs),)


class DepOnNodeMV(NodeMV):
    """View factory for depend-on."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.depends_on:
            return []

        tvs_dep_on: List[Text] = []
        for dep in node.depends_on:
            dep_failed = False
            if node.enabled:
                # Node is enabled, dependencies should be enabled.
                for parent in dep.rwalk():
                    if not parent.enabled:
                        dep_failed = True

            tvs_dep_on.append(DTModelView.mk_depends_on(dep.name, dep_failed))

        if sketch.layout == SketchMV.Layout.LIST_MULTI:
            return tvs_dep_on

        return (TextUtil.join(", ", tvs_dep_on),)


class ReqByNodeMV(NodeMV):
    """View factory for required-by."""

    @classmethod
    def mk_text(cls, node: DTNode, sketch: SketchMV) -> Sequence[Text]:
        """Overrides NodeMV.mk_text()."""
        if not node.required_by:
            return []

        tvs_req_by = (
            DTModelView.mk_requiredy_by(node.name) for node in node.required_by
        )
        # NOTE: if nodes are ordered, also order required_by ?

        if sketch.layout == SketchMV.Layout.LIST_MULTI:
            return list(tvs_req_by)

        return (TextUtil.join(", ", tvs_req_by),)


class ViewNodeAkaList(GridLayout):
    """Base view for Also Known As (e.g. aliases)."""

    def __init__(
        self,
        aka2node: Mapping[str, DTNode],
        cols: Sequence[NodeColumnMV],
        no_wrap: bool = True,
        expand: bool = False,
    ) -> None:
        """Initialize view.

        Args:
            aka2node: Map "Also Known As" to nodes.
            cols: The list view columns.
            no_wrap: Disable wrapping entirely for this layout.
            expand: Whether to expand the table to fit the available space.
        """
        super().__init__(
            2,
            padding=(0, 2, 0, 0),
            no_wrap=no_wrap,
            expand=expand,
        )

        grid_aka = GridLayout(2, padding=(0, 1, 0, 1))
        for name in aka2node:
            text = TextUtil.mk_text(name, DTShTheme.STYLE_DT_ALIAS)
            grid_aka.add_row(text, _dtshconf.wchar_arrow_right)

        listview = ViewNodeList(cols, SketchMV(SketchMV.Layout.LIST_VIEW))
        listview.extend(list(node for node in aka2node.values()))

        if _dtshconf.pref_list_headers:
            grid_aka.top_indent(2)

        self.add_row(grid_aka, listview)
