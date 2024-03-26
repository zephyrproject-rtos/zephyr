# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell helpers.

Flags, arguments and parameters for DTSh commands.

Unit tests and examples: tests/test_dtsh_shellutils.py
"""


from typing import Any, Type, Optional, Sequence, Mapping, List

import re

from dtsh.model import DTNodeSorter, DTNodeCriterion
from dtsh.modelutils import (
    DTNodeSortByPathName,
    DTNodeSortByNodeName,
    DTNodeSortByUnitName,
    DTNodeSortByUnitAddr,
    DTNodeSortByDeviceLabel,
    DTNodeSortByNodeLabel,
    DTNodeSortByAlias,
    DTNodeSortByCompatible,
    DTNodeSortByBinding,
    DTNodeSortByVendor,
    DTNodeSortByBus,
    DTNodeSortByOnBus,
    DTNodeSortByDepOrdinal,
    DTNodeSortByIrqNumber,
    DTNodeSortByIrqPriority,
    DTNodeSortByRegSize,
    DTNodeSortByRegAddr,
    DTNodeSortByBindingDepth,
    # Text-based criteria.
    DTNodeTextCriterion,
    DTNodeWithPath,
    DTNodeWithStatus,
    DTNodeWithName,
    DTNodeWithUnitName,
    DTNodeWithCompatible,
    DTNodeWithBinding,
    DTNodeWithVendor,
    DTNodeWithDeviceLabel,
    DTNodeWithNodeLabel,
    DTNodeWithAlias,
    DTNodeWithChosen,
    DTNodeAlsoKnownAs,
    DTNodeWithBus,
    DTNodeWithOnBus,
    DTNodeWithDescription,
    # Integer-based criteria.
    DTNodeIntCriterion,
    DTNodeWithUnitAddr,
    DTNodeWithIrqPriority,
    DTNodeWithIrqNumber,
    DTNodeWithRegAddr,
    DTNodeWithRegSize,
    DTNodeWithBindingDepth,
)
from dtsh.rl import DTShReadline
from dtsh.autocomp import DTShAutocomp, RlStateEnum
from dtsh.shell import (
    DTSh,
    DTShCommand,
    DTShFlag,
    DTShArg,
    DTShParameter,
    DTShError,
    DTPathNotFoundError,
    DTShCommandError,
)


class DTShFlagReverse(DTShFlag):
    """Flag to reverse command output.

    If the command outputs a tree, reverse the children order when walking.
    If the command outputs a list, reverse this list.
    """

    BRIEF = "reverse command output"
    SHORTNAME = "r"


class DTShFlagEnabledOnly(DTShFlag):
    """Flag to filter out disabled nodes or stop at disabled branches."""

    BRIEF = "filter out disabled nodes or branches"
    LONGNAME = "enabled-only"


class DTShFlagPager(DTShFlag):
    """Flag to page command output."""

    BRIEF = "page command output"
    LONGNAME = "pager"


class DTShFlagRegex(DTShFlag):
    """Flag to enforce patterns are interpreted as Regular Expressions.

    If unset, a text search (with wild-card substitution) is assumed.
    """

    BRIEF = "patterns are regular expressions"
    SHORTNAME = "E"


class DTShFlagIgnoreCase(DTShFlag):
    """Flag to ignore case in patterns.

    If unset, case-insensitive patterns are assumed.
    """

    BRIEF = "ignore case"
    SHORTNAME = "i"


class DTShFlagCount(DTShFlag):
    """Print matches count."""

    BRIEF = "print matches count"
    LONGNAME = "count"


class DTShFlagTreeLike(DTShFlag):
    """Whether to list results in tree-like format."""

    BRIEF = "list results in tree-like format"
    SHORTNAME = "T"


class DTShFlagNoChildren(DTShFlag):
    """Whether to list branches themselves, not their contents."""

    BRIEF = "list nodes, not branch contents"
    SHORTNAME = "d"


class DTShFlagRecursive(DTShFlag):
    """Whether to list branches rescursively."""

    BRIEF = "list recursively"
    SHORTNAME = "R"


class DTShFlagLogicalOr(DTShFlag):
    """Whether to match any criterion instead of all."""

    BRIEF = "match any criterion instead of all"
    LONGNAME = "OR"


class DTShFlagLogicalNot(DTShFlag):
    """Whether to negate the criterion chain."""

    BRIEF = "negate the criterion chain"
    LONGNAME = "NOT"


class DTShArgFixedDepth(DTShArg):
    """Argument that limits the depth when walking the devicetree.

    The default value is 0, which means unlimited depth.
    """

    BRIEF = "limit devicetree depth"
    LONGNAME = "fixed-depth"

    # Argument state: depth parsed on the command line.
    _depth: Optional[int]

    def __init__(self) -> None:
        super().__init__(argname="depth")

    @property
    def depth(self) -> int:
        """The fixed depth value.

        Defaults to zero if unset.
        """
        if self._depth is not None:
            return self._depth
        return 0

    def reset(self) -> None:
        """Reset this argument to its default value (zero).

        Overrides DTShOption.reset().
        """
        super().reset()
        self._depth = None

    def parsed(self, value: Optional[str] = None) -> None:
        """Overrides DTShOption.parsed()."""
        super().parsed(value)
        if self._raw:
            try:
                depth = int(self._raw or "0")
            except ValueError as e:
                raise DTShError(
                    f"expects an integer value (got '{self._raw}')"
                ) from e

            if depth < 0:
                raise DTShError(f"expects a non negative value (got {depth})")

            self._depth = depth


class DTShArgCriterion(DTShArg):
    """Base for arguments that append a criterion to the chain."""

    def get_criterion(  # pylint: disable=useless-return
        self, **kwargs: Any
    ) -> Optional[DTNodeCriterion]:
        """Get the criterion set by this argument.

        Args:
            kwargs: Criterion-specific parameters as keyword arguments.

        Returns:
            The criterion set by the command line, if any.

        Raises:
            DTShError: Failed to initialize criterion, should eventually
              come up as a command usage error.
        """
        del kwargs
        return None


class DTShArgIntCriterion(DTShArgCriterion):
    """Base for arguments that append an integer-based (expression) criterion.

    Argument format: [OPERATOR] N [UNIT]

    OPERATOR: "<" | ">" | "<=" | ">=" | "=" | "!="
    N: the integer to compare with
    UNIT (case-insensitive):  "k" | "kb" | "m" | "mb"
    """

    # Match argument with <operator><integer><unit>.
    _re: re.Pattern[str] = re.compile(
        r"^(?P<operator>[<>=!]+)?[\s]*(?P<integer>[\da-fA-FxX]+)[\s]*(?P<unit>[kKbBmM]+)?$"
    )

    # Concrete criterion class.
    _criter_cls: Type[DTNodeIntCriterion]

    # Parsed criterion instance.
    _criterion: Optional[DTNodeIntCriterion]

    def __init__(self, criter_cls: Type[DTNodeIntCriterion]) -> None:
        """Initialize criterion.

        Args:
            criter_cls: The concrete type for this criterion.
        """
        super().__init__(argname="expr")
        self._criter_cls = criter_cls

    def parsed(self, value: Optional[str] = None) -> None:
        """Overrides DTShArg.parsed()."""
        super().parsed(value)
        if not self._raw:
            return

        if self._raw == "*":
            # Match any integer value.
            self._criterion = self._criter_cls(None, None)
        else:
            match = DTShArgIntCriterion._re.match(self._raw)
            if match:
                op_str = match.group("operator")
                if op_str:
                    # Optional spaces after operator.
                    op_str = op_str.rstrip()
                    try:
                        criter_op = DTNodeIntCriterion.OPERATORS[op_str]
                    except KeyError as e:
                        raise DTShError(f"invalid operator: '{op_str}'") from e
                else:
                    criter_op = None

                int_str = match.group("integer")
                try:
                    criter_int = int(int_str, base=0)
                except ValueError as e:
                    raise DTShError(f"not a number: '{int_str}'") from e

                unit_str = match.group("unit")
                if unit_str:
                    unit_str = unit_str.lower()
                    if unit_str in ("k", "kb"):
                        criter_int *= 1024
                    elif unit_str in ("m", "mb"):
                        criter_int *= 1024**2
                    else:
                        raise DTShError(f"not an SI unit: '{unit_str}'")

                self._criterion = self._criter_cls(criter_op, criter_int)
            else:
                raise DTShError(f"invalid integer expression: '{self._raw}'")

    def reset(self) -> None:
        """Reset this argument.

        Overrides DTShOption.reset().
        """
        super().reset()
        self._criterion = None

    def get_criterion(self, **kwargs: Any) -> Optional[DTNodeCriterion]:
        """Overrides DTShArgCriterion.get_criterion()."""
        return self._criterion


class DTShArgNodeWithUnitAddr(DTShArgIntCriterion):
    """Match unit address."""

    BRIEF = "match unit addresse"
    LONGNAME = "with-unit-addr"

    def __init__(self) -> None:
        super().__init__(DTNodeWithUnitAddr)


class DTShArgNodeWithIrqNumber(DTShArgIntCriterion):
    """Match IRQ number."""

    BRIEF = "match IRQ numbers"
    LONGNAME = "with-irq-number"

    def __init__(self) -> None:
        super().__init__(DTNodeWithIrqNumber)


class DTShArgNodeWithIrqPriority(DTShArgIntCriterion):
    """Match IRQ priority."""

    BRIEF = "match IRQ priorities"
    LONGNAME = "with-irq-priority"

    def __init__(self) -> None:
        super().__init__(DTNodeWithIrqPriority)


class DTShArgNodeWithRegAddr(DTShArgIntCriterion):
    """Match  register address."""

    BRIEF = "match register addresses"
    LONGNAME = "with-reg-addr"

    def __init__(self) -> None:
        super().__init__(DTNodeWithRegAddr)


class DTShArgNodeWithRegSize(DTShArgIntCriterion):
    """Match  register address."""

    BRIEF = "match register sizes"
    LONGNAME = "with-reg-size"

    def __init__(self) -> None:
        super().__init__(DTNodeWithRegSize)


class DTShArgNodeWithBindingDepth(DTShArgIntCriterion):
    """Match  child-binding depth."""

    BRIEF = "match child-binding depth"
    LONGNAME = "with-binding-depth"

    def __init__(self) -> None:
        super().__init__(DTNodeWithBindingDepth)


class DTShArgTextCriterion(DTShArgCriterion):
    """Base for arguments that append a text-based (pattern) criterion.

    See DTNodeTextCriterion.
    """

    # Concrete criterion class.
    _criter_cls: Type[DTNodeTextCriterion]

    def __init__(self, criter_cls: Type[DTNodeTextCriterion]) -> None:
        """Initialize criterion.

        Args:
            criter_cls: The concrete type for this criterion.
        """
        super().__init__(argname="pattern")
        self._criter_cls = criter_cls

    def get_criterion(self, **kwargs: Any) -> Optional[DTNodeCriterion]:
        """Overrides DTShArgCriterion.get_criterion()."""
        if self._raw:
            try:
                return self._criter_cls(
                    self._raw,
                    re_strict=kwargs.get("re_strict", False),
                    ignore_case=kwargs.get("ignore_case", False),
                )
            except re.error as e:
                raise DTShError(f"invalid RE '{self._raw}': {e.msg}") from e
        return None


class DTShArgNodeWithPath(DTShArgTextCriterion):
    """Match path name."""

    BRIEF = "match path name"
    LONGNAME = "with-path"

    def __init__(self) -> None:
        super().__init__(DTNodeWithPath)


class DTShArgNodeWithStatus(DTShArgTextCriterion):
    """Match status string."""

    BRIEF = "match status string"
    LONGNAME = "with-status"

    def __init__(self) -> None:
        super().__init__(DTNodeWithStatus)


class DTShArgNodeWithName(DTShArgTextCriterion):
    """Match  unit name."""

    BRIEF = "match node name"
    LONGNAME = "with-name"

    def __init__(self) -> None:
        super().__init__(DTNodeWithName)


class DTShArgNodeWithUnitName(DTShArgTextCriterion):
    """Match  unit name."""

    BRIEF = "match unit name"
    LONGNAME = "with-unit-name"

    def __init__(self) -> None:
        super().__init__(DTNodeWithUnitName)


class DTShArgNodeWithCompatible(DTShArgTextCriterion):
    """Match compatible strings."""

    BRIEF = "match compatible strings"
    LONGNAME = "with-compatible"

    def __init__(self) -> None:
        super().__init__(DTNodeWithCompatible)

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Auto-complete with compatible strings.

        Overrides DTShArg.autocomp().
        """
        return DTShAutocomp.complete_dtcompat(txt, sh)


class DTShArgNodeWithBinding(DTShArgTextCriterion):
    """Match binding compatible or description."""

    BRIEF = "match binding's compatible or headline"
    LONGNAME = "with-binding"

    def __init__(self) -> None:
        super().__init__(DTNodeWithBinding)

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Auto-complete with compatible strings.

        Overrides DTShArg.autocomp().
        """
        return DTShAutocomp.complete_dtcompat(txt, sh)


class DTShArgNodeWithVendor(DTShArgTextCriterion):
    """Match vendor prefix or name."""

    BRIEF = "match vendor prefix or name"
    LONGNAME = "with-vendor"

    def __init__(self) -> None:
        super().__init__(DTNodeWithVendor)

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Auto-complete with vendor prefixes.

        Overrides DTShArg.autocomp().
        """
        return DTShAutocomp.complete_dtvendor(txt, sh)


class DTShArgNodeWithDescription(DTShArgTextCriterion):
    """Match description."""

    BRIEF = "grep binding's description"
    LONGNAME = "with-description"

    def __init__(self) -> None:
        super().__init__(DTNodeWithDescription)


class DTShArgNodeWithBus(DTShArgTextCriterion):
    """Match supported bus protocol."""

    BRIEF = "match supported bus protocols"
    LONGNAME = "with-bus"

    def __init__(self) -> None:
        super().__init__(DTNodeWithBus)

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Auto-complete with bus protocols.

        Overrides DTShArg.autocomp().
        """
        return DTShAutocomp.complete_dtbus(txt, sh)


class DTShArgNodeWithOnBus(DTShArgTextCriterion):
    """Match bus of appearance."""

    BRIEF = "match bus of appearance"
    LONGNAME = "on-bus"

    def __init__(self) -> None:
        super().__init__(DTNodeWithOnBus)

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Auto-complete with bus protocols.

        Overrides DTShArg.autocomp().
        """
        return DTShAutocomp.complete_dtbus(txt, sh)


class DTShArgNodeWithDeviceLabel(DTShArgTextCriterion):
    """Match device label."""

    BRIEF = "match device label"
    LONGNAME = "with-device-label"

    def __init__(self) -> None:
        super().__init__(DTNodeWithDeviceLabel)


class DTShArgNodeWithNodeLabel(DTShArgTextCriterion):
    """Match node labels."""

    BRIEF = "match node labels"
    LONGNAME = "with-label"

    def __init__(self) -> None:
        super().__init__(DTNodeWithNodeLabel)

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Auto-complete with node labels.

        Overrides DTShArg.autocomp().
        """
        if txt.startswith("&"):
            # We're completing labels, and labels can't start with "&".
            return []
        return DTShAutocomp.complete_dtlabel(txt, sh)


class DTShArgNodeWithAlias(DTShArgTextCriterion):
    """Match nodes with device label."""

    BRIEF = "match aliases"
    LONGNAME = "with-alias"

    def __init__(self) -> None:
        super().__init__(DTNodeWithAlias)

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Auto-complete with alias names.

        Overrides DTShArg.autocomp().
        """
        return DTShAutocomp.complete_dtalias(txt, sh)


class DTShArgNodeWithChosen(DTShArgTextCriterion):
    """Match chosen nodes."""

    BRIEF = "match chosen nodes"
    LONGNAME = "chosen-for"

    def __init__(self) -> None:
        super().__init__(DTNodeWithChosen)

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Auto-complete with parameter names.

        Overrides DTShArg.autocomp().
        """
        return DTShAutocomp.complete_dtchosen(txt, sh)


class DTShArgNodeAlsoKnownAs(DTShArgTextCriterion):
    """Match labels and aliases."""

    BRIEF = "match labels or aliases"
    LONGNAME = "also-known-as"

    def __init__(self) -> None:
        super().__init__(DTNodeAlsoKnownAs)


DTSH_ARG_NODE_CRITERIA: Sequence[DTShArgCriterion] = [
    # Text-based criteria.
    DTShArgNodeWithPath(),
    DTShArgNodeWithStatus(),
    DTShArgNodeWithName(),
    DTShArgNodeWithUnitName(),
    DTShArgNodeWithCompatible(),
    DTShArgNodeWithBinding(),
    DTShArgNodeWithVendor(),
    DTShArgNodeWithDescription(),
    DTShArgNodeWithBus(),
    DTShArgNodeWithOnBus(),
    DTShArgNodeWithDeviceLabel(),
    DTShArgNodeWithNodeLabel(),
    DTShArgNodeWithAlias(),
    DTShArgNodeWithChosen(),
    DTShArgNodeAlsoKnownAs(),
    # Integer-based criteria.
    DTShArgNodeWithUnitAddr(),
    DTShArgNodeWithIrqNumber(),
    DTShArgNodeWithIrqPriority(),
    DTShArgNodeWithRegAddr(),
    DTShArgNodeWithRegSize(),
    DTShArgNodeWithBindingDepth(),
]
"""Pre-defined criteria shell commands may use to match nodes."""


class DTShNodeOrderBy:
    """Node sorter definition to be used by shell commands.

    Associate user friendly meta-data to sorter implementations.
    """

    _key: str
    # Order by what (human readable form) ?
    _what: str

    # The sorter implementation.
    _sorter: DTNodeSorter

    def __init__(self, key: str, by_what: str, sorter: DTNodeSorter) -> None:
        """Initialize sorted definition.

        Args:
            key: A single character key to reference the sorter with.
            by_what: A human readable name for the node aspect this sorter
              relies on.
            sorter: The corresponding sorter implementation.
        """
        self._key = key
        self._what = by_what
        self._sorter = sorter

    @property
    def key(self) -> str:
        """A single character key to reference the sorter with."""
        return self._key

    @property
    def brief(self) -> str:
        """A brief description of the sorter."""
        return f"sort by {self._what}"

    @property
    def sorter(self) -> DTNodeSorter:
        """The sorter implementation."""
        return self._sorter

    def __eq__(self, other: object) -> bool:
        if isinstance(other, DTShNodeOrderBy):
            return self._key == other._key
        return False

    def __lt__(self, other: object) -> bool:
        if isinstance(other, DTShNodeOrderBy):
            return self._key < other._key
        raise TypeError(other)

    def __hash__(self) -> int:
        return hash(self._key)


DTSH_NODE_ORDER_BY: Mapping[str, DTShNodeOrderBy] = {
    order_by.key: order_by
    for order_by in [
        DTShNodeOrderBy("p", "node path", DTNodeSortByPathName()),
        DTShNodeOrderBy("N", "node name", DTNodeSortByNodeName()),
        DTShNodeOrderBy("n", "unit name", DTNodeSortByUnitName()),
        DTShNodeOrderBy("a", "unit address", DTNodeSortByUnitAddr()),
        DTShNodeOrderBy("c", "compatible strings", DTNodeSortByCompatible()),
        DTShNodeOrderBy("C", "binding", DTNodeSortByBinding()),
        DTShNodeOrderBy("v", "vendor name", DTNodeSortByVendor()),
        DTShNodeOrderBy("l", "device label", DTNodeSortByDeviceLabel()),
        DTShNodeOrderBy("L", "node labels", DTNodeSortByNodeLabel()),
        DTShNodeOrderBy("A", "aliases", DTNodeSortByAlias()),
        DTShNodeOrderBy("B", "supported bus protocols", DTNodeSortByBus()),
        DTShNodeOrderBy("b", "bus of appearance", DTNodeSortByOnBus()),
        DTShNodeOrderBy("o", "dependency ordinal", DTNodeSortByDepOrdinal()),
        DTShNodeOrderBy("i", "IRQ numbers", DTNodeSortByIrqNumber()),
        DTShNodeOrderBy("I", "IRQ priorities", DTNodeSortByIrqPriority()),
        DTShNodeOrderBy("r", "register addresses", DTNodeSortByRegAddr()),
        DTShNodeOrderBy("s", "register sizes", DTNodeSortByRegSize()),
        DTShNodeOrderBy("X", "child-binding depth", DTNodeSortByBindingDepth()),
    ]
}
"""Pre-defined order relationships shell commands may use to sort nodes."""


class DTShArgOrderBy(DTShArg):
    """Argument to set the nodes sorter.

    The relationship is selected with a key specifier.
    """

    BRIEF = "sort nodes or branches"
    LONGNAME = "order-by"

    # Argument state: sorter implementation, no default value.
    _sorter: Optional[DTNodeSorter]

    def __init__(self) -> None:
        super().__init__(argname="key")

    def reset(self) -> None:
        """Overrides DTShOption.reset()."""
        super().reset()
        self._sorter = None

    def parsed(self, value: Optional[str] = None) -> None:
        """Overrides DTShOption.parsed()."""
        super().parsed(value)
        if self._raw:
            try:
                self._sorter = DTSH_NODE_ORDER_BY[self._raw].sorter
            except KeyError as e:
                raise DTShError(f"invalid sort key: '{self._raw}'") from e

    @property
    def sorter(self) -> Optional[DTNodeSorter]:
        """The sorter argument parsed on the command line."""
        return self._sorter

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Auto-complete with the predefined sorter definitions.

        Overrides DTShArg.autocomp().
        """
        return sorted(
            [
                RlStateEnum(key, order_by.brief)
                for key, order_by in DTSH_NODE_ORDER_BY.items()
                if key.startswith(txt)
            ],
            key=lambda x: x.rlstr.lower(),
        )


class DTShParamDTPath(DTShParameter):
    """Devicetree path parameter.

    This parameter accepts an optional single value,
    and is not intended to support path expansion (globbing).
    """

    def __init__(self) -> None:
        super().__init__(
            name="path",
            multiplicity="?",
            brief="devicetree path",
        )

    @property
    def path(self) -> str:
        """The path parameter value set by the command line.

        If unset, will answer an empty string,
        the semantic of which will depend on the
        supporting command.
        """
        return self._raw[0] if self._raw else ""

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Auto-complete with Devicetree paths.

        Overrides DTShParameter.autocomp().
        """
        return DTShAutocomp.complete_dtpath(txt, sh)


class DTShParamDTPaths(DTShParameter):
    """Devicetree paths parameter.

    This parameter accepts any number of values,
    each representing a path expression that may expand (globbing)
    to several Devicetree paths.
    """

    def __init__(self) -> None:
        super().__init__(
            name="path",
            multiplicity="*",
            brief="devicetree path",
        )

    @property
    def paths(self) -> Sequence[str]:
        """The path parameter values set by the command line.

        If unset, will answer an single empty value,
        representing the current working branch.
        """
        return self._raw if self._raw else [""]

    def expand(self, cmd: DTShCommand, sh: DTSh) -> List[DTSh.PathExpansion]:
        """Expand this parameter.

        Args:
            cmd: The executing command.
            sh: The context shell.

        Returns:
            A list containing one path expansion per parameter value.
            For convenience, nodes are filtered if the command supports
            the "enabled only" flag.

        Raises:
            DTShCommandError: Path expansion failed (node not found).
        """
        enabled_only: bool = False
        try:
            enabled_only = cmd.with_flag(DTShFlagEnabledOnly)
        except KeyError:
            # Unsupported option, no filter.
            enabled_only = False

        try:
            return [
                sh.path_expansion(path, enabled_only)
                for path in self._raw or [""]
            ]
        except DTPathNotFoundError as e:
            raise DTShCommandError(cmd, e.msg) from e

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Auto-complete with Devicetree paths.

        Overrides DTShParameter.autocomp().
        """
        return DTShAutocomp.complete_dtpath(txt, sh)


class DTShParamAlias(DTShParameter):
    """Alias name parameter."""

    def __init__(self) -> None:
        super().__init__(
            name="name",
            multiplicity="?",
            brief="alias name",
        )

    @property
    def alias(self) -> str:
        """The parameter value set by the command line.

        If unset, will answer an empty string means (any/all aliased nodes).
        """
        return self._raw[0] if self._raw else ""

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Auto-complete parameter with alias names.

        Overrides DTShParameter.autocomp().
        """
        return DTShAutocomp.complete_dtalias(txt, sh)


class DTShParamChosen(DTShParameter):
    """Chosen name parameter."""

    def __init__(self) -> None:
        super().__init__(
            name="name",
            multiplicity="?",
            brief="chosen name",
        )

    @property
    def chosen(self) -> str:
        """The parameter value set by the command line.

        If unset, will answer an empty string means (any/all aliased nodes).
        """
        return self._raw[0] if self._raw else ""

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Auto-complete argument value with chosen names.

        Overrides DTShParameter.autocomp().
        """
        return DTShAutocomp.complete_dtchosen(txt, sh)
