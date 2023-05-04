# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: BSD-3-Clause

import contextlib
from copy import deepcopy
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

    node = edt.get_node("/interrupt-parent-test/node")
    controller = edt.get_node('/interrupt-parent-test/controller')
    assert node.interrupts == [
        edtlib.ControllerAndData(node=node, controller=controller, data={'one': 1, 'two': 2, 'three': 3}, name='foo', basename=None),
        edtlib.ControllerAndData(node=node, controller=controller, data={'one': 4, 'two': 5, 'three': 6}, name='bar', basename=None)
    ]

    node = edt.get_node("/interrupts-extended-test/node")
    controller_0 = edt.get_node('/interrupts-extended-test/controller-0')
    controller_1 = edt.get_node('/interrupts-extended-test/controller-1')
    controller_2 = edt.get_node('/interrupts-extended-test/controller-2')
    assert node.interrupts == [
        edtlib.ControllerAndData(node=node, controller=controller_0, data={'one': 1}, name=None, basename=None),
        edtlib.ControllerAndData(node=node, controller=controller_1, data={'one': 2, 'two': 3}, name=None, basename=None),
        edtlib.ControllerAndData(node=node, controller=controller_2, data={'one': 4, 'two': 5, 'three': 6}, name=None, basename=None)
    ]

    node = edt.get_node("/interrupt-map-test/node@0")
    controller_0 = edt.get_node('/interrupt-map-test/controller-0')
    controller_1 = edt.get_node('/interrupt-map-test/controller-1')
    controller_2 = edt.get_node('/interrupt-map-test/controller-2')

    assert node.interrupts == [
        edtlib.ControllerAndData(node=node, controller=controller_0, data={'one': 0}, name=None, basename=None),
        edtlib.ControllerAndData(node=node, controller=controller_1, data={'one': 0, 'two': 1}, name=None, basename=None),
        edtlib.ControllerAndData(node=node, controller=controller_2, data={'one': 0, 'two': 0, 'three': 2}, name=None, basename=None)
    ]

    node = edt.get_node("/interrupt-map-test/node@1")
    assert node.interrupts == [
        edtlib.ControllerAndData(node=node, controller=controller_0, data={'one': 3}, name=None, basename=None),
        edtlib.ControllerAndData(node=node, controller=controller_1, data={'one': 0, 'two': 4}, name=None, basename=None),
        edtlib.ControllerAndData(node=node, controller=controller_2, data={'one': 0, 'two': 0, 'three': 5}, name=None, basename=None)
    ]

    node = edt.get_node("/interrupt-map-bitops-test/node@70000000E")
    assert node.interrupts == [
        edtlib.ControllerAndData(node=node, controller=edt.get_node('/interrupt-map-bitops-test/controller'), data={'one': 3, 'two': 2}, name=None, basename=None)
    ]

def test_ranges():
    '''Tests for the ranges property'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])

    node = edt.get_node("/reg-ranges/parent")
    assert node.ranges == [
        edtlib.Range(node=node, child_bus_cells=0x1, child_bus_addr=0x1, parent_bus_cells=0x2, parent_bus_addr=0xa0000000b, length_cells=0x1, length=0x1),
        edtlib.Range(node=node, child_bus_cells=0x1, child_bus_addr=0x2, parent_bus_cells=0x2, parent_bus_addr=0xc0000000d, length_cells=0x1, length=0x2),
        edtlib.Range(node=node, child_bus_cells=0x1, child_bus_addr=0x4, parent_bus_cells=0x2, parent_bus_addr=0xe0000000f, length_cells=0x1, length=0x1)
    ]

    node = edt.get_node("/reg-nested-ranges/grandparent")
    assert node.ranges == [
        edtlib.Range(node=node, child_bus_cells=0x2, child_bus_addr=0x0, parent_bus_cells=0x3, parent_bus_addr=0x30000000000000000, length_cells=0x2, length=0x200000002)
    ]

    node = edt.get_node("/reg-nested-ranges/grandparent/parent")
    assert node.ranges == [
        edtlib.Range(node=node, child_bus_cells=0x1, child_bus_addr=0x0, parent_bus_cells=0x2, parent_bus_addr=0x200000000, length_cells=0x1, length=0x2)
    ]

    assert edt.get_node("/ranges-zero-cells/node").ranges == []

    node = edt.get_node("/ranges-zero-parent-cells/node")
    assert node.ranges == [
        edtlib.Range(node=node, child_bus_cells=0x1, child_bus_addr=0xa, parent_bus_cells=0x0, parent_bus_addr=None, length_cells=0x0, length=None),
        edtlib.Range(node=node, child_bus_cells=0x1, child_bus_addr=0x1a, parent_bus_cells=0x0, parent_bus_addr=None, length_cells=0x0, length=None),
        edtlib.Range(node=node, child_bus_cells=0x1, child_bus_addr=0x2a, parent_bus_cells=0x0, parent_bus_addr=None, length_cells=0x0, length=None)
    ]

    node = edt.get_node("/ranges-one-address-cells/node")
    assert node.ranges == [
        edtlib.Range(node=node, child_bus_cells=0x1, child_bus_addr=0xa, parent_bus_cells=0x0, parent_bus_addr=None, length_cells=0x1, length=0xb),
        edtlib.Range(node=node, child_bus_cells=0x1, child_bus_addr=0x1a, parent_bus_cells=0x0, parent_bus_addr=None, length_cells=0x1, length=0x1b),
        edtlib.Range(node=node, child_bus_cells=0x1, child_bus_addr=0x2a, parent_bus_cells=0x0, parent_bus_addr=None, length_cells=0x1, length=0x2b)
    ]

    node = edt.get_node("/ranges-one-address-two-size-cells/node")
    assert node.ranges == [
        edtlib.Range(node=node, child_bus_cells=0x1, child_bus_addr=0xa, parent_bus_cells=0x0, parent_bus_addr=None, length_cells=0x2, length=0xb0000000c),
        edtlib.Range(node=node, child_bus_cells=0x1, child_bus_addr=0x1a, parent_bus_cells=0x0, parent_bus_addr=None, length_cells=0x2, length=0x1b0000001c),
        edtlib.Range(node=node, child_bus_cells=0x1, child_bus_addr=0x2a, parent_bus_cells=0x0, parent_bus_addr=None, length_cells=0x2, length=0x2b0000002c)
    ]

    node = edt.get_node("/ranges-two-address-cells/node@1")
    assert node.ranges == [
        edtlib.Range(node=node, child_bus_cells=0x2, child_bus_addr=0xa0000000b, parent_bus_cells=0x1, parent_bus_addr=0xc, length_cells=0x1, length=0xd),
        edtlib.Range(node=node, child_bus_cells=0x2, child_bus_addr=0x1a0000001b, parent_bus_cells=0x1, parent_bus_addr=0x1c, length_cells=0x1, length=0x1d),
        edtlib.Range(node=node, child_bus_cells=0x2, child_bus_addr=0x2a0000002b, parent_bus_cells=0x1, parent_bus_addr=0x2c, length_cells=0x1, length=0x2d)
    ]

    node = edt.get_node("/ranges-two-address-two-size-cells/node@1")
    assert node.ranges == [
        edtlib.Range(node=node, child_bus_cells=0x2, child_bus_addr=0xa0000000b, parent_bus_cells=0x1, parent_bus_addr=0xc, length_cells=0x2, length=0xd0000000e),
        edtlib.Range(node=node, child_bus_cells=0x2, child_bus_addr=0x1a0000001b, parent_bus_cells=0x1, parent_bus_addr=0x1c, length_cells=0x2, length=0x1d0000001e),
        edtlib.Range(node=node, child_bus_cells=0x2, child_bus_addr=0x2a0000002b, parent_bus_cells=0x1, parent_bus_addr=0x2c, length_cells=0x2, length=0x2d0000001d)
    ]

    node = edt.get_node("/ranges-three-address-cells/node@1")
    assert node.ranges == [
        edtlib.Range(node=node, child_bus_cells=0x3, child_bus_addr=0xa0000000b0000000c, parent_bus_cells=0x2, parent_bus_addr=0xd0000000e, length_cells=0x1, length=0xf),
        edtlib.Range(node=node, child_bus_cells=0x3, child_bus_addr=0x1a0000001b0000001c, parent_bus_cells=0x2, parent_bus_addr=0x1d0000001e, length_cells=0x1, length=0x1f),
        edtlib.Range(node=node, child_bus_cells=0x3, child_bus_addr=0x2a0000002b0000002c, parent_bus_cells=0x2, parent_bus_addr=0x2d0000002e, length_cells=0x1, length=0x2f)
    ]

    node = edt.get_node("/ranges-three-address-two-size-cells/node@1")
    assert node.ranges == [
        edtlib.Range(node=node, child_bus_cells=0x3, child_bus_addr=0xa0000000b0000000c, parent_bus_cells=0x2, parent_bus_addr=0xd0000000e, length_cells=0x2, length=0xf00000010),
        edtlib.Range(node=node, child_bus_cells=0x3, child_bus_addr=0x1a0000001b0000001c, parent_bus_cells=0x2, parent_bus_addr=0x1d0000001e, length_cells=0x2, length=0x1f00000110),
        edtlib.Range(node=node, child_bus_cells=0x3, child_bus_addr=0x2a0000002b0000002c, parent_bus_cells=0x2, parent_bus_addr=0x2d0000002e, length_cells=0x2, length=0x2f00000210)
    ]

def test_reg():
    '''Tests for the regs property'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])

    def verify_regs(node, expected_tuples):
        regs = node.regs
        assert len(regs) == len(expected_tuples)
        for reg, expected_tuple in zip(regs, expected_tuples):
            name, addr, size = expected_tuple
            assert reg.node is node
            assert reg.name == name
            assert reg.addr == addr
            assert reg.size == size

    verify_regs(edt.get_node("/reg-zero-address-cells/node"),
                [('foo', None, 0x1),
                 ('bar', None, 0x2)])

    verify_regs(edt.get_node("/reg-zero-size-cells/node"),
                [(None, 0x1, None),
                 (None, 0x2, None)])

    verify_regs(edt.get_node("/reg-ranges/parent/node"),
                [(None, 0x5, 0x1),
                 (None, 0xe0000000f, 0x1),
                 (None, 0xc0000000e, 0x1),
                 (None, 0xc0000000d, 0x1),
                 (None, 0xa0000000b, 0x1),
                 (None, 0x0, 0x1)])

    verify_regs(edt.get_node("/reg-nested-ranges/grandparent/parent/node"),
                [(None, 0x30000000200000001, 0x1)])

def test_pinctrl():
    '''Test 'pinctrl-<index>'.'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])

    node = edt.get_node("/pinctrl/dev")
    state_1 = edt.get_node('/pinctrl/pincontroller/state-1')
    state_2 = edt.get_node('/pinctrl/pincontroller/state-2')
    assert node.pinctrls == [
        edtlib.PinCtrl(node=node, name='zero', conf_nodes=[]),
        edtlib.PinCtrl(node=node, name='one', conf_nodes=[state_1]),
        edtlib.PinCtrl(node=node, name='two', conf_nodes=[state_1, state_2])
    ]

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

    binding_include = edt.get_node("/binding-include")

    assert binding_include.description == "Parent binding"

    verify_props(binding_include,
                 ['foo', 'bar', 'baz', 'qaz'],
                 ['int', 'int', 'int', 'int'],
                 [0, 1, 2, 3])

    verify_props(edt.get_node("/binding-include/child"),
                 ['foo', 'bar', 'baz', 'qaz'],
                 ['int', 'int', 'int', 'int'],
                 [0, 1, 2, 3])

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
    verify_props(child1, ['child-prop'], ['int'], [1])

    assert str(child2.binding_path) == hpath("test-bindings/child-binding.yaml")
    assert str(child2.description) == "child node"
    verify_props(child2, ['child-prop'], ['int'], [3])

    assert str(grandchild.binding_path) == hpath("test-bindings/child-binding.yaml")
    assert str(grandchild.description) == "grandchild node"
    verify_props(grandchild, ['grandchild-prop'], ['int'], [2])

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

    props_node = edt.get_node('/props')
    ctrl_1, ctrl_2 = [edt.get_node(path) for path in ['/ctrl-1', '/ctrl-2']]

    verify_props(props_node,
                 ['int',
                  'existent-boolean', 'nonexistent-boolean',
                  'array', 'uint8-array',
                  'string', 'string-array',
                  'phandle-ref', 'phandle-refs',
                  'path'],
                 ['int',
                  'boolean', 'boolean',
                  'array', 'uint8-array',
                  'string', 'string-array',
                  'phandle', 'phandles',
                  'path'],
                 [1,
                  True, False,
                  [1,2,3], b'\x124',
                  'foo', ['foo','bar','baz'],
                  ctrl_1, [ctrl_1,ctrl_2],
                  ctrl_1])

    verify_phandle_array_prop(props_node,
                              'phandle-array-foos',
                              [(ctrl_1, {'one': 1}),
                               (ctrl_2, {'one': 2, 'two': 3})])

    verify_phandle_array_prop(edt.get_node("/props-2"),
                              "phandle-array-foos",
                              [(edt.get_node('/ctrl-0-1'), {}),
                               None,
                               (edt.get_node('/ctrl-0-2'), {})])

    verify_phandle_array_prop(props_node,
                              'foo-gpios',
                              [(ctrl_1, {'gpio-one': 1})])

def test_nexus():
    '''Test <prefix>-map via gpio-map (the most common case).'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])

    source = edt.get_node("/gpio-map/source")
    destination = edt.get_node('/gpio-map/destination')
    verify_phandle_array_prop(source,
                              'foo-gpios',
                              [(destination, {'val': 6}),
                               (destination, {'val': 5})])

    assert source.props["foo-gpios"].val[0].basename == f"gpio"

def test_prop_defaults():
    '''Test property default values given in bindings'''
    with from_here():
        edt = edtlib.EDT("test.dts", ["test-bindings"])

    verify_props(edt.get_node("/defaults"),
                 ['int',
                  'array', 'uint8-array',
                  'string', 'string-array',
                  'default-not-used'],
                 ['int',
                  'array', 'uint8-array',
                  'string', 'string-array',
                  'int'],
                 [123,
                  [1,2,3], b'\x89\xab\xcd',
                  'hello', ['hello','there'],
                  234])

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
    ctrl_1 = edt.get_node('/ctrl-1')
    ctrl_2 = edt.get_node('/ctrl-2')
    zephyr_user = edt.get_node("/zephyr,user")

    verify_props(zephyr_user,
                 ['boolean', 'bytes', 'number',
                  'numbers', 'string', 'strings'],
                 ['boolean', 'uint8-array', 'int',
                  'array', 'string', 'string-array'],
                 [True, b'\x81\x82\x83', 23,
                  [1,2,3], 'text', ['a','b','c']])

    assert zephyr_user.props['handle'].val is ctrl_1

    phandles = zephyr_user.props['phandles']
    val = phandles.val
    assert len(val) == 2
    assert val[0] is ctrl_1
    assert val[1] is ctrl_2

    verify_phandle_array_prop(zephyr_user,
                              'phandle-array-foos',
                              [(edt.get_node('/ctrl-2'), {'one': 1, 'two': 2})])

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


def test_deepcopy():
    with from_here():
        # We intentionally use different kwarg values than the
        # defaults to make sure they're getting copied. This implies
        # we have to set werror=True, so we can't use test.dts, since
        # that generates warnings on purpose.
        edt = edtlib.EDT("test-multidir.dts",
                         ["test-bindings", "test-bindings-2"],
                         warn_reg_unit_address_mismatch=False,
                         default_prop_types=False,
                         support_fixed_partitions_on_any_bus=False,
                         infer_binding_for_paths=['/test-node'],
                         vendor_prefixes={'test-vnd': 'A test vendor'},
                         werror=True)
        edt_copy = deepcopy(edt)

    def equal_paths(list1, list2):
        assert len(list1) == len(list2)
        return all(elt1.path == elt2.path for elt1, elt2 in zip(list1, list2))

    def equal_key2path(key2node1, key2node2):
        assert len(key2node1) == len(key2node2)
        return (all(key1 == key2 for (key1, key2) in
                    zip(key2node1, key2node2)) and
                all(node1.path == node2.path for (node1, node2) in
                    zip(key2node1.values(), key2node2.values())))

    def equal_key2paths(key2nodes1, key2nodes2):
        assert len(key2nodes1) == len(key2nodes2)
        return (all(key1 == key2 for (key1, key2) in
                    zip(key2nodes1, key2nodes2)) and
                all(equal_paths(nodes1, nodes2) for (nodes1, nodes2) in
                    zip(key2nodes1.values(), key2nodes2.values())))

    def test_equal_but_not_same(attribute, equal=None):
        if equal is None:
            equal = lambda a, b: a == b
        copy = getattr(edt_copy, attribute)
        original = getattr(edt, attribute)
        assert equal(copy, original)
        assert copy is not original

    test_equal_but_not_same("nodes", equal_paths)
    test_equal_but_not_same("compat2nodes", equal_key2paths)
    test_equal_but_not_same("compat2okay", equal_key2paths)
    test_equal_but_not_same("compat2vendor")
    test_equal_but_not_same("compat2model")
    test_equal_but_not_same("label2node", equal_key2path)
    test_equal_but_not_same("dep_ord2node", equal_key2path)
    assert edt_copy.dts_path == "test-multidir.dts"
    assert edt_copy.bindings_dirs == ["test-bindings", "test-bindings-2"]
    assert edt_copy.bindings_dirs is not edt.bindings_dirs
    assert not edt_copy._warn_reg_unit_address_mismatch
    assert not edt_copy._default_prop_types
    assert not edt_copy._fixed_partitions_no_bus
    assert edt_copy._infer_binding_for_paths == set(["/test-node"])
    assert edt_copy._infer_binding_for_paths is not edt._infer_binding_for_paths
    assert edt_copy._vendor_prefixes == {"test-vnd": "A test vendor"}
    assert edt_copy._vendor_prefixes is not edt._vendor_prefixes
    assert edt_copy._werror
    test_equal_but_not_same("_compat2binding", equal_key2path)
    test_equal_but_not_same("_binding_paths")
    test_equal_but_not_same("_binding_fname2path")
    assert len(edt_copy._node2enode) == len(edt._node2enode)
    for node1, node2 in zip(edt_copy._node2enode, edt._node2enode):
        enode1 = edt_copy._node2enode[node1]
        enode2 = edt._node2enode[node2]
        assert node1.path == node2.path
        assert enode1.path == enode2.path
        assert node1 is not node2
        assert enode1 is not enode2
    assert edt_copy._dt is not edt._dt


def verify_error(dts, dts_file, expected_err):
    # Verifies that parsing a file 'dts_file' with the contents 'dts'
    # (a string) raises an EDTError with the message 'expected_err'.
    #
    # The path 'dts_file' is written with the string 'dts' before the
    # test is run.

    with open(dts_file, "w", encoding="utf-8") as f:
        f.write(dts)
        f.flush()  # Can't have unbuffered text IO, so flush() instead

    with pytest.raises(edtlib.EDTError) as e:
        edtlib.EDT(dts_file, [])

    assert str(e.value) == expected_err


def verify_props(node, names, types, values):
    # Verifies that each property in 'names' has the expected
    # value in 'values'. Property lookup is done in Node 'node'.

    for name, type, value in zip(names, types, values):
        prop = node.props[name]
        assert prop.name == name
        assert prop.type == type
        assert prop.val == value
        assert prop.node is node

def verify_phandle_array_prop(node, name, values):
    # Verifies 'node.props[name]' is a phandle-array, and has the
    # expected controller/data values in 'values'. Elements
    # of 'values' may be None.

    prop = node.props[name]
    assert prop.type == 'phandle-array'
    assert prop.name == name
    val = prop.val
    assert isinstance(val, list)
    assert len(val) == len(values)
    for actual, expected in zip(val, values):
        if expected is not None:
            controller, data = expected
            assert isinstance(actual, edtlib.ControllerAndData)
            assert actual.controller is controller
            assert actual.data == data
        else:
            assert actual is None
