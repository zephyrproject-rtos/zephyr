# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Helpers for formatted command output.

Command options to configure formatted outputs.

Base command with boilerplate code to support formatted outputs.

Unit tests and examples: tests/test_dtsh_rich_shellutils.py
"""


from typing import Optional, Sequence, List, Mapping

import sys

from dtsh.config import DTShConfig
from dtsh.model import DTNode, DTNodeSorter
from dtsh.rl import DTShReadline
from dtsh.shell import (
    DTSh,
    DTShOption,
    DTShParameter,
    DTShFlag,
    DTShArg,
    DTShError,
    DTShCommand,
)
from dtsh.shellutils import DTShFlagReverse, DTShFlagEnabledOnly, DTShArgOrderBy
from dtsh.autocomp import RlStateEnum

from dtsh.rich.modelview import (
    SketchMV,
    NodeColumnMV,
    PathNameNodeMV,
    NodeNameNodeMV,
    UnitNameNodeMV,
    UnitAddrNodeMV,
    DepOrdinalNodeMV,
    DeviceLabelNodeMV,
    NodeLabelsNodeMV,
    CompatibleNodeMV,
    BindingNodeMV,
    BindingDepthNodeMV,
    DescriptionNodeMV,
    VendorNodeMV,
    StatusNodeMV,
    AliasesNodeMV,
    AlsoKnownAsNodeMV,
    OnBusNodeMV,
    BusesNodeMV,
    BusNodeMV,
    InterruptsNodeMV,
    RegistersNodeMV,
    RegisterRangesNodeMV,
    DepOnNodeMV,
    ReqByNodeMV,
)


_dtshconf: DTShConfig = DTShConfig.getinstance()


class DTShFlagLongList(DTShFlag):
    """Whether to use a long listing format."""

    BRIEF = "use a long listing format"
    SHORTNAME = "l"


class DTShNodeFmt:
    """Node output format.

    An output format is a list of columns specified by a format string,
    e.g. "pd" is a format string where specifiers "p" and "d" represent
    the "Path" and "Description" columns.
    """

    T = Sequence[NodeColumnMV]
    """A node output format is actually a list of columns (view factories)."""

    class Spec:
        """Output format specifier.

        Associate meta-data to view factories (columns).
        """

        _key: str
        _brief: str
        _col: NodeColumnMV

        def __init__(self, spec: str, brief: str, col: NodeColumnMV) -> None:
            """Define output format specifier.

            Args:
                spec: Format specifier (single character).
                header: Human readable name for the field.
                brief: Brief description.
                factory: Formatted view factory.
            """
            self._key = spec
            self._brief = brief
            self._col = col

        @property
        def key(self) -> str:
            """Format specifier key (single character)."""
            return self._key

        @property
        def brief(self) -> str:
            """Brief description."""
            return self._brief

        @property
        def col(self) -> NodeColumnMV:
            """View factory associated to this specifier."""
            return self._col

        def __eq__(self, other: object) -> bool:
            if isinstance(other, DTShNodeFmt.Spec):
                return self._key == other._key
            return False

        def __lt__(self, other: object) -> bool:
            if isinstance(other, DTShNodeFmt.Spec):
                return self._key < other._key
            raise TypeError(other)

        def __hash__(self) -> int:
            return hash(self._key)

    @staticmethod
    def parse(fmt: str) -> "DTShNodeFmt.T":
        """Parse format string into node output format.

        Args:
            fmt: A format string.

        Returns:
            The format columns (view factories).
        """
        try:
            return [DTSH_NODE_FMT_SPEC[spec].col for spec in fmt]
        except KeyError as e:
            raise DTShError(f"invalid format specifier: '{e}'") from e

    @staticmethod
    def is_valid(fmt: str) -> bool:
        """Validate format string.

        Args:
            fmt: A format string.

        Returns:
            False if the format string is invalid.
        """
        return all(spec in DTSH_NODE_FMT_SPEC for spec in fmt)


class DTShArgLongFmt(DTShArg):
    """Argument to set the output format."""

    BRIEF = "node output format"
    LONGNAME = "format"

    _fmt: Optional[DTShNodeFmt.T]

    def __init__(self) -> None:
        super().__init__(argname="fmt")

    @property
    def fmt(self) -> Optional[DTShNodeFmt.T]:
        """The parsed node output format."""
        return self._fmt

    def reset(self) -> None:
        """Overrides DTShOption.reset()."""
        super().reset()
        self._fmt = None

    def parsed(self, value: Optional[str] = None) -> None:
        """Overrides DTShArg.parsed()."""
        super().parsed(value)
        if self._raw:
            self._fmt = DTShNodeFmt.parse(self._raw)

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Overrides DTShArg.autocomp().

        Auto-complete argument with format specifiers.
        """
        if not DTShNodeFmt.is_valid(txt):
            # Don't complete when current format string is invalid.
            return []
        # Answer all available cells but those already set.
        return sorted(
            [
                RlStateEnum(key, spec.brief)
                for key, spec in DTSH_NODE_FMT_SPEC.items()
                if key not in txt
            ],
            key=lambda x: x.rlstr.lower(),
        )


class DTShCommandLongFmt(DTShCommand):
    """Base for commands that enumerate nodes with formatted output support.

    Provide options and boilerplate code for:
    - formatted output: the options DTShFlagLongList and DTShArgLongFmt
      are appended to command specific options, get_sketch() and get_longfmt()
      will help getting the appropriate formatted columns
    - sorting nodes: if the options DTShFlagReverse and DTShArgOrderBy
      are supported, they can be accessed as properties flag_reverse()
      and arg_sorter(), and sort() will sort nodes according to
      the command's supported options
    - filtering disabled nodes if DTShFlagEnabledOnly is supported
    """

    def __init__(
        self,
        name: str,
        brief: str,
        options: Optional[Sequence[DTShOption]],
        parameter: Optional[DTShParameter],
    ) -> None:
        """Initialize command.

        Args:
            name: The command's name.
            brief: Brief command description.
            options: The options supported by this command.
              Options DTShFlagLongList and DTShArgLongFmt are appended
              to the command specific options.
            parameter: The parameter expected by this command, if any.
        """
        super().__init__(name, brief, options, parameter)
        # Append flag and parameters related to long listing formats.
        self._options.append(DTShFlagLongList())
        self._options.append(DTShArgLongFmt())

    @property
    def has_longfmt(self) -> bool:
        """Whether formatted output is disabled by an option or preference."""
        return (
            _dtshconf.pref_always_longfmt
            or self.with_flag(DTShFlagLongList)
            or self.with_arg(DTShArgLongFmt).isset
        )

    @property
    def flag_reverse(self) -> bool:
        """Shortcut to the "reverse output" flag if supported."""
        try:
            return self.with_flag(DTShFlagReverse)
        except KeyError:
            # Option not supported.
            pass
        return False

    @property
    def arg_sorter(self) -> Optional[DTNodeSorter]:
        """Shortcut to the "order-by" argument's value if supported."""
        try:
            return self.with_arg(DTShArgOrderBy).sorter
        except KeyError:
            # Option not supported.
            pass
        return None

    @property
    def flag_enabled_only(self) -> bool:
        """Shortcut to the "ennabled only" flag if supported."""
        try:
            return self.with_flag(DTShFlagEnabledOnly)
        except KeyError:
            # Option not supported.
            pass
        return False

    def sort(self, nodes: Sequence[DTNode]) -> Sequence[DTNode]:
        """Sort nodes if the "order-by" argument is set
        and/or the "reverse output" flag if set.

        Args:
            nodes: The nodes to sort.

        Returns:
            The sorted nodes.
        """
        if self.arg_sorter:
            return self.arg_sorter.sort(nodes, reverse=self.flag_reverse)
        if self.flag_reverse:
            return list(reversed(nodes))
        return nodes

    def prune(self, nodes: Sequence[DTNode]) -> Sequence[DTNode]:
        """Filter out disabled nodes if the "ennabled only" flag is set.

        Args:
            nodes: The nodes to filter.

        Returns:
            The filtered nodes.
        """
        enabled_only = self.flag_enabled_only
        return [node for node in nodes if node.enabled or not enabled_only]

    def get_sketch(self, layout: "SketchMV.Layout") -> SketchMV:
        """Initialize a rendering context for formatted output.

        Args:
            layout: The rendering layout to configure.
              LIST_VIEW is promoted to LIST_MULTI if "pref_list_multi" is set.

        Returns:
            A rendering context.
        """
        if (layout == SketchMV.Layout.LIST_VIEW) and _dtshconf.pref_list_multi:
            # Allow multiple-line cells.
            layout = SketchMV.Layout.LIST_MULTI

        return SketchMV(layout, self.arg_sorter, self.flag_reverse)

    def get_longfmt(self, default_fmt: Optional[str] = None) -> DTShNodeFmt.T:
        """Get the node output format to use.

        Args:
            default_fmt: A default format string to use when none
              was parsed from the command line.
              Typically set by the rendering context.

        Returns:
            The formatted columns as a list of view factories.
        """
        cols = self.with_arg(DTShArgLongFmt).fmt
        if (not cols) and default_fmt:
            try:
                cols = DTShNodeFmt.parse(default_fmt)
            except DTShError as e:
                print(
                    f"Invalid preferred format: '{default_fmt}' ('{e}')",
                    file=sys.stderr,
                )

        return cols or [DTShNodeFmt.parse("p")[0]]


DTSH_NODE_FMT_SPEC: Mapping[str, DTShNodeFmt.Spec] = {
    spec.key: spec
    for spec in [
        DTShNodeFmt.Spec(
            "p", "path name", NodeColumnMV("Path", PathNameNodeMV)
        ),
        DTShNodeFmt.Spec(
            "N", "node name", NodeColumnMV("Name", NodeNameNodeMV)
        ),
        DTShNodeFmt.Spec(
            "n", "unit name", NodeColumnMV("Name", UnitNameNodeMV)
        ),
        DTShNodeFmt.Spec(
            "a", "unit address", NodeColumnMV("Address", UnitAddrNodeMV)
        ),
        DTShNodeFmt.Spec(
            "o", "dependency ordinal", NodeColumnMV("Ordinal", DepOrdinalNodeMV)
        ),
        DTShNodeFmt.Spec(
            "l", "device label", NodeColumnMV("Label", DeviceLabelNodeMV)
        ),
        DTShNodeFmt.Spec(
            "L", "DTS labels", NodeColumnMV("Labels", NodeLabelsNodeMV)
        ),
        DTShNodeFmt.Spec(
            "c",
            "compatible strings",
            NodeColumnMV("Compatible", CompatibleNodeMV),
        ),
        DTShNodeFmt.Spec(
            "C",
            "binding's compatible or headline",
            NodeColumnMV("Binding", BindingNodeMV),
        ),
        DTShNodeFmt.Spec(
            "X",
            "child-binding depth",
            NodeColumnMV("Binding Depth", BindingDepthNodeMV),
        ),
        DTShNodeFmt.Spec(
            "d", "description", NodeColumnMV("Description", DescriptionNodeMV)
        ),
        DTShNodeFmt.Spec(
            "v", "vendor name", NodeColumnMV("Vendor", VendorNodeMV)
        ),
        DTShNodeFmt.Spec(
            "s", "status string", NodeColumnMV("Status", StatusNodeMV)
        ),
        DTShNodeFmt.Spec(
            "A", "node aliases", NodeColumnMV("Aliases", AliasesNodeMV)
        ),
        DTShNodeFmt.Spec(
            "K",
            "all labels and aliases",
            NodeColumnMV("Also Known As", AlsoKnownAsNodeMV),
        ),
        DTShNodeFmt.Spec(
            "b", "bus of appearance", NodeColumnMV("On Bus", OnBusNodeMV)
        ),
        DTShNodeFmt.Spec(
            "B", "supported bus protocols", NodeColumnMV("Buses", BusesNodeMV)
        ),
        DTShNodeFmt.Spec(
            "Y", "bus information", NodeColumnMV("Bus", BusNodeMV)
        ),
        DTShNodeFmt.Spec(
            "i",
            "generated interrupts",
            NodeColumnMV("IRQs", InterruptsNodeMV),
        ),
        DTShNodeFmt.Spec(
            "r",
            "registers (base address and size)",
            NodeColumnMV("Registers", RegistersNodeMV),
        ),
        DTShNodeFmt.Spec(
            "R",
            "registers (address range)",
            NodeColumnMV("Registers", RegisterRangesNodeMV),
        ),
        DTShNodeFmt.Spec(
            "D", "node dependencies", NodeColumnMV("Depends-on", DepOnNodeMV)
        ),
        DTShNodeFmt.Spec(
            "T", "dependent nodes", NodeColumnMV("Required-by", ReqByNodeMV)
        ),
    ]
}
"""Map meta-data to node format specifiers and view factories."""
