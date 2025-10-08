# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors

import enum
from collections.abc import Iterable
from typing import NamedTuple

class DTError(Exception): ...

class Node:
    props: dict[str, Property]
    nodes: dict[str, Node]
    labels: list[str]
    parent: Node | None
    dt: DT
    def __init__(
        self, name: str, parent: Node | None, dt: DT, filename: str, lineno: int
    ) -> None: ...
    @property
    def name(self) -> str: ...
    @property
    def lineno(self) -> int: ...
    @property
    def filename(self) -> str: ...
    @property
    def unit_addr(self) -> str: ...
    @property
    def path(self) -> str: ...
    def node_iter(self) -> Iterable[Node]: ...

class Type(enum.IntEnum):
    EMPTY = 0
    BYTES = 1
    NUM = 2
    NUMS = 3
    STRING = 4
    STRINGS = 5
    PATH = 6
    PHANDLE = 7
    PHANDLES = 8
    PHANDLES_AND_NUMS = 9
    COMPOUND = 10

class _MarkerType(enum.IntEnum):
    PATH = 0
    PHANDLE = 1
    LABEL = 2
    UINT8 = 3
    UINT16 = 4
    UINT32 = 5
    UINT64 = 6
    STRING = 7

class Property:
    name: str
    filename: str
    lineno: int
    value: bytes
    labels: list[str]
    offset_labels: dict[str, int]
    node: Node
    def __init__(self, node: Node, name: str) -> None: ...
    @property
    def type(self) -> Type: ...
    def to_num(self, signed: bool = False) -> int: ...
    def to_nums(self, signed: bool = False) -> list[int]: ...
    def to_bytes(self) -> bytes: ...
    def to_string(self) -> str: ...
    def to_strings(self) -> list[str]: ...
    def to_node(self) -> Node: ...
    def to_nodes(self) -> list[Node]: ...
    def to_path(self) -> Node: ...

class _T(enum.IntEnum):
    INCLUDE = 1
    LINE = 2
    STRING = 3
    DTS_V1 = 4
    PLUGIN = 5
    MEMRESERVE = 6
    BITS = 7
    DEL_PROP = 8
    DEL_NODE = 9
    OMIT_IF_NO_REF = 10
    LABEL = 11
    CHAR_LITERAL = 12
    REF = 13
    INCBIN = 14
    SKIP = 15
    EOF = 16
    NUM = 17
    PROPNODENAME = 18
    MISC = 19
    BYTE = 20
    BAD = 21

class _FileStackElt(NamedTuple):
    filename: str
    lineno: int
    contents: str
    pos: int

class _Token(NamedTuple):
    id: int
    val: int | str

class DT:
    alias2node: dict[str, Node]
    label2node: dict[str, Node]
    label2prop: dict[str, Property]
    label2prop_offset: dict[str, tuple[Property, int]]
    phandle2node: dict[int, Node]
    memreserves: list[tuple[set[str], int, int]]
    filename: str | None
    def __init__(
        self, filename: str | None, include_path: Iterable[str] = (), force: bool = False
    ) -> None: ...
    @property
    def root(self) -> Node: ...
    def get_node(self, path: str) -> Node: ...
    def has_node(self, path: str) -> bool: ...
    def move_node(self, node: Node, new_path: str): ...
    def node_iter(self) -> Iterable[Node]: ...
    def __deepcopy__(self, memo): ...

def to_num(data: bytes, length: int | None = None, signed: bool = False) -> int: ...
def to_nums(data: bytes, length: int = 4, signed: bool = False) -> list[int]: ...
