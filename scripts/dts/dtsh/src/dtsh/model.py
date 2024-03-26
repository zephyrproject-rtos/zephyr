# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree model.

Rationale:

- factorize the dtsh interface with edtlib (aka python-devicetree)
- support the hierarchical file system metaphor at the model layer
- unified API for sorting and matching nodes
- provide model objects (nodes, bindings, etc) with identity, equality
  and a default order-by semantic
- write most dtsh unit tests at the model layer

Implementation notes:

- identity: permits to build sets (uniqueness) of model objects
  and filter out duplicates, should imply equality
- equality: testing equality for model objects of different types
  is allowed and answers false, because an orange is not an apple
- default order: "less than" typically means "appears first";
  comparing model objects of different types is an API violation because
  we can't sort oranges and apples without an additional heuristic,
  such as preferring oranges

Unit tests and examples: tests/test_dtsh_model.py
"""


from typing import (
    Any,
    Optional,
    Iterator,
    List,
    Set,
    Mapping,
    Dict,
    Tuple,
    Sequence,
)

import os
import posixpath
import sys

from devicetree import edtlib

from dtsh.dts import DTS, YAMLFile


class DTPath:
    """Devicetree paths for the hierarchical file-system metaphor.

    This API does not involve actual Devicetree nodes, only path strings.

    An absolute path is a path name,
    as defined in DTSpec 2.2.1 and 2.2.3..

    A path that does not start from the devicetree root ("/")
    is a relative path.

    API functions may:

    - work equally with absolute and relative path parameters
    - expect an additional "current working branch" parameter
      to interpret a relative path parameter
    - require an absolute path parameter, and fault if invoked
      with a relative path

    POSIX-like path references are supported:

    - "." represents the current working branch
    - ".." represents the parent of the current working branch;

    By convention, the Devicetree root is its own parent.
    """

    @staticmethod
    def split(path: str) -> List[str]:
        """Convert a DT path into a list of path segments.

        Each path segment is a node name:

            split('a') == ['a']
            split('a/b/c') == ['a', 'b', 'c']

        By convention, the DT root "/" always represent the first
        node name of a path name:

            split('/') == ['/']
            split('/x') == ['/', 'x']
            split('/x/y/z') == ['/', 'x', 'y', 'z']

        Any Trailing empty node name component is stripped:

            split('/a/') == ['/', "a"]
            split('') == []

        Args:
            path: A DT path.

        Returns:
            The list of the node names in path.
            The number of names in this list minus one
            represents the (relative) depth of the node at path.
        """
        if not path:
            return []
        splits = path.split("/")
        if not splits[0]:
            # path := /[<foobar>]
            # Substitute 1st empty split with "/".
            splits[0] = "/"
        if not splits[-1]:
            # path := <foobar>/
            # Remove trailing empty split.
            splits = splits[:-1]
        return splits

    @staticmethod
    def join(path: str, *paths: str) -> str:
        """Concatenate DT paths.

        Join the paths *intelligently*, as defined by posixpath.join().

        For example:

            join('/', 'a', 'b') == "/a/b"
            join('/a', 'b') == '/a/b'
            join('a/', 'b') == 'a/b'

        Paths are not normalized:

            join('a', '.') == 'a/.'
            join('a', 'b/') == 'a/b/'
            join('a', '') == 'a/'

        Joining an absolute path will reset the join chain:

            join('/a', 'b', '/x', 'y') == '/x/y'

        Args:
            path: A DT path.
            *paths: The path segments to concatenate to path.

        Returns:
            The joined path segments.
        """
        return posixpath.join(path, *paths)

    @staticmethod
    def normpath(path: str) -> str:
        """Normalize a DT path.

        Normalize a path by collapsing redundant or trailing separators
        and common path references ("." and ".."),
        as defined in posixpath.normpath().

        For example:

            normpath('/.') == '/'
            normpath('a/b/') == 'a/b'
            normpath('a//b') == 'a/b'
            normpath('a/foo/./../b') == 'a/b'

        The devicetree root is its own parent:

            normpath('/../a') == '/a'

        The normalized form of an empty path is a reference
        to the current working branch.

             normpath('') == '.'

        Note: a relative path that starts with a reference to a parent,
        e.g. '../a', cannot be normalized alone. See also DTPath.abspath().

        Args:
            path: A DT path.

        Returns:
            The normalized path.
        """
        return posixpath.normpath(path)

    @staticmethod
    def abspath(path: str, pwd: str = "/") -> str:
        """Absolutize and normalize a DT path.

        This is equivalent to normpath(join(pwd, path)).

        For example:

            abspath('') == '/'
            abspath('/') == '/'
            abspath('a/b') == '/a/b'
            abspath('/a/b', '/x') == '/a/b'
            abspath('a', '/foo') == '/foo/a'

        Args:
            path: A DT path.
            pwd: The DT path name of the current working branch.

        Returns:
            The normalized absolute writing of path.
        """
        DTPath.check_path_name(pwd)
        return DTPath.normpath(DTPath.join(pwd, path))

    @staticmethod
    def relpath(pathname: str, pwd: str = "/") -> str:
        """Get the relative DT path from one node to another.

        For example:

            relpath('/') == '.'
            relpath('/a') == 'a'
            relpath('/a', '/a') == '.'
            relpath('/a/b/c', '/a') == 'b/c'

        If going backward is necessary, ".." references are used:

            relpath('/foo/a/b/c', '/bar') == '../foo/a/b/c'

        Args:
            pathname: The DT path name of the final node.
            pwd: The DT path name of the initial node.

        Returns:
            The relative DT path from pwd to path.
        """
        DTPath.check_path_name(pathname)
        DTPath.check_path_name(pwd)
        return posixpath.relpath(pathname, pwd)

    @staticmethod
    def dirname(path: str) -> str:
        """Get the head of the node names in a DT path.

        Most often, dirname() semantic will match the parent node's path:

            dirname(node.path) == node.parent.path

        For example:

            dirname('/a') == '/'
            dirname('/a/b/c') == '/a/b'
            dirname('a/b') == 'a'

        The root node is its own parent:

            dirname('/') == '/'

        When the path does not contain any '/', dirname() always
        returns a reference to the *current working branch*:

            dirname('') == '.'
            dirname('a') == '.'
            dirname('.') == '.'
            dirname('..') == '.'

        A trailing '/' is interpreted as an empty trailing node name:

            dirname('/a/') == '/a'

        Args:
            path: A DT path.

        Returns:
            The absolute or relative head of the split path,
            or "." when path does not contain any "/".
        """
        # Note: the sematic here is NOT DtPath.split().
        head, _ = posixpath.split(path)
        return head or "."

    @staticmethod
    def basename(path: str) -> str:
        """Get the tail of the node names in a DT path.

        Most often, basename() semantic will match the node name:

            basename(node.path) == node.name

        For example:

            basename('/a') == 'a'
            basename('a') == 'a'
            basename('a/b') == 'b'

        A trailing '/' is interpreted as an empty trailing node name:

            basename('/') == ''
            basename('a/') == ''

        And by convention:

            basename('') == ''
            basename('.') == '.'
            basename('..') == '..'

        Args:
            path: A DT path.

        Returns:
            The tail of the split path,
            or an empty string when path ends with "/".
        """
        # Note: the sematic here is NOT DtPath.split().
        _, tail = posixpath.split(path)
        return tail

    @staticmethod
    def check_path_name(path: str) -> None:
        """Check path names.

        Will fault if:
        - path is not absolute (does not start with "/")
        - path contains path references ("." or "..")
        """
        if not path.startswith("/"):
            # Path names are absolute DT paths.
            raise ValueError(path)
        if "." in path:
            # Path names do not contain path references ("." or "..").
            raise ValueError(path)


class DTVendor:
    """Device or device class vendor.

    Identity, equality and default order relationship are based on
    the vendor prefix.

    See:
    - DTSpec 2.3.1. compatible
    - zephyr/dts/bindings/vendor-prefixes.txt
    """

    _prefix: str
    _name: str

    def __init__(self, prefix: str, name: str) -> None:
        """Initialize vendor.

        Args
            prefix: Vendor prefix, e.g. "nordic".
            name: Vendor name, e.g. "Nordic Semiconductor"
        """
        self._prefix = prefix
        self._name = name

    @property
    def prefix(self) -> str:
        """The vendor prefix.

        This prefix appears as the manufacturer component
        in compatible strings.
        """
        return self._prefix

    @property
    def name(self) -> str:
        """The vendor name."""
        return self._name

    def __eq__(self, other: object) -> bool:
        if isinstance(other, DTVendor):
            return self.prefix == other.prefix
        return False

    def __lt__(self, other: object) -> bool:
        if isinstance(other, DTVendor):
            return self.prefix < other.prefix
        raise TypeError(other)

    def __hash__(self) -> int:
        return hash(self.prefix)

    def __repr__(self) -> str:
        return self._prefix


class DTBinding:
    """Devicetree binding (DTSpec 4. Device Bindings).

    Bindings include:

    - bindings that specify a device or device class identified by
      a compatible string
    - child-bindings of those, which may or not be identified by
      their own compatible string
    - the base bindings recursively included by the above

    Bindings identity, equality and default order are based on:

    - the base name of the YAML file: bindings that originate from
      different YAML files are different and ordered by base name
    - a child-binding depth that permits to distinguish and compare
      bindings that originate from the same YAML file
    """

    # Peer edtlib binding (permits to avoid "if self.binding" statements
    # when accessing most properties).
    _edtbinding: edtlib.Binding

    # Child-binding depth.
    _cb_depth: int

    # YAML binding file.
    _yaml: YAMLFile

    # Nested child-binding this binding defines, if any.
    _child_binding: Optional["DTBinding"]

    # The parent model (used to resolve child binding).
    def __init__(
        self,
        edtbinding: edtlib.Binding,
        cb_depth: int,
        child_binding: Optional["DTBinding"],
    ) -> None:
        """Initialize binding.

        Args:
            edtbinding: Peer edtlib binding object.
            cb_depth: The child-binding depth.
            child_binding: Nested child-binding this binding defines, if any.
        """
        if not edtbinding.path:
            # DTModel hypothesis.
            raise ValueError(edtbinding)

        self._edtbinding = edtbinding
        self._cb_depth = cb_depth
        self._child_binding = child_binding
        # Lazy-initialized: won't read/parse YAML content until needed.
        self._yaml = YAMLFile(edtbinding.path)

    @property
    def path(self) -> str:
        """Absolute path to the YAML file defining the binding."""
        return self._yaml.path

    @property
    def compatible(self) -> Optional[str]:
        """Compatible string for the devices specified by this binding.

        None for child-bindings without compatible string,
        and for bindings that do not specify a device or device class.
        """
        return self._edtbinding.compatible

    @property
    def buses(self) -> Sequence[str]:
        """Bus protocols that the nodes specified by this binding should support.

        Empty list if this binding does not specify a bus node.
        """
        return self._edtbinding.buses

    @property
    def on_bus(self) -> Optional[str]:
        """The bus that the nodes specified by this binding should appear on.

        None if this binding does not expect a bus of appearance.
        """
        return self._edtbinding.on_bus

    @property
    def description(self) -> Optional[str]:
        """The description of this binding, if any."""
        return self._edtbinding.description

    @property
    def includes(self) -> Sequence[str]:
        """The bindings included by this binding file."""
        return self._yaml.includes

    @property
    def cb_depth(self) -> int:
        """Child-binding depth.

        Zero if this is not a child-binding.
        """
        return self._cb_depth

    @property
    def child_binding(self) -> Optional["DTBinding"]:
        """The nested child-binding this binding defines, if any."""
        return self._child_binding

    def get_headline(self) -> Optional[str]:
        """The headline of this binding description, if any."""
        desc = self._edtbinding.description
        if desc:
            return desc.lstrip().split("\n", 1)[0]
        return None

    def __eq__(self, other: object) -> bool:
        if isinstance(other, DTBinding):
            this_fname = os.path.basename(self.path)
            other_fname = os.path.basename(other.path)
            return (this_fname == other_fname) and (
                self.cb_depth == other.cb_depth
            )
        return False

    def __lt__(self, other: object) -> bool:
        if isinstance(other, DTBinding):
            this_fname = os.path.basename(self.path)
            other_fname = os.path.basename(other.path)
            if this_fname == other_fname:
                # Bindings that originate from the same YAML file
                # are ordered from parent to child-bindings.
                return self.cb_depth < other.cb_depth
            # Bindings that originate from different YAML files
            # are ordered by file name.
            return this_fname < other_fname
        raise TypeError(other)

    def __hash__(self) -> int:
        return hash((os.path.basename(self.path), self.cb_depth))

    def __repr__(self) -> str:
        return f"yaml:{os.path.basename(self.path)}, cb_depth:{self.cb_depth}"


class DTNodeInterrupt:
    """Interrupts a node may generate.

    See DTSpec 2.4.2. Properties for Interrupt Controllers.
    """

    _edtirq: edtlib.ControllerAndData
    _node: "DTNode"

    @classmethod
    def sort_by_number(
        cls, irqs: Sequence["DTNodeInterrupt"], reverse: bool
    ) -> List["DTNodeInterrupt"]:
        """Sort interrupts by IRQ number."""
        return sorted(irqs, key=lambda irq: irq.number, reverse=reverse)

    @classmethod
    def sort_by_priority(
        cls, irqs: Sequence["DTNodeInterrupt"], reverse: bool
    ) -> List["DTNodeInterrupt"]:
        """Sort interrupts by IRQ priority."""
        return sorted(
            irqs,
            key=lambda irq: irq.priority
            if irq.priority is not None
            else sys.maxsize,
            reverse=reverse,
        )

    def __init__(
        self, edtirq: edtlib.ControllerAndData, node: "DTNode"
    ) -> None:
        """Initialize interrupt.

        Args:
            edtirq: Peer edtlib IRQ object.
            node: The device node that may generate this IRQ.
        """
        self._edtirq = edtirq
        self._node = node

    @property
    def number(self) -> int:
        """The IRQ number."""
        return self._edtirq.data.get("irq", sys.maxsize)

    @property
    def priority(self) -> Optional[int]:
        """The IRQ priority.

        Although interrupts have a priority on most platforms,
        it's not actually true for all boards, e.g. the EPS32 SoC.
        """
        # NOTE[PR-edtlib]: ControllerAndData docstring may be misleading
        # about the "priority" and/or "level" data.
        return self._edtirq.data.get("priority")

    @property
    def name(self) -> Optional[str]:
        """The IRQ name."""
        return self._edtirq.name

    @property
    def emitter(self) -> "DTNode":
        """The device node that may generate this IRQ."""
        return self._node

    @property
    def controller(self) -> "DTNode":
        """The controller this interrupt gets sent to."""
        return self._node.dt[self._edtirq.controller.path]

    def __eq__(self, other: object) -> bool:
        """Interrupts equal when IRQ numbers, priorities equal."""
        if isinstance(other, DTNodeInterrupt):
            return (self.number == other.number) and (
                self.priority == other.priority
            )
        return False

    def __lt__(self, other: object) -> bool:
        """By default, interrupts are sorted by IRQ numbers, then priorities."""
        if isinstance(other, DTNodeInterrupt):
            if self.number == other.number:
                if (self.priority is not None) and (other.priority is not None):
                    return self.priority < other.priority
            return self.number < other.number
        raise TypeError(other)

    def __hash__(self) -> int:
        """Identity inlcludes IRQ number and priority, and emitter."""
        return hash((self.number, self.priority, self.emitter))

    def __repr__(self) -> str:
        return (
            f"IRQ_{self.number}, prio:{self.priority}, src:{self.emitter.path}"
        )


class DTNodeRegister:
    """Address of a node resource.

    A register describes the address of a resource
    within the address space defined by its parent bus.

    According to DTSpec 2.3.6 reg:

    - the reg property is composed of an arbitrary number of pairs
      of address and length
    - if the parent node specifies a value of 0 for #size-cells,
      the length field in the value of reg shall be omitted

    This API will then assume:

    - all node registers should have an address (will fallback to MAXINT
      rather than fault when unset)
    - an unspecified size represents a zero-size register (an address)

    A default order is defined, based on register addresses,
    which is meaningful only when sorting registers within a same parent bus.
    """

    _edtreg: edtlib.Register
    _addr: int

    @classmethod
    def sort_by_addr(
        cls, regs: Sequence["DTNodeRegister"], reverse: bool
    ) -> List["DTNodeRegister"]:
        """Sort registers by address."""
        return sorted(regs, key=lambda reg: reg.address, reverse=reverse)

    @classmethod
    def sort_by_size(
        cls, regs: Sequence["DTNodeRegister"], reverse: bool
    ) -> List["DTNodeRegister"]:
        """Sort registers by size."""
        return sorted(regs, key=lambda reg: reg.size, reverse=reverse)

    def __init__(self, edtreg: edtlib.Register) -> None:
        self._edtreg = edtreg
        if edtreg.addr is not None:
            self._addr = edtreg.addr
        else:
            # We assume node registers have an address.
            # Fallback to funny value (not 0).
            self._addr = sys.maxsize

    @property
    def address(self) -> int:
        """The address within the address space defined by the parent bus."""
        return self._addr

    @property
    def size(self) -> int:
        """The register size.

        Mostly meaningful for memory mapped IO.
        """
        return self._edtreg.size or 0

    @property
    def tail(self) -> int:
        """The last address accessible through this register.

        Mostly meaningful for memory mapped IO.
        """
        if self.size > 0:
            return self.address + self.size - 1
        return self.address

    @property
    def name(self) -> Optional[str]:
        """The register name, if any."""
        return self._edtreg.name

    def __lt__(self, other: object) -> bool:
        if isinstance(other, DTNodeRegister):
            return self.address < other.address
        raise TypeError(other)

    def __repr__(self) -> str:
        return f"addr:{hex(self.address)}, size:{hex(self.size)}"


class DTWalkable:
    """Virtual devicetree we can walk through.

    The simplest walk-able is a Devicetree branch,
    implemented by DTNode objects.

    DTWalkableComb permits to define a virtual devicetree as a root node
    and a set of selected leaves.
    """

    def walk(
        self,
        /,
        order_by: Optional["DTNodeSorter"] = None,
        reverse: bool = False,
        enabled_only: bool = False,
        fixed_depth: int = 0,
    ) -> Iterator["DTNode"]:
        """Walk through the virtual devicetree.

        Args:
            order_by: Children ordering while walking branches.
              None will preserve the DTS order.
            reverse: Whether to reverse children order.
              If set and no order_by is given, means reversed DTS order.
            enabled_only: Whether to stop at disabled branches.
            fixed_depth: The depth limit, defaults to 0,
              walking through to leaf nodes, according to enabled_only.

        Returns:
            An iterator yielding the nodes in order of traversal.
        """
        del order_by
        del reverse
        del enabled_only
        del fixed_depth
        yield from ()


class DTNode(DTWalkable):
    """Devicetree nodes.

    The nodes API has a double aspect:

    - node-oriented: properties, identity, equality,
      and default order-by relationship
    - branch-oriented: walk-able, search-able
    """

    # Peer edtlib node.
    _edtnode: edtlib.Node

    # The devicetree model this node belongs to.
    _dt: "DTModel"

    # Parent node.
    _parent: "DTNode"

    # Child nodes (DTS-order).
    _children: List["DTNode"]

    # The binding that specifies the node content, if any.
    _binding: Optional[DTBinding]

    def __init__(
        self,
        edtnode: edtlib.Node,
        model: "DTModel",
        parent: Optional["DTNode"],
    ) -> None:
        """Insert a node in a devicetree model.

        Nodes are first inserted childless at the location pointed to by
        the parent parameter, then children are appended while the model
        initialization process walks the devicetree in DTS order.

        Args:
            edtnode: The peer edtlib.Node node.
            model: The devicetree model this node is inserted into.
            parent: The parent node (point of insertion),
              or None when creating the model's root.
        """
        self._edtnode = edtnode
        self._dt = model
        # The devicetree root is its own parent.
        self._parent = parent or self
        # Nodes are first inserted childless.
        self._children = []
        # Device bindings are initialized on start-up.
        self._binding = self._dt.get_device_binding(self)

    @property
    def dt(self) -> "DTModel":
        """The devicetree model this node belongs to."""
        return self._dt

    @property
    def path(self) -> str:
        """The path name (DTSpec 2.2.3)."""
        return self._edtnode.path

    @property
    def name(self) -> str:
        """The node name (DTSpec 2.2.1)."""
        return self._edtnode.name

    @property
    def unit_name(self) -> str:
        """The unit-name component of the node name.

        The term unit-name is not widely used for the node-name component
        of node names: for an example, see in DTSpec 3.4. /memory node.
        """
        return self._edtnode.name.split("@")[0]

    @property
    def unit_addr(self) -> Optional[int]:
        """The (value of the) unit-address component of the node name.

        May be None if this node has no unit address.
        """
        # NOTE: edtlib.Node.unit_addr may answer a string in the (near) future.
        return self._edtnode.unit_addr

    @property
    def status(self) -> str:
        """The node's status string (DTSpec 2.3.4).

        While the devicetree specifications allows this property
        to have values "okay", "disabled", "reserved", "fail", and "fail-sss",
        only the values "okay" and "disabled" are currently relevant to Zephyr.
        """
        return self._edtnode.status

    @property
    def enabled(self) -> bool:
        """Whether this node is enabled, according to its status property.

        For backwards compatibility, the value "ok" is treated the same
        as "okay", but this usage is deprecated.
        """
        # edtlib.Node.status() has already substituted "ok" with "okay",
        # no need to test both values again.
        return self._edtnode.status == "okay"

    @property
    def aliases(self) -> Sequence[str]:
        """The names this node is aliased to.

        Retrieved from the "/aliases" node content (DTSpec 3.3).
        """
        return self._edtnode.aliases

    @property
    def chosen(self) -> List[str]:
        """The parameters this node is a chosen for.

        Retrieved from the "/chosen" node content (DTSpec 3.3).
        """
        return [
            chosen
            for chosen, node in self._dt.chosen_nodes.items()
            if node is self
        ]

    @property
    def labels(self) -> Sequence[str]:
        """The labels attached to the node in the DTS (DTSpec 6.2)."""
        return self._edtnode.labels

    @property
    def label(self) -> Optional[str]:
        """A human readable description of the node device (DTSpec 4.1.2.3)."""
        return self._edtnode.label

    @property
    def compatibles(self) -> Sequence[str]:
        """Compatible strings, from most specific to most general (DTSpec 3.3.1)."""
        return self._edtnode.compats

    @property
    def compatible(self) -> Optional[str]:
        """The compatible string value that actually specifies the node content.

        A few nodes have no compatible value.
        """
        return self._edtnode.matching_compat

    @property
    def vendor(self) -> Optional[DTVendor]:
        """The device vendor.

        This is the manufacturer for the compatible string
        that specifies the node content.
        """
        if self.compatible:
            return self._dt.get_vendor(self.compatible)

        if self.binding:
            # Search parent nodes for vendor up to child-binding depth.
            node = self.parent
            cb_depth = self.binding.cb_depth
            while cb_depth != 0:
                if node.vendor:
                    return node.vendor

                cb_depth -= 1
                node = node.parent

        return None

    @property
    def binding_path(self) -> Optional[str]:
        """Path to the binding file that specifies this node content."""
        return self._edtnode.binding_path

    @property
    def binding(self) -> Optional[DTBinding]:
        """The binding that specifies the node content, if any."""
        return self._binding

    @property
    def on_bus(self) -> Optional[str]:
        """The bus this node appears on, if any."""
        # NOTE[edtlib]:
        # What's the actual semantic of edtlib.Node.on_buses() ?
        # Doesn't the node appear on a single bus in a given devicetree ?
        return self.binding.on_bus if self.binding else None

    @property
    def on_bus_device(self) -> Optional["DTNode"]:
        """The bus controller if this node is connected to a bus."""
        if self._edtnode.bus_node:
            return self._dt[self._edtnode.bus_node.path]
        return None

    @property
    def buses(self) -> Sequence[str]:
        """List of supported protocols if this node is a bus device.

        Empty list if this node is not a bus node.
        """
        return self._edtnode.buses

    @property
    def interrupts(self) -> List[DTNodeInterrupt]:
        """The interrupts generated by the node."""
        return [
            DTNodeInterrupt(edtirq, self) for edtirq in self._edtnode.interrupts
        ]

    @property
    def registers(self) -> List[DTNodeRegister]:
        """Addresses of the device resources (DTSpec 2.3.6)."""
        return [DTNodeRegister(edtreg) for edtreg in self._edtnode.regs]

    @property
    def description(self) -> Optional[str]:
        """The node description retrieved from its binding, if any."""
        return self._edtnode.description

    @property
    def children(self) -> Sequence["DTNode"]:
        """The node children, in DTS-order."""
        return self._children

    @property
    def parent(self) -> "DTNode":
        """The parent node.

        By convention, the root node is its own parent.
        """
        return self._parent

    @property
    def dep_ordinal(self) -> int:
        """Dependency ordinal.

        Non-negative integer value such that the value for a node is less
        than the value for all nodes that depend on it.

        See edtlib.Node.dep_ordinal.
        """
        return self._edtnode.dep_ordinal

    @property
    def required_by(self) -> List["DTNode"]:
        """The nodes that directly depend on this device."""
        return [
            self._dt[edt_node.path] for edt_node in self._edtnode.required_by
        ]

    @property
    def depends_on(self) -> List["DTNode"]:
        """The nodes this device directly depends on."""
        return [
            self._dt[edt_node.path] for edt_node in self._edtnode.depends_on
        ]

    def get_child(self, name: str) -> "DTNode":
        """Retrieve a child node by name.

        The requested child MUST exist.

        Args:
            name: The child node name to search for.

        Returns:
            The requested child.
        """
        for node in self._children:
            if node.name == name:
                return node
        raise KeyError(name)

    def walk(
        self,
        /,
        order_by: Optional["DTNodeSorter"] = None,
        reverse: bool = False,
        enabled_only: bool = False,
        fixed_depth: int = 0,
    ) -> Iterator["DTNode"]:
        """Walk the devicetree branch under this node.

        Args:
            order_by: Children ordering while walking through branches.
              None will preserve the DTS order.
            reverse: Whether to reverse sort order.
              If set and no order_by is given, means reverse DTS-order.
            enabled_only: Whether to stop at disabled branches.
            fixed_depth: The depth limit.
              Defaults to 0, which means walking through to leaf nodes,
              according to enabled_only.

        Returns:
            An iterator yielding the nodes in order of traversal.
        """
        return self._walk(
            self,
            order_by=order_by,
            reverse=reverse,
            enabled_only=enabled_only,
            fixed_depth=fixed_depth,
        )

    def rwalk(self) -> Iterator["DTNode"]:
        """Walk the devicetree backward from this node through to the root node.

        Returns:
            An iterator yielding the nodes in order of traversal.
        """
        return self._rwalk(self)

    def find(
        self,
        criterion: "DTNodeCriterion",
        /,
        order_by: Optional["DTNodeSorter"] = None,
        reverse: bool = False,
        enabled_only: bool = False,
    ) -> List["DTNode"]:
        """Search the devicetree branch under this node.

        Args:
            criterion: The search criterion children must match.
            order_by: Sort matched nodes, None will preserve the DTS order.
            reverse: Whether to reverse sort order.
              If set and no order_by is given, means reverse DTS-order.
            enabled_only: Whether to stop at disabled branches.

        Returns:
            The list of matched nodes.
        """
        nodes: List[DTNode] = [
            node
            for node in self.walk(
                enabled_only=enabled_only,
                # We don't want children ordering here,
                # we'll sort results once finished.
                order_by=None,
            )
            if criterion.match(node)
        ]

        # Sort matched nodes.
        if order_by:
            nodes = order_by.sort(nodes, reverse=reverse)
        elif reverse:
            # Reverse DTS-order.
            nodes.reverse()
        return nodes

    def _walk(
        self,
        node: "DTNode",
        /,
        order_by: Optional["DTNodeSorter"],
        reverse: bool,
        enabled_only: bool,
        fixed_depth: int,
        at_depth: int = 0,
    ) -> Iterator["DTNode"]:
        if enabled_only and not node.enabled:
            # Abort early on disabled branches when enabled_only is set.
            return

        # Yield branch and increment depth.
        yield node
        if fixed_depth > 0:
            if at_depth == fixed_depth:
                return
            at_depth += 1

        # Filter and sort children.
        children = node.children
        if enabled_only:
            children = [child for child in children if child.enabled]
        if order_by:
            children = order_by.sort(children, reverse=reverse)
        elif reverse:
            children = list(reversed(children))

        for child in children:
            yield from self._walk(
                child,
                order_by=order_by,
                reverse=reverse,
                enabled_only=enabled_only,
                fixed_depth=fixed_depth,
                at_depth=at_depth,
            )

    def _rwalk(self, node: "DTNode") -> Iterator["DTNode"]:
        # Walk the subtree backward to the root node,
        # which is by convention its own parent.
        yield node
        if node.parent != node:
            yield from self._rwalk(node.parent)

    def __eq__(self, other: object) -> bool:
        if isinstance(other, DTNode):
            return other.path == self.path
        return False

    def __lt__(self, other: object) -> bool:
        if isinstance(other, DTNode):
            return self.path < other.path
        raise TypeError(other)

    def __hash__(self) -> int:
        return hash(self.path)

    def __repr__(self) -> str:
        return self.path


class DTModel:
    """Devicetree model.

    This is the main entry point dtsh will rely on to access
    a successfully loaded devicetree.

    It's a facade to an edtlib.EDT devicetree model,
    with extensions and helpers for implementing the Devicetree shell.
    """

    # The EDT model this is a facade of.
    _edt: edtlib.EDT

    # Devicetree root.
    _root: DTNode

    # Devicetree source definition.
    _dts: DTS

    # Map path names to nodes,
    # redundant with edtlib.EDT but "cheap".
    _nodes: Dict[str, DTNode]

    # Bindings identified by a compatible string:
    # (compatible string, bus of appearance) -> binding
    _compatible_bindings: Dict[Tuple[str, Optional[str]], DTBinding]

    # Bindings without compatible string.
    # (YAML file name, cb_depth) -> binding
    _compatless_bindings: Dict[Tuple[str, int], DTBinding]

    # YAML binding files included by actual device bindings.
    # YAML file name -> binding
    _base_bindings: Dict[str, DTBinding]

    # Device and device class vendors that appear in this model.
    # prefix (aka manufacturer) -> vendor
    _vendors: Dict[str, DTVendor]

    # See aliased_nodes().
    _aliased_nodes: Dict[str, DTNode]

    # See chosen_nodes().
    _chosen_nodes: Dict[str, DTNode]

    # See labeled_nodes().
    _labeled_nodes: Dict[str, DTNode]

    @staticmethod
    def get_cb_depth(node: DTNode) -> int:
        """Compute the child-binding depth for a node.

        This is a non negative integer:

        - initialized to zero
        - incremented while walking the devicetree backward
          until we reach a node whose binding does not have child-binding
        """
        cb_depth = 0
        edtparent = node._edtnode.parent  # pylint: disable=protected-access
        while edtparent:
            parent_binding = (
                edtparent._binding  # pylint: disable=protected-access
            )
            if parent_binding and parent_binding.child_binding:
                cb_depth += 1
                edtparent = edtparent.parent
            else:
                break
        return cb_depth

    @classmethod
    def create(
        cls,
        dts_path: str,
        binding_dirs: Optional[Sequence[str]] = None,
        vendors_file: Optional[str] = None,
    ) -> "DTModel":
        """Devicetree model factory.

        Args:
            dts_path: The DTS file path.
            binding_dirs: The DT bindings search path as a list of directories.
              These directories are not required to exist.
              If unset, a default search path is retrieved or worked out.
            vendors_file: Path to a file in vendor-prefixes.txt format.

        Returns:
            An initialized devicetree model.

        Raises:
            OSError: Failed to open the DTS file.
            EDTError: Failed to initialize the devicetree model.
        """
        return cls(DTS(dts_path, binding_dirs, vendors_file))

    def __init__(self, dts: DTS) -> None:
        """Initialize a Devicetree model.

        Args:
            dts: The devicetree source definition.

        Raises:
            OSError: Failed to open the DTS file.
            EDTError: Failed to parse the DTS file, missing bindings.
        """
        self._dts = dts

        # Vendors file support initialization.
        self._vendors = {}
        if self._dts.vendors_file:
            # Load vendor definitions, and get the prefixes mapping
            # to provide EDT() with.
            vendor_prefixes = self._load_vendors_file(self._dts.vendors_file)
        else:
            vendor_prefixes = None

        self._edt = edtlib.EDT(
            self._dts.path,
            # NOTE[PR-edtlib]: could EDT() accept a Sequence
            # as const-qualified argument ?
            list(self._dts.bindings_search_path),
            vendor_prefixes=vendor_prefixes,
        )

        # Lazy-initialized base bindings.
        self._base_bindings = {}
        # Mappings initialized on 1st access.
        self._labeled_nodes = {}
        self._aliased_nodes = {}
        self._chosen_nodes = {}

        # The root node is its own parent.
        self._root = DTNode(self._edt.get_node("/"), self, parent=None)

        # Lazy-initialization.
        self._compatible_bindings = {}
        self._compatless_bindings = {}

        # Walk the EDT model, recursively initializing peer nodes.
        self._nodes = {}
        self._init_dt(self._root)

    @property
    def dts(self) -> DTS:
        """The devicetree source definition."""
        return self._dts

    @property
    def root(self) -> DTNode:
        """The devicetree root node."""
        return self._root

    @property
    def size(self) -> int:
        """The number of nodes in this model (including the root node)."""
        return len(self._edt.nodes)

    @property
    def aliased_nodes(self) -> Mapping[str, DTNode]:
        """Aliases retrieved from the "/aliases" node."""
        if not self._aliased_nodes:
            self._init_aliased_nodes()
        return self._aliased_nodes

    @property
    def chosen_nodes(self) -> Mapping[str, DTNode]:
        """Chosen configurations retrieved from the "/chosen" node."""
        if not self._chosen_nodes:
            self._init_chosen_nodes()
        return self._chosen_nodes

    @property
    def labeled_nodes(self) -> Mapping[str, DTNode]:
        """Nodes that can be referenced with a devicetree label."""
        if not self._labeled_nodes:
            self._init_labeled_nodes()
        return self._labeled_nodes

    @property
    def bus_protocols(self) -> List[str]:
        """All bus protocols supported by this model."""
        buses: Set[str] = set()
        for node in self._nodes.values():
            buses.update(node.buses)
        return list(buses)

    @property
    def compatible_strings(self) -> List[str]:
        """All compatible strings that appear on some node in this model."""
        return list(self._edt.compat2nodes.keys())

    @property
    def vendors(self) -> List[DTVendor]:
        """Vendors for this model compatible strings.

        NOTE: Will return an empty list if this model contains
        compatible strings with undefined manufacturers.
        """
        if not self._vendors:
            return []

        try:
            return [
                self._vendors[prefix]
                for prefix in {
                    compat.split(",", 1)[0]
                    for compat in self.compatible_strings
                    if "," in compat
                }
            ]
        except KeyError as e:
            print(f"WARN: invalid manufacturer: {e}", file=sys.stderr)
        return []

    def get_device_binding(self, node: DTNode) -> Optional[DTBinding]:
        """Retrieve a device binding.

        Args:
            node: The device to get the bindings for.

        Returns:
            The device or device class binding,
            or None if the node does not represent an actual device,
            e.g. "/cpus".
        """
        edtnode, edtbinding = self._edtnode_edtbinding(node)
        if not edtbinding:
            return None

        compat = edtbinding.compatible
        if compat:
            bus = edtbinding.on_bus
            return self.get_compatible_binding(compat, bus)

        cb_depth = self._edtnode_cb_depth(edtnode)
        return self._get_compatless_binding(edtbinding, cb_depth)

    def get_compatible_binding(
        self, compat: str, bus: Optional[str] = None
    ) -> Optional[DTBinding]:
        """Access bindings identified by a compatible string.

        If the lookup fails for the requested bus of appearance,
        a binding is searched for the compatible string only.
        This permits client code to uniformly enumerate the node bindings,
        not all of which will relate to the bus the node may appear on:

            for compat in node.compatibles:
                binding = model.get_compatible_binding(node.compatible, node.on_bus)
                if binding:
                    # Do something if the binding

        Args:
            compat: A compatible string to search the defining binding of.
            bus: The bus that nodes with this binding should appear on, if any.

        Returns:
            The binding for compatible devices, or None if not found.
        """
        binding: Optional[DTBinding] = None

        binding = self._compatible_bindings.get((compat, bus))
        if not binding and bus:
            binding = self._compatible_bindings.get((compat, None))

        if not binding:
            edtbinding = self._edt_compat2binding(compat, bus)
            if edtbinding:
                cb_depth = self._edtbinding_cb_depth(edtbinding)
                binding = self._init_binding(edtbinding, cb_depth)

        return binding

    def get_base_binding(self, basename: str) -> DTBinding:
        """Retrieve a base binding by its file name.

        The requested binding file MUST exist.

        This API is a factory and a cache.

        Args:
            basename: The binding file name.

        Returns:
            The binding the file specifies.
        """
        if basename not in self._base_bindings:
            path = self.dts.yamlfs.find_path(basename)
            if path:
                self._base_bindings[basename] = self._load_binding_file(path)
            else:
                # Should not happen: all included YAML files have been
                # loaded by EDT at this point, failing now to retrieve
                # them again is worth a fault.
                pass
        return self._base_bindings[basename]

    def get_vendor(self, compat: str) -> Optional[DTVendor]:
        """Retrieve the vendor for a compatible string.

        Args:
            compat: The compatible string to search the vendor for.

        Returns:
            The device or device class vendor,
            or None if the compatible string has no vendor prefix.
        """
        if self._dts.vendors_file and ("," in compat):
            manufacturer, _ = compat.split(",", 1)
            try:
                return self._vendors[manufacturer]
            except KeyError:
                # All vendors that appear as manufacturer in compatible
                # strings should exit: don't fault but log this nonetheless.
                print(
                    f"WARN: unknown manufacturer: {manufacturer}",
                    file=sys.stderr,
                )
        return None

    def get_compatible_devices(self, compat: str) -> List[DTNode]:
        """Retrieve devices whose binding matches a given compatible string.

        Args:
            compat: The compatible string to match.

        Returns:
            The matched nodes.
        """
        return [
            self[edt_node.path]
            for edt_node in self._edt.compat2nodes[compat]
            if edt_node.matching_compat == compat
        ]

    def walk(
        self,
        /,
        order_by: Optional["DTNodeSorter"] = None,
        reverse: bool = False,
        enabled_only: bool = False,
        fixed_depth: int = 0,
    ) -> Iterator[DTNode]:
        """Walk the devicetree from the root node through to all leaves.

        Shortcut for root.walk().

        Args:
            order_by: Children ordering while walking branches.
              None will preserve the DTS-order.
            reverse: Whether to reverse walk order.
              If set and no order_by is given, means reverse DTS-order.
            enabled_only: Whether to stop at disabled branches.
            fixed_depth: The depth limit.
              Defaults to 0, which means walking through to leaf nodes,
              according to enabled_only.

        Returns:
            A generator yielding the devicetree nodes in order of traversal.
        """
        return self._root.walk(
            order_by=order_by,
            reverse=reverse,
            enabled_only=enabled_only,
            fixed_depth=fixed_depth,
        )

    def find(
        self,
        criterion: "DTNodeCriterion",
        /,
        order_by: Optional["DTNodeSorter"] = None,
        reverse: bool = False,
        enabled_only: bool = False,
    ) -> List[DTNode]:
        """Search the devicetree.

        Shortcut for root.find().

        Args:
            criterion: The search criterion nodes must match.
            order_by: Sort matched nodes, None will preserve the DTS-order.
            reverse: Whether to reverse found nodes order.
              If set and no order_by is given, means reverse DTS-order.
            enabled_only: Whether to stop at disabled branches.

        Returns:
            The list of matched nodes.
        """
        return self._root.find(
            criterion,
            order_by=order_by,
            reverse=reverse,
            enabled_only=enabled_only,
        )

    def __contains__(self, pathname: str) -> bool:
        return pathname in self._nodes

    def __getitem__(self, pathname: str) -> DTNode:
        return self._nodes[pathname]

    def __len__(self) -> int:
        return self.size

    def __iter__(self) -> Iterator[DTNode]:
        return self.walk()

    def _init_dt(self, branch: DTNode) -> None:
        self._nodes[branch.path] = branch
        # Append children in DTS order.
        for (
            edtchild
        ) in (
            branch._edtnode.children.values()  # pylint: disable=protected-access
        ):
            child = DTNode(edtchild, self, branch)
            self._nodes[child.path] = child
            branch._children.append(child)  # pylint: disable=protected-access
            self._init_dt(child)

    def _init_aliased_nodes(self) -> None:
        self._aliased_nodes.update(
            {
                alias: self[dtnode.path]
                # NOTE[PR-edtlib]: Could we have something like EDT.aliased_nodes ?
                for alias, dtnode in self._edt._dt.alias2node.items()  # pylint: disable=protected-access
            }
        )

    def _init_chosen_nodes(self) -> None:
        self._chosen_nodes.update(
            {
                chosen: self[edtnode.path]
                for chosen, edtnode in self._edt.chosen_nodes.items()
            }
        )

    def _init_labeled_nodes(self) -> None:
        for node, labels in [
            (node_, node_.labels) for node_ in self._nodes.values()
        ]:
            self._labeled_nodes.update({label: node for label in labels})

    def _get_compatless_binding(
        self, edtbinding: edtlib.Binding, cb_depth: int
    ) -> DTBinding:
        if not edtbinding.path:
            raise ValueError(edtbinding)

        basename = os.path.basename(edtbinding.path)
        if (basename, cb_depth) not in self._compatless_bindings:
            binding = self._init_binding(edtbinding, cb_depth)
            self._compatless_bindings[(basename, cb_depth)] = binding

        return self._compatless_bindings[(basename, cb_depth)]

    def _init_binding(
        self, edtbinding: edtlib.Binding, cb_depth: int
    ) -> DTBinding:
        if not edtbinding.path:
            raise ValueError(f"Binding file expected: {edtlib.Binding}")

        edtbinding_child = edtbinding.child_binding
        if edtbinding_child:
            child_binding = self._init_binding(edtbinding_child, cb_depth + 1)
        else:
            child_binding = None

        binding = DTBinding(edtbinding, cb_depth, child_binding)

        if edtbinding.compatible:
            compat = edtbinding.compatible
            bus = edtbinding.on_bus
            self._compatible_bindings[(compat, bus)] = binding
        else:
            basename = os.path.basename(edtbinding.path)
            self._compatless_bindings[(basename, cb_depth)] = binding

        return binding

    def _edt_compat2binding(
        self, compat: str, bus: Optional[str]
    ) -> Optional[edtlib.Binding]:
        edtbinding = (
            self._edt._compat2binding.get(  # pylint: disable=protected-access
                (compat, bus)
            )
        )
        if not edtbinding and bus:
            edtbinding = self._edt._compat2binding.get(  # pylint: disable=protected-access
                (compat, None)
            )
        return edtbinding

    def _edtbinding_cb_depth(self, edtbinding: edtlib.Binding) -> int:
        if not edtbinding.compatible:
            raise ValueError(edtbinding)

        # We're computing the child-binding depth of any node
        # whose compatible value and bus of appearance
        # match this binding.
        compat = edtbinding.compatible
        bus = edtbinding.on_bus

        for edtnode in self._edt.compat2nodes[compat]:
            binding = edtnode._binding  # pylint: disable=protected-access
            if binding and (binding.on_bus == bus):
                return self._edtnode_cb_depth(edtnode)

        # The binding does not appear in any compatible string in the model.
        raise ValueError(edtbinding)

    def _edtnode_cb_depth(self, edtnode: edtlib.Node) -> int:
        cb_depth = 0

        # Walk the devicetree backward until we're not specified
        # by a child-binding.
        parent = edtnode.parent
        while parent:
            binding = parent._binding  # pylint: disable=protected-access
            if binding and binding.child_binding:
                cb_depth += 1
                parent = parent.parent
            else:
                parent = None

        return cb_depth

    def _edtnode_edtbinding(
        self, node: DTNode
    ) -> Tuple[edtlib.Node, Optional[edtlib.Binding]]:
        return (
            node._edtnode,  # pylint: disable=protected-access
            node._edtnode._binding,  # pylint: disable=protected-access
        )

    def _load_binding_file(self, path: str) -> DTBinding:
        edtbinding = edtlib.Binding(
            path,
            # NOTE[PR-edtlib]: patch Binding ctor to accept a Mapping
            # as const-qualified argument.
            fname2path=dict(self._dts.yamlfs.name2path),
            require_compatible=False,
            require_description=False,
        )
        return DTBinding(edtbinding, 0, None)

    def _load_vendors_file(self, vendors_file: str) -> Dict[str, str]:
        vendor_prefixes = edtlib.load_vendor_prefixes_txt(vendors_file)
        self._vendors.update(
            {
                prefix: DTVendor(prefix, name)
                for prefix, name in vendor_prefixes.items()
            }
        )
        return vendor_prefixes


class DTNodeSorter:
    """Basis for order relationships applicable to nodes.

    This defines stateless adapters for the standard Python sorted() function
    that support input lists containing nodes for which the key function
    would return None values.

    This is a 3-stage sort:

    - split nodes into sortable and unsortable
    - sort those that can be sorted using
    - append the unsortable to the sorted (by default, nodes for which
      the key function would have no value appear last)

    This base implementation sort nodes into "natural order".
    """

    def split_sortable_unsortable(
        self, nodes: Sequence[DTNode]
    ) -> Tuple[List[DTNode], List[DTNode]]:
        """Split nodes into sortable and unsortable.

        Returns:
            The tuple of (sortable, unsortable) nodes.
            This base implementation returns all nodes.
        """
        return (list(nodes), [])

    def weight(self, node: DTNode) -> Any:
        """The key function.

        Args:
            node: The node to get the weight of.

        Returns:
            The weight this sorter gives to the node.
            This base implementation returns the node itself ("natural order").
        """
        # TODO[python]: Where is SupportsLessThanT defined ?
        return node

    def sort(
        self, nodes: Sequence[DTNode], reverse: bool = False
    ) -> List[DTNode]:
        """Sort nodes.

        Args:
            nodes: The nodes to sort.
            reverse: Whether to reverse sort order.

        Returns:
            A list with the nodes sorted.
        """
        sortable, unsortable = self.split_sortable_unsortable(nodes)

        # We can't rely on Python sort(reverse=True) when multiple
        # items would pretend for the "same" position in the sorted
        # list (e.g. sorting nodes with the same unit-name by unit-name):
        # the Python sort() implementation is not granted to actually
        # reverse the items in the way we'd expect,
        # e.g. for the "-r" option that should always reverse
        # the command output order.
        sortable.sort(key=self.weight)
        if reverse:
            sortable.reverse()
            # Reverse order also applies to unsortable (sic).
            unsortable.reverse()

            # Unsortable first.
            unsortable.extend(sortable)
            return unsortable

        # Unsortable last.
        sortable.extend(unsortable)
        return sortable


class DTNodeCriterion:
    """Base criterion for searching or filtering DT nodes.

    This base criterion does not match with any node.
    """

    def match(self, node: DTNode) -> bool:
        """Match a node with this criterion.

        Args:
            node: The DT node to match.

        Returns:
            True if the node matches with this criterion.
        """
        del node  # Unused argument.
        return False


class DTNodeCriteria(DTNodeCriterion):
    """Chain of criterion for searching or filtering DT nodes.

    The criterion chain is evaluated either as:

    - a logical conjunction, and fails at the first unmatched criterion (default)
    - a logical disjunction, and succeeds at the first matched criterion

    By default, an empty chain will match any node.

    A logical negation may eventually be applied to the chain.
    """

    _criteria: List[DTNodeCriterion]
    _ored_chain: bool
    _negative_chain: bool

    def __init__(
        self,
        criterion_chain: Optional[List[DTNodeCriterion]] = None,
        ored_chain: bool = False,
        negative_chain: bool = False,
    ) -> None:
        """Initialize a new chain of criterion.

        Args:
            *criteria: Initial criterion chain.
            ored_chain: Whether this chain is a logical disjonction of
              criterion (default is logical conjunction).
            negative_chain: Whether to apply a logical negation to the chain.
        """
        self._criteria = criterion_chain if criterion_chain else []
        self._ored_chain = ored_chain
        self._negative_chain = negative_chain

    def append_criterion(self, criterion: DTNodeCriterion) -> None:
        """Append a criterion.

        Args:
            criterion: The criterion to append to this chain.
        """
        self._criteria.append(criterion)

    def match(self, node: DTNode) -> bool:
        """Does a node match this chain of criterion ?

        Args:
            node: The node to match.

        Returns:
            True if the node matches this criteria.
        """
        if self._ored_chain:
            chain_match = any(
                criterion.match(node) for criterion in self._criteria
            )
        else:
            chain_match = all(
                criterion.match(node) for criterion in self._criteria
            )

        return (not chain_match) if self._negative_chain else chain_match
