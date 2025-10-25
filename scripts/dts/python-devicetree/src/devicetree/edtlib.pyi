# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors

from collections.abc import Callable as Callable
from collections.abc import Iterable
from dataclasses import dataclass
from typing import Any

from yaml import Loader

class Binding:
    path: str | None
    raw: dict
    child_binding: Binding | None
    prop2specs: dict[str, PropertySpec]
    specifier2cells: dict[str, list[str]]
    def __init__(
        self,
        path: str | None,
        fname2path: dict[str, str],
        raw: Any = None,
        require_compatible: bool = True,
        require_description: bool = True,
    ) -> None: ...
    @property
    def description(self) -> str | None: ...
    @property
    def compatible(self) -> str | None: ...
    @property
    def bus(self) -> None | str | list[str]: ...
    @property
    def buses(self) -> list[str]: ...
    @property
    def on_bus(self) -> str | None: ...

class PropertySpec:
    binding: Binding
    name: str
    def __init__(self, name: str, binding: Binding) -> None: ...
    @property
    def path(self) -> str | None: ...
    @property
    def type(self) -> str: ...
    @property
    def description(self) -> str | None: ...
    @property
    def enum(self) -> list | None: ...
    @property
    def enum_tokenizable(self) -> bool: ...
    @property
    def enum_upper_tokenizable(self) -> bool: ...
    @property
    def const(self) -> None | int | list[int] | str | list[str]: ...
    @property
    def default(self) -> None | int | list[int] | str | list[str]: ...
    @property
    def required(self) -> bool: ...
    @property
    def deprecated(self) -> bool: ...
    @property
    def specifier_space(self) -> str | None: ...

@dataclass
class Property:
    spec: PropertySpec
    val: (
        int
        | str
        | list[int]
        | list[str]
        | Node
        | list[Node]
        | list[ControllerAndData | None]
        | bytes
        | None
    )
    node: Node
    @property
    def name(self) -> str: ...
    @property
    def description(self) -> str | None: ...
    @property
    def type(self) -> str: ...
    @property
    def val_as_tokens(self) -> list[str]: ...
    @property
    def enum_indices(self) -> list[int] | None: ...
    def __init__(self, spec, val, node) -> None: ...

@dataclass
class Register:
    node: Node
    name: str | None
    addr: int | None
    size: int | None
    def __init__(self, node, name, addr, size) -> None: ...

@dataclass
class Range:
    node: Node
    child_bus_cells: int
    child_bus_addr: int | None
    parent_bus_cells: int
    parent_bus_addr: int | None
    length_cells: int
    length: int | None
    def __init__(
        self,
        node,
        child_bus_cells,
        child_bus_addr,
        parent_bus_cells,
        parent_bus_addr,
        length_cells,
        length,
    ) -> None: ...

@dataclass
class ControllerAndData:
    node: Node
    controller: Node
    data: dict
    name: str | None
    basename: str | None
    def __init__(self, node, controller, data, name, basename) -> None: ...

@dataclass
class PinCtrl:
    node: Node
    name: str | None
    conf_nodes: list[Node]
    @property
    def name_as_token(self): ...
    def __init__(self, node, name, conf_nodes) -> None: ...

class Node:
    edt: EDT
    dep_ordinal: int
    compats: list[str]
    ranges: list[Range]
    regs: list[Register]
    props: dict[str, Property]
    interrupts: list[ControllerAndData]
    pinctrls: list[PinCtrl]
    bus_node: Node | None
    hash: str
    def __init__(
        self, dt_node: Node, edt: EDT, support_fixed_partitions_on_any_bus: bool = True
    ) -> None: ...
    @property
    def name(self) -> str: ...
    @property
    def filename(self) -> str: ...
    @property
    def lineno(self) -> int: ...
    @property
    def unit_addr(self) -> int | None: ...
    @property
    def description(self) -> str | None: ...
    @property
    def path(self) -> str: ...
    @property
    def label(self) -> str | None: ...
    @property
    def labels(self) -> list[str]: ...
    @property
    def parent(self) -> Node | None: ...
    @property
    def children(self) -> dict[str, Node]: ...
    def child_index(self, node) -> int: ...
    @property
    def required_by(self) -> list[Node]: ...
    @property
    def depends_on(self) -> list[Node]: ...
    @property
    def status(self) -> str: ...
    @property
    def read_only(self) -> bool: ...
    @property
    def matching_compat(self) -> str | None: ...
    @property
    def binding_path(self) -> str | None: ...
    @property
    def aliases(self) -> list[str]: ...
    @property
    def buses(self) -> list[str]: ...
    @property
    def on_buses(self) -> list[str]: ...
    @property
    def flash_controller(self) -> Node: ...
    @property
    def spi_cs_gpio(self) -> ControllerAndData | None: ...
    @property
    def gpio_hogs(self) -> list[ControllerAndData]: ...
    @property
    def has_child_binding(self) -> bool: ...
    @property
    def is_pci_device(self) -> bool: ...

class EDT:
    nodes: list[Node]
    compat2nodes: dict[str, list[Node]]
    compat2okay: dict[str, list[Node]]
    compat2notokay: dict[str, list[Node]]
    compat2vendor: dict[str, str]
    compat2model: dict[str, str]
    label2node: dict[str, Node]
    dep_ord2node: dict[int, Node]
    dts_path: str
    bindings_dirs: list[str]
    def __init__(
        self,
        dts: str | None,
        bindings_dirs: list[str],
        warn_reg_unit_address_mismatch: bool = True,
        default_prop_types: bool = True,
        support_fixed_partitions_on_any_bus: bool = True,
        infer_binding_for_paths: Iterable[str] | None = None,
        vendor_prefixes: dict[str, str] | None = None,
        werror: bool = False,
    ) -> None: ...
    def get_node(self, path: str) -> Node: ...
    @property
    def chosen_nodes(self) -> dict[str, Node]: ...
    def chosen_node(self, name: str) -> Node | None: ...
    @property
    def dts_source(self) -> str: ...
    def __deepcopy__(self, memo) -> EDT: ...
    @property
    def scc_order(self) -> list[list[Node]]: ...

def bindings_from_paths(yaml_paths: list[str], ignore_errors: bool = False) -> list[Binding]: ...

class EDTError(Exception): ...

def load_vendor_prefixes_txt(vendor_prefixes: str) -> dict[str, str]: ...
def str_as_token(val: str) -> str: ...

class _BindingLoader(Loader): ...
