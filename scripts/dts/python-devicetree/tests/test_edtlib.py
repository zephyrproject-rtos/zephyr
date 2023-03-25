# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: BSD-3-Clause

import contextlib
import io
from logging import WARNING
import os
from pathlib import Path

import pytest

from devicetree import edtlib

# Test suite for edtlib.py.
#
# Run it using pytest (https://docs.pytest.org/en/stable/usage.html):
#
#   $ pytest testedtlib.py
#
# See the comment near the top of testdtlib.py for additional pytest advice.
#
# test.dts is the main test file. test-bindings/ and test-bindings-2/ has
# bindings. The tests mostly use string comparisons via the various __repr__()
# methods.

HERE = os.path.dirname(__file__)

@contextlib.contextmanager
def from_here():
    # Convenience hack to minimize diff from zephyr.
    cwd = os.getcwd()
    try:
        os.chdir(HERE)
        yield
    finally:
        os.chdir(cwd)

def hpath(filename):
    '''Convert 'filename' to the host path syntax.'''
    return os.fspath(Path(filename))

def test_warnings(caplog):
    '''Tests for situations that should cause warnings.'''

    with from_here(): edtlib.EDT("test.dts", ["test-bindings"])

    enums_hpath = hpath('test-bindings/enums.yaml')
    expected_warnings = [
        f"'oldprop' is marked as deprecated in 'properties:' in {hpath('test-bindings/deprecated.yaml')} for node /test-deprecated.",
        "unit address and first address in 'reg' (0x1) don't match for /reg-zero-size-cells/node",
        "unit address and first address in 'reg' (0x5) don't match for /reg-ranges/parent/node",
        "unit address and first address in 'reg' (0x30000000200000001) don't match for /reg-nested-ranges/grandparent/parent/node",
        f"compatible 'enums' in binding '{enums_hpath}' has non-tokenizable enum for property 'string-enum': 'foo bar', 'foo_bar'",
        f"compatible 'enums' in binding '{enums_hpath}' has enum for property 'tokenizable-lower-enum' that is only tokenizable in lowercase: 'bar', 'BAR'",
    ]
    assert caplog.record_tuples == [('devicetree.edtlib', WARNING, warning_message)
                                    for warning_message in expected_warnings]

def test_interrupts():
    '''Tests for the interrupts property.'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])
    filenames = {i: hpath(f'test-bindings/interrupt-{i}-cell.yaml')
                 for i in range(1, 4)}

    assert str(edt.get_node("/interrupt-parent-test/node").interrupts) == \
        f"[<ControllerAndData, name: foo, controller: <Node /interrupt-parent-test/controller in 'test.dts', binding {filenames[3]}>, data: {{'one': 1, 'two': 2, 'three': 3}}>, <ControllerAndData, name: bar, controller: <Node /interrupt-parent-test/controller in 'test.dts', binding {filenames[3]}>, data: {{'one': 4, 'two': 5, 'three': 6}}>]"

    assert str(edt.get_node("/interrupts-extended-test/node").interrupts) == \
        f"[<ControllerAndData, controller: <Node /interrupts-extended-test/controller-0 in 'test.dts', binding {filenames[1]}>, data: {{'one': 1}}>, <ControllerAndData, controller: <Node /interrupts-extended-test/controller-1 in 'test.dts', binding {filenames[2]}>, data: {{'one': 2, 'two': 3}}>, <ControllerAndData, controller: <Node /interrupts-extended-test/controller-2 in 'test.dts', binding {filenames[3]}>, data: {{'one': 4, 'two': 5, 'three': 6}}>]"

    assert str(edt.get_node("/interrupt-map-test/node@0").interrupts) == \
        f"[<ControllerAndData, controller: <Node /interrupt-map-test/controller-0 in 'test.dts', binding {filenames[1]}>, data: {{'one': 0}}>, <ControllerAndData, controller: <Node /interrupt-map-test/controller-1 in 'test.dts', binding {filenames[2]}>, data: {{'one': 0, 'two': 1}}>, <ControllerAndData, controller: <Node /interrupt-map-test/controller-2 in 'test.dts', binding {filenames[3]}>, data: {{'one': 0, 'two': 0, 'three': 2}}>]"

    assert str(edt.get_node("/interrupt-map-test/node@1").interrupts) == \
        f"[<ControllerAndData, controller: <Node /interrupt-map-test/controller-0 in 'test.dts', binding {filenames[1]}>, data: {{'one': 3}}>, <ControllerAndData, controller: <Node /interrupt-map-test/controller-1 in 'test.dts', binding {filenames[2]}>, data: {{'one': 0, 'two': 4}}>, <ControllerAndData, controller: <Node /interrupt-map-test/controller-2 in 'test.dts', binding {filenames[3]}>, data: {{'one': 0, 'two': 0, 'three': 5}}>]"

    assert str(edt.get_node("/interrupt-map-bitops-test/node@70000000E").interrupts) == \
        f"[<ControllerAndData, controller: <Node /interrupt-map-bitops-test/controller in 'test.dts', binding {filenames[2]}>, data: {{'one': 3, 'two': 2}}>]"

def test_ranges():
    '''Tests for the ranges property'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])

    assert str(edt.get_node("/reg-ranges/parent").ranges) == \
        "[<Range, child-bus-cells: 0x1, child-bus-addr: 0x1, parent-bus-cells: 0x2, parent-bus-addr: 0xa0000000b, length-cells 0x1, length 0x1>, <Range, child-bus-cells: 0x1, child-bus-addr: 0x2, parent-bus-cells: 0x2, parent-bus-addr: 0xc0000000d, length-cells 0x1, length 0x2>, <Range, child-bus-cells: 0x1, child-bus-addr: 0x4, parent-bus-cells: 0x2, parent-bus-addr: 0xe0000000f, length-cells 0x1, length 0x1>]"

    assert str(edt.get_node("/reg-nested-ranges/grandparent").ranges) == \
        "[<Range, child-bus-cells: 0x2, child-bus-addr: 0x0, parent-bus-cells: 0x3, parent-bus-addr: 0x30000000000000000, length-cells 0x2, length 0x200000002>]"

    assert str(edt.get_node("/reg-nested-ranges/grandparent/parent").ranges) == \
        "[<Range, child-bus-cells: 0x1, child-bus-addr: 0x0, parent-bus-cells: 0x2, parent-bus-addr: 0x200000000, length-cells 0x1, length 0x2>]"

    assert str(edt.get_node("/ranges-zero-cells/node").ranges) == "[]"

    assert str(edt.get_node("/ranges-zero-parent-cells/node").ranges) == \
        "[<Range, child-bus-cells: 0x1, child-bus-addr: 0xa, parent-bus-cells: 0x0, length-cells 0x0>, <Range, child-bus-cells: 0x1, child-bus-addr: 0x1a, parent-bus-cells: 0x0, length-cells 0x0>, <Range, child-bus-cells: 0x1, child-bus-addr: 0x2a, parent-bus-cells: 0x0, length-cells 0x0>]"

    assert str(edt.get_node("/ranges-one-address-cells/node").ranges) == \
        "[<Range, child-bus-cells: 0x1, child-bus-addr: 0xa, parent-bus-cells: 0x0, length-cells 0x1, length 0xb>, <Range, child-bus-cells: 0x1, child-bus-addr: 0x1a, parent-bus-cells: 0x0, length-cells 0x1, length 0x1b>, <Range, child-bus-cells: 0x1, child-bus-addr: 0x2a, parent-bus-cells: 0x0, length-cells 0x1, length 0x2b>]"

    assert str(edt.get_node("/ranges-one-address-two-size-cells/node").ranges) == \
        "[<Range, child-bus-cells: 0x1, child-bus-addr: 0xa, parent-bus-cells: 0x0, length-cells 0x2, length 0xb0000000c>, <Range, child-bus-cells: 0x1, child-bus-addr: 0x1a, parent-bus-cells: 0x0, length-cells 0x2, length 0x1b0000001c>, <Range, child-bus-cells: 0x1, child-bus-addr: 0x2a, parent-bus-cells: 0x0, length-cells 0x2, length 0x2b0000002c>]"

    assert str(edt.get_node("/ranges-two-address-cells/node@1").ranges) == \
        "[<Range, child-bus-cells: 0x2, child-bus-addr: 0xa0000000b, parent-bus-cells: 0x1, parent-bus-addr: 0xc, length-cells 0x1, length 0xd>, <Range, child-bus-cells: 0x2, child-bus-addr: 0x1a0000001b, parent-bus-cells: 0x1, parent-bus-addr: 0x1c, length-cells 0x1, length 0x1d>, <Range, child-bus-cells: 0x2, child-bus-addr: 0x2a0000002b, parent-bus-cells: 0x1, parent-bus-addr: 0x2c, length-cells 0x1, length 0x2d>]"

    assert str(edt.get_node("/ranges-two-address-two-size-cells/node@1").ranges) == \
        "[<Range, child-bus-cells: 0x2, child-bus-addr: 0xa0000000b, parent-bus-cells: 0x1, parent-bus-addr: 0xc, length-cells 0x2, length 0xd0000000e>, <Range, child-bus-cells: 0x2, child-bus-addr: 0x1a0000001b, parent-bus-cells: 0x1, parent-bus-addr: 0x1c, length-cells 0x2, length 0x1d0000001e>, <Range, child-bus-cells: 0x2, child-bus-addr: 0x2a0000002b, parent-bus-cells: 0x1, parent-bus-addr: 0x2c, length-cells 0x2, length 0x2d0000001d>]"

    assert str(edt.get_node("/ranges-three-address-cells/node@1").ranges) == \
        "[<Range, child-bus-cells: 0x3, child-bus-addr: 0xa0000000b0000000c, parent-bus-cells: 0x2, parent-bus-addr: 0xd0000000e, length-cells 0x1, length 0xf>, <Range, child-bus-cells: 0x3, child-bus-addr: 0x1a0000001b0000001c, parent-bus-cells: 0x2, parent-bus-addr: 0x1d0000001e, length-cells 0x1, length 0x1f>, <Range, child-bus-cells: 0x3, child-bus-addr: 0x2a0000002b0000002c, parent-bus-cells: 0x2, parent-bus-addr: 0x2d0000002e, length-cells 0x1, length 0x2f>]"

    assert str(edt.get_node("/ranges-three-address-two-size-cells/node@1").ranges) == \
        "[<Range, child-bus-cells: 0x3, child-bus-addr: 0xa0000000b0000000c, parent-bus-cells: 0x2, parent-bus-addr: 0xd0000000e, length-cells 0x2, length 0xf00000010>, <Range, child-bus-cells: 0x3, child-bus-addr: 0x1a0000001b0000001c, parent-bus-cells: 0x2, parent-bus-addr: 0x1d0000001e, length-cells 0x2, length 0x1f00000110>, <Range, child-bus-cells: 0x3, child-bus-addr: 0x2a0000002b0000002c, parent-bus-cells: 0x2, parent-bus-addr: 0x2d0000002e, length-cells 0x2, length 0x2f00000210>]"

def test_reg():
    '''Tests for the regs property'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])

    assert str(edt.get_node("/reg-zero-address-cells/node").regs) == \
        "[<Register, size: 0x1>, <Register, size: 0x2>]"

    assert str(edt.get_node("/reg-zero-size-cells/node").regs) == \
        "[<Register, addr: 0x1>, <Register, addr: 0x2>]"

    assert str(edt.get_node("/reg-ranges/parent/node").regs) == \
        "[<Register, addr: 0x5, size: 0x1>, <Register, addr: 0xe0000000f, size: 0x1>, <Register, addr: 0xc0000000e, size: 0x1>, <Register, addr: 0xc0000000d, size: 0x1>, <Register, addr: 0xa0000000b, size: 0x1>, <Register, addr: 0x0, size: 0x1>]"

    assert str(edt.get_node("/reg-nested-ranges/grandparent/parent/node").regs) == \
        "[<Register, addr: 0x30000000200000001, size: 0x1>]"

def test_pinctrl():
    '''Test 'pinctrl-<index>'.'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])

    assert str(edt.get_node("/pinctrl/dev").pinctrls) == \
        "[<PinCtrl, name: zero, configuration nodes: []>, <PinCtrl, name: one, configuration nodes: [<Node /pinctrl/pincontroller/state-1 in 'test.dts', no binding>]>, <PinCtrl, name: two, configuration nodes: [<Node /pinctrl/pincontroller/state-1 in 'test.dts', no binding>, <Node /pinctrl/pincontroller/state-2 in 'test.dts', no binding>]>]"

def test_hierarchy():
    '''Test Node.parent and Node.children'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])

    assert edt.get_node("/").parent is None

    assert str(edt.get_node("/parent/child-1").parent) == \
        "<Node /parent in 'test.dts', no binding>"

    assert str(edt.get_node("/parent/child-2/grandchild").parent) == \
        "<Node /parent/child-2 in 'test.dts', no binding>"

    assert str(edt.get_node("/parent").children) == \
        "{'child-1': <Node /parent/child-1 in 'test.dts', no binding>, 'child-2': <Node /parent/child-2 in 'test.dts', no binding>}"

    assert edt.get_node("/parent/child-1").children == {}

def test_child_index():
    '''Test Node.child_index.'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])

    parent, child_1, child_2 = [edt.get_node(path) for path in
                                ("/parent",
                                 "/parent/child-1",
                                 "/parent/child-2")]
    assert parent.child_index(child_1) == 0
    assert parent.child_index(child_2) == 1
    with pytest.raises(KeyError):
        parent.child_index(parent)

def test_include():
    '''Test 'include:' and the legacy 'inherits: !include ...' in bindings'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])

    assert str(edt.get_node("/binding-include").description) == \
        "Parent binding"

    assert str(edt.get_node("/binding-include").props) == (
        "{"
        "'foo': <Property, name: foo, type: int, value: 0>, "
        "'bar': <Property, name: bar, type: int, value: 1>, "
        "'baz': <Property, name: baz, type: int, value: 2>, "
        "'qaz': <Property, name: qaz, type: int, value: 3>"
        "}"
    )

    assert str(edt.get_node("/binding-include/child").props) == (
        "{"
        "'foo': <Property, name: foo, type: int, value: 0>, "
        "'bar': <Property, name: bar, type: int, value: 1>, "
        "'baz': <Property, name: baz, type: int, value: 2>, "
        "'qaz': <Property, name: qaz, type: int, value: 3>"
        "}"
    )

def test_include_filters():
    '''Test property-allowlist and property-blocklist in an include.'''

    fname2path = {'include.yaml': 'test-bindings-include/include.yaml',
                  'include-2.yaml': 'test-bindings-include/include-2.yaml'}

    with pytest.raises(edtlib.EDTError) as e:
        with from_here():
            edtlib.Binding("test-bindings-include/allow-and-blocklist.yaml", fname2path)
    assert ("should not specify both 'property-allowlist:' and 'property-blocklist:'"
            in str(e.value))

    with pytest.raises(edtlib.EDTError) as e:
        with from_here():
            edtlib.Binding("test-bindings-include/allow-and-blocklist-child.yaml", fname2path)
    assert ("should not specify both 'property-allowlist:' and 'property-blocklist:'"
            in str(e.value))

    with pytest.raises(edtlib.EDTError) as e:
        with from_here():
            edtlib.Binding("test-bindings-include/allow-not-list.yaml", fname2path)
    value_str = str(e.value)
    assert value_str.startswith("'property-allowlist' value")
    assert value_str.endswith("should be a list")

    with pytest.raises(edtlib.EDTError) as e:
        with from_here():
            edtlib.Binding("test-bindings-include/block-not-list.yaml", fname2path)
    value_str = str(e.value)
    assert value_str.startswith("'property-blocklist' value")
    assert value_str.endswith("should be a list")

    with pytest.raises(edtlib.EDTError) as e:
        with from_here():
            binding = edtlib.Binding("test-bindings-include/include-invalid-keys.yaml", fname2path)
    value_str = str(e.value)
    assert value_str.startswith(
        "'include:' in test-bindings-include/include-invalid-keys.yaml should not have these "
        "unexpected contents: ")
    assert 'bad-key-1' in value_str
    assert 'bad-key-2' in value_str

    with pytest.raises(edtlib.EDTError) as e:
        with from_here():
            binding = edtlib.Binding("test-bindings-include/include-invalid-type.yaml", fname2path)
    value_str = str(e.value)
    assert value_str.startswith(
        "'include:' in test-bindings-include/include-invalid-type.yaml "
        "should be a string or list, but has type ")

    with pytest.raises(edtlib.EDTError) as e:
        with from_here():
            binding = edtlib.Binding("test-bindings-include/include-no-name.yaml", fname2path)
    value_str = str(e.value)
    assert value_str.startswith("'include:' element")
    assert value_str.endswith(
        "in test-bindings-include/include-no-name.yaml should have a 'name' key")

    with from_here():
        binding = edtlib.Binding("test-bindings-include/allowlist.yaml", fname2path)
        assert set(binding.prop2specs.keys()) == {'x'}  # 'x' is allowed

        binding = edtlib.Binding("test-bindings-include/empty-allowlist.yaml", fname2path)
        assert set(binding.prop2specs.keys()) == set()  # nothing is allowed

        binding = edtlib.Binding("test-bindings-include/blocklist.yaml", fname2path)
        assert set(binding.prop2specs.keys()) == {'y', 'z'}  # 'x' is blocked

        binding = edtlib.Binding("test-bindings-include/empty-blocklist.yaml", fname2path)
        assert set(binding.prop2specs.keys()) == {'x', 'y', 'z'}  # nothing is blocked

        binding = edtlib.Binding("test-bindings-include/intermixed.yaml", fname2path)
        assert set(binding.prop2specs.keys()) == {'x', 'a'}

        binding = edtlib.Binding("test-bindings-include/include-no-list.yaml", fname2path)
        assert set(binding.prop2specs.keys()) == {'x', 'y', 'z'}

        binding = edtlib.Binding("test-bindings-include/filter-child-bindings.yaml", fname2path)
        child = binding.child_binding
        grandchild = child.child_binding
        assert set(binding.prop2specs.keys()) == {'x'}
        assert set(child.prop2specs.keys()) == {'child-prop-2'}
        assert set(grandchild.prop2specs.keys()) == {'grandchild-prop-1'}

        binding = edtlib.Binding("test-bindings-include/allow-and-blocklist-multilevel.yaml",
                                 fname2path)
        assert set(binding.prop2specs.keys()) == {'x'}  # 'x' is allowed
        child = binding.child_binding
        assert set(child.prop2specs.keys()) == {'child-prop-1', 'child-prop-2',
                                                'x', 'z'}  # root level 'y' is blocked


def test_bus():
    '''Test 'bus:' and 'on-bus:' in bindings'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])

    assert isinstance(edt.get_node("/buses/foo-bus").buses, list)
    assert "foo" in edt.get_node("/buses/foo-bus").buses

    # foo-bus does not itself appear on a bus
    assert isinstance(edt.get_node("/buses/foo-bus").on_buses, list)
    assert not edt.get_node("/buses/foo-bus").on_buses
    assert edt.get_node("/buses/foo-bus").bus_node is None

    # foo-bus/node1 is not a bus node...
    assert isinstance(edt.get_node("/buses/foo-bus/node1").buses, list)
    assert not edt.get_node("/buses/foo-bus/node1").buses
    # ...but is on a bus
    assert isinstance(edt.get_node("/buses/foo-bus/node1").on_buses, list)
    assert "foo" in edt.get_node("/buses/foo-bus/node1").on_buses
    assert edt.get_node("/buses/foo-bus/node1").bus_node.path == \
        "/buses/foo-bus"

    # foo-bus/node2 is not a bus node...
    assert isinstance(edt.get_node("/buses/foo-bus/node2").buses, list)
    assert not edt.get_node("/buses/foo-bus/node2").buses
    # ...but is on a bus
    assert isinstance(edt.get_node("/buses/foo-bus/node2").on_buses, list)
    assert "foo" in edt.get_node("/buses/foo-bus/node2").on_buses

    # no-bus-node is not a bus node...
    assert isinstance(edt.get_node("/buses/no-bus-node").buses, list)
    assert not edt.get_node("/buses/no-bus-node").buses
    # ... and is not on a bus
    assert isinstance(edt.get_node("/buses/no-bus-node").on_buses, list)
    assert not edt.get_node("/buses/no-bus-node").on_buses

    # Same compatible string, but different bindings from being on different
    # buses
    assert str(edt.get_node("/buses/foo-bus/node1").binding_path) == \
        hpath("test-bindings/device-on-foo-bus.yaml")
    assert str(edt.get_node("/buses/foo-bus/node2").binding_path) == \
        hpath("test-bindings/device-on-any-bus.yaml")
    assert str(edt.get_node("/buses/bar-bus/node").binding_path) == \
        hpath("test-bindings/device-on-bar-bus.yaml")
    assert str(edt.get_node("/buses/no-bus-node").binding_path) == \
        hpath("test-bindings/device-on-any-bus.yaml")

    # foo-bus/node/nested also appears on the foo-bus bus
    assert isinstance(edt.get_node("/buses/foo-bus/node1/nested").on_buses, list)
    assert "foo" in edt.get_node("/buses/foo-bus/node1/nested").on_buses
    assert str(edt.get_node("/buses/foo-bus/node1/nested").binding_path) == \
        hpath("test-bindings/device-on-foo-bus.yaml")

def test_child_binding():
    '''Test 'child-binding:' in bindings'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])
    child1 = edt.get_node("/child-binding/child-1")
    child2 = edt.get_node("/child-binding/child-2")
    grandchild = edt.get_node("/child-binding/child-1/grandchild")

    assert str(child1.binding_path) == hpath("test-bindings/child-binding.yaml")
    assert str(child1.description) == "child node"
    assert str(child1.props) == "{'child-prop': <Property, name: child-prop, type: int, value: 1>}"

    assert str(child2.binding_path) == hpath("test-bindings/child-binding.yaml")
    assert str(child2.description) == "child node"
    assert str(child2.props) == "{'child-prop': <Property, name: child-prop, type: int, value: 3>}"

    assert str(grandchild.binding_path) == hpath("test-bindings/child-binding.yaml")
    assert str(grandchild.description) == "grandchild node"
    assert str(grandchild.props) == "{'grandchild-prop': <Property, name: grandchild-prop, type: int, value: 2>}"

    with from_here():
        binding_file = Path("test-bindings/child-binding.yaml").resolve()
        top = edtlib.Binding(binding_file, {})
    child = top.child_binding
    assert Path(top.path) == binding_file
    assert Path(child.path) == binding_file
    assert top.compatible == 'top-binding'
    assert child.compatible is None

    with from_here():
        binding_file = Path("test-bindings/child-binding-with-compat.yaml").resolve()
        top = edtlib.Binding(binding_file, {})
    child = top.child_binding
    assert Path(top.path) == binding_file
    assert Path(child.path) == binding_file
    assert top.compatible == 'top-binding-with-compat'
    assert child.compatible == 'child-compat'

def test_props():
    '''Test Node.props (derived from DT and 'properties:' in the binding)'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])
    filenames = {i: hpath(f'test-bindings/phandle-array-controller-{i}.yaml')
                 for i in range(0, 4)}

    assert str(edt.get_node("/props").props["int"]) == \
        "<Property, name: int, type: int, value: 1>"

    assert str(edt.get_node("/props").props["existent-boolean"]) == \
        "<Property, name: existent-boolean, type: boolean, value: True>"

    assert str(edt.get_node("/props").props["nonexistent-boolean"]) == \
        "<Property, name: nonexistent-boolean, type: boolean, value: False>"

    assert str(edt.get_node("/props").props["array"]) == \
        "<Property, name: array, type: array, value: [1, 2, 3]>"

    assert str(edt.get_node("/props").props["uint8-array"]) == \
        r"<Property, name: uint8-array, type: uint8-array, value: b'\x124'>"

    assert str(edt.get_node("/props").props["string"]) == \
        "<Property, name: string, type: string, value: 'foo'>"

    assert str(edt.get_node("/props").props["string-array"]) == \
        "<Property, name: string-array, type: string-array, value: ['foo', 'bar', 'baz']>"

    assert str(edt.get_node("/props").props["phandle-ref"]) == \
        f"<Property, name: phandle-ref, type: phandle, value: <Node /ctrl-1 in 'test.dts', binding {filenames[1]}>>"

    assert str(edt.get_node("/props").props["phandle-refs"]) == \
        f"<Property, name: phandle-refs, type: phandles, value: [<Node /ctrl-1 in 'test.dts', binding {filenames[1]}>, <Node /ctrl-2 in 'test.dts', binding {filenames[2]}>]>"

    assert str(edt.get_node("/props").props["phandle-array-foos"]) == \
        f"<Property, name: phandle-array-foos, type: phandle-array, value: [<ControllerAndData, controller: <Node /ctrl-1 in 'test.dts', binding {filenames[1]}>, data: {{'one': 1}}>, <ControllerAndData, controller: <Node /ctrl-2 in 'test.dts', binding {filenames[2]}>, data: {{'one': 2, 'two': 3}}>]>"

    assert str(edt.get_node("/props-2").props["phandle-array-foos"]) == \
        ("<Property, name: phandle-array-foos, type: phandle-array, value: ["
         f"<ControllerAndData, name: a, controller: <Node /ctrl-0-1 in 'test.dts', binding {filenames[0]}>, data: {{}}>, "
         "None, "
         f"<ControllerAndData, name: b, controller: <Node /ctrl-0-2 in 'test.dts', binding {filenames[0]}>, data: {{}}>]>")

    assert str(edt.get_node("/props").props["foo-gpios"]) == \
        f"<Property, name: foo-gpios, type: phandle-array, value: [<ControllerAndData, controller: <Node /ctrl-1 in 'test.dts', binding {filenames[1]}>, data: {{'gpio-one': 1}}>]>"

    assert str(edt.get_node("/props").props["path"]) == \
        f"<Property, name: path, type: path, value: <Node /ctrl-1 in 'test.dts', binding {filenames[1]}>>"

def test_nexus():
    '''Test <prefix>-map via gpio-map (the most common case).'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])
    filename = hpath('test-bindings/gpio-dst.yaml')

    assert str(edt.get_node("/gpio-map/source").props["foo-gpios"]) == \
        f"<Property, name: foo-gpios, type: phandle-array, value: [<ControllerAndData, controller: <Node /gpio-map/destination in 'test.dts', binding {filename}>, data: {{'val': 6}}>, <ControllerAndData, controller: <Node /gpio-map/destination in 'test.dts', binding {filename}>, data: {{'val': 5}}>]>"

    assert str(edt.get_node("/gpio-map/source").props["foo-gpios"].val[0].basename) == f"gpio"

def test_prop_defaults():
    '''Test property default values given in bindings'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])

    assert str(edt.get_node("/defaults").props) == \
        r"{'int': <Property, name: int, type: int, value: 123>, 'array': <Property, name: array, type: array, value: [1, 2, 3]>, 'uint8-array': <Property, name: uint8-array, type: uint8-array, value: b'\x89\xab\xcd'>, 'string': <Property, name: string, type: string, value: 'hello'>, 'string-array': <Property, name: string-array, type: string-array, value: ['hello', 'there']>, 'default-not-used': <Property, name: default-not-used, type: int, value: 234>}"

def test_prop_enums():
    '''test properties with enum: in the binding'''

    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])
    props = edt.get_node('/enums').props
    int_enum = props['int-enum']
    string_enum = props['string-enum']
    tokenizable_enum = props['tokenizable-enum']
    tokenizable_lower_enum = props['tokenizable-lower-enum']
    no_enum = props['no-enum']

    assert int_enum.val == 1
    assert int_enum.enum_index == 0
    assert not int_enum.spec.enum_tokenizable
    assert not int_enum.spec.enum_upper_tokenizable

    assert string_enum.val == 'foo_bar'
    assert string_enum.enum_index == 1
    assert not string_enum.spec.enum_tokenizable
    assert not string_enum.spec.enum_upper_tokenizable

    assert tokenizable_enum.val == '123 is ok'
    assert tokenizable_enum.val_as_token == '123_is_ok'
    assert tokenizable_enum.enum_index == 2
    assert tokenizable_enum.spec.enum_tokenizable
    assert tokenizable_enum.spec.enum_upper_tokenizable

    assert tokenizable_lower_enum.val == 'bar'
    assert tokenizable_lower_enum.val_as_token == 'bar'
    assert tokenizable_lower_enum.enum_index == 0
    assert tokenizable_lower_enum.spec.enum_tokenizable
    assert not tokenizable_lower_enum.spec.enum_upper_tokenizable

    assert no_enum.enum_index is None
    assert not no_enum.spec.enum_tokenizable
    assert not no_enum.spec.enum_upper_tokenizable

def test_binding_inference():
    '''Test inferred bindings for special zephyr-specific nodes.'''
    warnings = io.StringIO()
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"], warnings)

    assert str(edt.get_node("/zephyr,user").props) == '{}'

    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"], warnings,
                         infer_binding_for_paths=["/zephyr,user"])
    filenames = {i: hpath(f'test-bindings/phandle-array-controller-{i}.yaml')
                 for i in range(1, 3)}

    assert str(edt.get_node("/zephyr,user").props) == (
        "{"
        "'boolean': <Property, name: boolean, type: boolean, value: True>, "
        r"'bytes': <Property, name: bytes, type: uint8-array, value: b'\x81\x82\x83'>, "
        "'number': <Property, name: number, type: int, value: 23>, "
        "'numbers': <Property, name: numbers, type: array, value: [1, 2, 3]>, "
        "'string': <Property, name: string, type: string, value: 'text'>, "
        "'strings': <Property, name: strings, type: string-array, value: ['a', 'b', 'c']>, "
        f"'handle': <Property, name: handle, type: phandle, value: <Node /ctrl-1 in 'test.dts', binding {filenames[1]}>>, "
        f"'phandles': <Property, name: phandles, type: phandles, value: [<Node /ctrl-1 in 'test.dts', binding {filenames[1]}>, <Node /ctrl-2 in 'test.dts', binding {filenames[2]}>]>, "
        f"'phandle-array-foos': <Property, name: phandle-array-foos, type: phandle-array, value: [<ControllerAndData, controller: <Node /ctrl-2 in 'test.dts', binding {filenames[2]}>, data: {{'one': 1, 'two': 2}}>]>"
        "}"
    )

def test_multi_bindings():
    '''Test having multiple directories with bindings'''
    with from_here():
        edt = edtlib.EDT("test-multidir.dts", ["test-bindings", "test-bindings-2"])

    assert str(edt.get_node("/in-dir-1").binding_path) == \
        hpath("test-bindings/multidir.yaml")

    assert str(edt.get_node("/in-dir-2").binding_path) == \
        hpath("test-bindings-2/multidir.yaml")

def test_dependencies():
    ''''Test dependency relations'''
    with from_here():
        edt = edtlib.EDT("test-multidir.dts", ["test-bindings", "test-bindings-2"])

    assert edt.get_node("/").dep_ordinal == 0
    assert edt.get_node("/in-dir-1").dep_ordinal == 1
    assert edt.get_node("/") in edt.get_node("/in-dir-1").depends_on
    assert edt.get_node("/in-dir-1") in edt.get_node("/").required_by

def test_slice_errs(tmp_path):
    '''Test error messages from the internal _slice() helper'''

    dts_file = tmp_path / "error.dts"

    verify_error("""
/dts-v1/;

/ {
	#address-cells = <1>;
	#size-cells = <2>;

	sub {
		reg = <3>;
	};
};
""",
                 dts_file,
                 f"'reg' property in <Node /sub in '{dts_file}'> has length 4, which is not evenly divisible by 12 (= 4*(<#address-cells> (= 1) + <#size-cells> (= 2))). Note that #*-cells properties come either from the parent node or from the controller (in the case of 'interrupts').")

    verify_error("""
/dts-v1/;

/ {
	sub {
		interrupts = <1>;
		interrupt-parent = < &{/controller} >;
	};
	controller {
		interrupt-controller;
		#interrupt-cells = <2>;
	};
};
""",
                 dts_file,
                 f"'interrupts' property in <Node /sub in '{dts_file}'> has length 4, which is not evenly divisible by 8 (= 4*<#interrupt-cells>). Note that #*-cells properties come either from the parent node or from the controller (in the case of 'interrupts').")

    verify_error("""
/dts-v1/;

/ {
	#address-cells = <1>;

	sub-1 {
		#address-cells = <2>;
		#size-cells = <3>;
		ranges = <4 5>;

		sub-2 {
			reg = <1 2 3 4 5>;
		};
	};
};
""",
                 dts_file,
                 f"'ranges' property in <Node /sub-1 in '{dts_file}'> has length 8, which is not evenly divisible by 24 (= 4*(<#address-cells> (= 2) + <#address-cells for parent> (= 1) + <#size-cells> (= 3))). Note that #*-cells properties come either from the parent node or from the controller (in the case of 'interrupts').")

def test_bad_compatible(tmp_path):
    # An invalid compatible should cause an error, even on a node with
    # no binding.

    dts_file = tmp_path / "error.dts"

    verify_error("""
/dts-v1/;

/ {
	foo {
		compatible = "no, whitespace";
	};
};
""",
                 dts_file,
                 r"node '/foo' compatible 'no, whitespace' must match this regular expression: '^[a-zA-Z][a-zA-Z0-9,+\-._]+$'")

def test_wrong_props():
    '''Test Node.wrong_props (derived from DT and 'properties:' in the binding)'''

    with from_here():
        with pytest.raises(edtlib.EDTError) as e:
            edtlib.Binding("test-wrong-bindings/wrong-specifier-space-type.yaml", None)
        assert ("'specifier-space' in 'properties: wrong-type-for-specifier-space' has type 'phandle', expected 'phandle-array'"
            in str(e.value))

        with pytest.raises(edtlib.EDTError) as e:
            edtlib.Binding("test-wrong-bindings/wrong-phandle-array-name.yaml", None)
        value_str = str(e.value)
        assert value_str.startswith("'wrong-phandle-array-name' in 'properties:'")
        assert value_str.endswith("but no 'specifier-space' was provided.")


def verify_error(dts, dts_file, expected_err, bindings_dirs=None):
    # Verifies that parsing a file 'dts_file' with the contents 'dts'
    # (a string) raises an EDTError with the message 'expected_err'.
    #
    # The path 'dts_file' is written with the string 'dts' before the
    # test is run.
    #
    # You can optionally specify a list of directories containing bindings
    # to use in 'bindings_dirs'.

    with open(dts_file, "w", encoding="utf-8") as f:
        f.write(dts)
        f.flush()  # Can't have unbuffered text IO, so flush() instead

    if bindings_dirs is None:
        bindings_dirs = []

    with pytest.raises(edtlib.EDTError) as e:
        edtlib.EDT(dts_file, bindings_dirs)

    assert str(e.value) == expected_err
