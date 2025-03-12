# Copyright (c) 2019, Nordic Semiconductor
# SPDX-License-Identifier: BSD-3-Clause

import collections
import enum
import errno
import os
import re
import string
import sys
import textwrap
from typing import Any, Iterable, NamedTuple, NoReturn, Optional, Union

class DTError(Exception):
    "Exception raised for devicetree-related errors"

class Node:
    def __init__(self, name: str, parent: Optional["Node"], dt: "DT", filename: str, lineno: int):
        self._name = name
        self._filename = filename
        self._lineno = lineno
        self.props: dict[str, Property] = {}
        self.nodes: dict[str, Node] = {}
        self.labels: list[str] = []
        self.parent = parent
        self.dt = dt
        self._omit_if_no_ref = False
        self._is_referenced = False

    @property
    def name(self) -> str:
        return self._name

    @property
    def lineno(self) -> int:
        return self._lineno

    @property
    def filename(self) -> str:
        return self._filename

    @property
    def unit_addr(self) -> str:
        return self.name.partition("@")[2]

    @property
    def path(self) -> str:
        node_names = []
        cur = self
        while cur.parent:
            node_names.append(cur.name)
            cur = cur.parent
        return "/" + "/".join(reversed(node_names))

    def node_iter(self) -> Iterable['Node']:
        yield self
        for node in self.nodes.values():
            yield from node.node_iter()

    def _get_prop(self, name: str) -> 'Property':
        prop = self.props.get(name)
        if not prop:
            prop = Property(self, name)
            self.props[name] = prop
        return prop

    def _del(self) -> None:
        if self.parent is None:
            self.nodes.clear()
            self.props.clear()
            return
        self.parent.nodes.pop(self.name)

    def __str__(self):
        s = "".join(label + ": " for label in self.labels)
        s += f"{self.name} {{\n"
        for prop in self.props.values():
            s += "\t" + str(prop) + "\n"
        for child in self.nodes.values():
            s += textwrap.indent(child.__str__(), "\t") + "\n"
        s += "};"
        return s

    def __repr__(self):
        return f"<Node {self.path} in '{self.dt.filename}'>"

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
    def __init__(self, node: Node, name: str):
        self.name = name
        self.filename = ""
        self.lineno = -1
        self.value = b""
        self.labels: list[str] = []
        self.offset_labels: dict[str, int] = {}
        self.node: Node = node
        self._label_offset_lst: list[tuple[str, int]] = []
        self._markers: list[list] = []

    @property
    def type(self) -> Type:
        types = [marker[1] for marker in self._markers if marker[1] != _MarkerType.LABEL]
        if not types:
            return Type.EMPTY
        if types == [_MarkerType.UINT8]:
            return Type.BYTES
        if types == [_MarkerType.UINT32]:
            return Type.NUM if len(self.value) == 4 else Type.NUMS
        if set(types) == {_MarkerType.UINT32}:
            return Type.NUMS
        if set(types) == {_MarkerType.STRING}:
            return Type.STRING if len(types) == 1 else Type.STRINGS
        if types == [_MarkerType.PATH]:
            return Type.PATH
        if (types == [_MarkerType.UINT32, _MarkerType.PHANDLE] and len(self.value) == 4):
            return Type.PHANDLE
        if set(types) == {_MarkerType.UINT32, _MarkerType.PHANDLE}:
            if len(self.value) == 4*types.count(_MarkerType.PHANDLE):
                return Type.PHANDLES
            return Type.PHANDLES_AND_NUMS
        return Type.COMPOUND

    def to_num(self, signed=False) -> int:
        if self.type is not Type.NUM:
            _err(f"expected property '{self.name}' on {self.node.path} in {self.node.dt.filename} to be assigned with '{self.name} = < (number) >;', not '{self}'")
        return int.from_bytes(self.value, "big", signed=signed)

    def to_nums(self, signed=False) -> list[int]:
        if self.type not in (Type.NUM, Type.NUMS):
            _err(f"expected property '{self.name}' on {self.node.path} in {self.node.dt.filename} to be assigned with '{self.name} = < (number) (number) ... >;', not '{self}'")
        return [int.from_bytes(self.value[i:i + 4], "big", signed=signed) for i in range(0, len(self.value), 4)]

    def to_bytes(self) -> bytes:
        if self.type is not Type.BYTES:
            _err(f"expected property '{self.name}' on {self.node.path} in {self.node.dt.filename} to be assigned with '{self.name} = [ (byte) (byte) ... ];', not '{self}'")
        return self.value

    def to_string(self) -> str:
        if self.type is not Type.STRING:
            _err(f"expected property '{self.name}' on {self.node.path} in {self.node.dt.filename} to be assigned with '{self.name} = \"string\";', not '{self}'")
        try:
            ret = self.value.decode("utf-8")[:-1]
        except UnicodeDecodeError:
            _err(f"value of property '{self.name}' ({self.value!r}) on {self.node.path} in {self.node.dt.filename} is not valid UTF-8")
        return ret

    def to_strings(self) -> list[str]:
        if self.type not in (Type.STRING, Type.STRINGS):
            _err(f"expected property '{self.name}' on {self.node.path} in {self.node.dt.filename} to be assigned with '{self.name} = \"string\", \"string\", ... ;', not '{self}'")
        try:
            ret = self.value.decode("utf-8").split("\0")[:-1]
        except UnicodeDecodeError:
            _err(f"value of property '{self.name}' ({self.value!r}) on {self.node.path} in {self.node.dt.filename} is not valid UTF-8")
        return ret

    def to_node(self) -> Node:
        if self.type is not Type.PHANDLE:
            _err(f"expected property '{self.name}' on {self.node.path} in {self.node.dt.filename} to be assigned with '{self.name} = < &foo >;', not '{self}'")
        return self.node.dt.phandle2node[int.from_bytes(self.value, "big")]

    def to_nodes(self) -> list[Node]:
        def type_ok():
            if self.type in (Type.PHANDLE, Type.PHANDLES):
                return True
            return self.type is Type.NUMS and not self.value
        if not type_ok():
            _err(f"expected property '{self.name}' on {self.node.path} in {self.node.dt.filename} to be assigned with '{self.name} = < &foo &bar ... >;', not '{self}'")
        return [self.node.dt.phandle2node[int.from_bytes(self.value[i:i + 4], "big")] for i in range(0, len(self.value), 4)]

    def to_path(self) -> Node:
        if self.type not in (Type.PATH, Type.STRING):
            _err(f"expected property '{self.name}' on {self.node.path} in {self.node.dt.filename} to be assigned with either '{self.name} = &foo' or '{self.name} = \"/path/to/node\"', not '{self}'")
        try:
            path = self.value.decode("utf-8")[:-1]
        except UnicodeDecodeError:
            _err(f"value of property '{self.name}' ({self.value!r}) on {self.node.path} in {self.node.dt.filename} is not valid UTF-8")
        try:
            ret = self.node.dt.get_node(path)
        except DTError:
            _err(f"property '{self.name}' on {self.node.path} in {self.node.dt.filename} points to the non-existent node \"{path}\"")
        return ret

    def __str__(self):
        s = "".join(label + ": " for label in self.labels) + self.name
        if not self.value:
            return s + ";"
        s += " ="
        for i, (pos, marker_type, ref) in enumerate(self._markers):
            if i < len(self._markers) - 1:
                next_marker = self._markers[i + 1]
            else:
                next_marker = None
            end = next_marker[0] if next_marker else len(self.value)
            if marker_type is _MarkerType.STRING:
                s += f' "{_decode_and_escape(self.value[pos:end - 1])}"'
                if end != len(self.value):
                    s += ","
            elif marker_type is _MarkerType.PATH:
                s += " &" + ref
                if end != len(self.value):
                    s += ","
            else:
                if marker_type is _MarkerType.LABEL:
                    s += f" {ref}:"
                elif marker_type is _MarkerType.PHANDLE:
                    s += " &" + ref
                    pos += 4
                else:
                    elm_size = _TYPE_TO_N_BYTES[marker_type]
                    s += _N_BYTES_TO_START_STR[elm_size]
                while pos != end:
                    num = int.from_bytes(self.value[pos:pos + elm_size], "big")
                    if elm_size == 1:
                        s += f" {num:02X}"
                    else:
                        s += f" {hex(num)}"
                    pos += elm_size
                if (pos != 0 and (not next_marker or next_marker[1] not in (_MarkerType.PHANDLE, _MarkerType.LABEL))):
                    s += _N_BYTES_TO_END_STR[elm_size]
                    if pos != len(self.value):
                        s += ","
        return s + ";"

    def __repr__(self):
        return (f"<Property '{self.name}' at '{self.node.path}' in '{self.node.dt.filename}'>")

    def _add_marker(self, marker_type: _MarkerType, data: Any = None):
        self._markers.append([len(self.value), marker_type, data])
        if marker_type is _MarkerType.PHANDLE:
            self.value += b"\0\0\0\0"

class DT:
    def __init__(self, filename: Optional[str], include_path: Iterable[str] = (), force: bool = False):
        self._root: Optional[Node] = None
        self.alias2node: dict[str, Node] = {}
        self.label2node: dict[str, Node] = {}
        self.label2prop: dict[str, Property] = {}
        self.label2prop_offset: dict[str, tuple[Property, int]] = {}
        self.phandle2node: dict[int, Node] = {}
        self.memreserves: list[tuple[set[str], int, int]] = []
        self.filename = filename
        self._force = force
        if filename is not None:
            self._parse_file(filename, include_path)
        else:
            self._include_path: list[str] = []

    @property
    def root(self) -> Node:
        return self._root

    def get_node(self, path: str) -> Node:
        if path.startswith("/"):
            return _root_and_path_to_node(self.root, path, path)
        alias, _, rest = path.partition("/")
        if alias not in self.alias2node:
            _err(f"no alias '{alias}' found -- did you forget the leading '/' in the node path?")
        return _root_and_path_to_node(self.alias2node[alias], rest, path)

    def has_node(self, path: str) -> bool:
        try:
            self.get_node(path)
            return True
        except DTError:
            return False

    def move_node(self, node: Node, new_path: str):
        if node is self.root:
            _err("the root node can't be moved")
        if self.has_node(new_path):
            _err(f"can't move '{node.path}' to '{new_path}': destination node exists")
        if not new_path.startswith('/'):
            _err(f"path '{new_path}' doesn't start with '/'")
        for component in new_path.split('/'):
            for char in component:
                if char not in _nodename_chars:
                    _err(f"new path '{new_path}': bad character '{char}'")
        old_name = node.name
        old_path = node.path
        new_parent_path, _, new_name = new_path.rpartition('/')
        if new_parent_path == '':
            new_parent_path = '/'
        if not self.has_node(new_parent_path):
            _err(f"can't move '{old_path}' to '{new_path}': parent node '{new_parent_path}' doesn't exist")
        new_parent = self.get_node(new_parent_path)
        if TYPE_CHECKING:
            assert new_parent is not None
            assert node.parent is not None
        del node.parent.nodes[old_name]
        node._name = new_name
        node.parent = new_parent
        new_parent.nodes[new_name] = node

    def node_iter(self) -> Iterable[Node]:
        yield from self.root.node_iter()

    def __str__(self):
        s = "/dts-v1/;\n\n"
        if self.memreserves:
            for labels, address, offset in self.memreserves:
                for label in labels:
                    s += f"{label}: "
                s += f"/memreserve/ {address:#018x} {offset:#018x};\n"
            s += "\n"
        return s + str(self.root)

    def __repr__(self):
        if self.filename:
            return (f"DT(filename='{self.filename}', include_path={self._include_path})")
        return super().__repr__()

    def __deepcopy__(self, memo):
        ret = DT(None, (), self._force)
        path2node_copy = {node.path: Node(node.name, None, ret, node.filename, node.lineno) for node in self.node_iter()}
        for node in self.node_iter():
            node_copy = path2node_copy[node.path]
            parent = node.parent
            if parent is not None:
                node_copy.parent = path2node_copy[parent.path]
            prop_name2prop_copy = {prop.name: Property(node_copy, prop.name) for prop in node.props.values()}
            for prop_name, prop_copy in prop_name2prop_copy.items():
                prop = node.props[prop_name]
                prop_copy.value = prop.value
                prop_copy.labels = prop.labels[:]
                prop_copy.offset_labels = prop.offset_labels.copy()
                prop_copy._label_offset_lst = prop._label_offset_lst[:]
                prop_copy._markers = [marker[:] for marker in prop._markers]
                prop_copy.filename = prop.filename
                prop_copy.lineno = prop.lineno
            node_copy.props = prop_name2prop_copy
            node_copy.nodes = {child_name: path2node_copy[child_node.path] for child_name, child_node in node.nodes.items()}
            node_copy.labels = node.labels[:]
            node_copy._omit_if_no_ref = node._omit_if_no_ref
            node_copy._is_referenced = node._is_referenced
        ret._root = path2node_copy['/']
        def copy_node_lookup_table(attr_name):
            original = getattr(self, attr_name)
            copy = {key: path2node_copy[original[key].path] for key in original}
            setattr(ret, attr_name, copy)
        copy_node_lookup_table('alias2node')
        copy_node_lookup_table('label2node')
        copy_node_lookup_table('phandle2node')
        ret_label2prop = {}
        for label, prop in self.label2prop.items():
            node_copy = path2node_copy[prop.node.path]
            prop_copy = node_copy.props[prop.name]
            ret_label2prop[label] = prop_copy
        ret.label2prop = ret_label2prop
        ret_label2prop_offset = {}
        for label, prop_offset in self.label2prop_offset.items():
            prop, offset = prop_offset
            node_copy = path2node_copy[prop.node.path]
            prop_copy = node_copy.props[prop.name]
            ret_label2prop_offset[label] = (prop_copy, offset)
        ret.label2prop_offset = ret_label2prop_offset
        ret.memreserves = [(set(memreserve[0]), memreserve[1], memreserve[2]) for memreserve in self.memreserves]
        ret.filename = self.filename
        return ret

    def _parse_file(self, filename: str, include_path: Iterable[str]):
        self._include_path = list(include_path)
        with open(filename, encoding="utf-8") as f:
            self._file_contents = f.read()
        self._tok_i = self._tok_end_i = 0
        self._filestack: list[_FileStackElt] = []
        self._lexer_state: int = _DEFAULT
        self._saved_token: Optional[_Token] = None
        self._lineno: int = 1
        self._parse_header()
        self._parse_memreserves()
        self._parse_dt()
        self._register_phandles()
        self._fixup_props()
        self._register_aliases()
        self._remove_unreferenced()
        self._register_labels()

# Helper functions
def _check_is_bytes(data):
    if not isinstance(data, bytes):
        _err(f"'{data}' has type '{type(data).__name__}', expected 'bytes'")

def _check_length_positive(length):
    if length < 1:
        _err("'length' must be greater than zero, was " + str(length))

def _append_no_dup(lst, elm):
    if elm not in lst:
        lst.append(elm)

def _decode_and_escape(b):
    return (b.decode("utf-8", "surrogateescape")
            .translate(_escape_table)
            .encode("utf-8", "surrogateescape")
            .decode("utf-8", "backslashreplace"))

def _root_and_path_to_node(cur, path, fullpath):
    for component in path.split("/"):
        if not component:
            continue
        if component not in cur.nodes:
            _err(f"component '{component}' in path '{fullpath}' does not exist")
        cur = cur.nodes[component]
    return cur

def _err(msg) -> NoReturn:
    raise DTError(msg)

_escape_table = str.maketrans({
    "\\": "\\\\", '"': '\\"', "\a": "\\a", "\b": "\\b",
    "\t": "\\t", "\n": "\\n", "\v": "\\v", "\f": "\\f", "\r": "\\r"
})

_DEFAULT = 0
_EXPECT_PROPNODENAME = 1
_EXPECT_BYTE = 2

_num_re = re.compile(r"(0[xX][0-9a-fA-F]+|[0-9]+)(?:ULL|UL|LL|U|L)?")
_propnodename_re = re.compile(r"\\?([a-zA-Z0-9,._+*#?@-]+)")
_nodename_chars = set(string.ascii_letters + string.digits + ',._+-@')
_byte_re = re.compile(r"[0-9a-fA-F]{2}")
_unescape_re = re.compile(br'\\([0-7]{1,3}|x[0-9A-Fa-f]{1,2}|.)')

_TYPE_TO_N_BYTES = {
    _MarkerType.UINT8: 1,
    _MarkerType.UINT16: 2,
    _MarkerType.UINT32: 4,
    _MarkerType.UINT64: 8,
}

_N_BYTES_TO_TYPE = {
    1: _MarkerType.UINT8,
    2: _MarkerType.UINT16,
    4: _MarkerType.UINT32,
    8: _MarkerType.UINT64,
}

_N_BYTES_TO_START_STR = {
    1: " [",
    2: " /bits/ 16 <",
    4: " <",
    8: " /bits/ 64 <",
}

_N_BYTES_TO_END_STR = {
    1: " ]",
    2: " >",
    4: " >",
    8: " >",
}
