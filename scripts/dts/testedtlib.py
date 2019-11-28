#!/usr/bin/env python3

# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: BSD-3-Clause

import io
import sys

import edtlib

# Test suite for edtlib.py. Run it directly as an executable, in this
# directory:
#
#   $ ./testedtlib.py
#
# test.dts is the test file. test-bindings/ has bindings. The tests mostly use
# string comparisons via the various __repr__() methods.


def run():
    """
    Runs all edtlib tests. Immediately exits with status 1 and a message on
    stderr on test suite failures.
    """

    def fail(msg):
        sys.exit("test failed: " + msg)

    def verify_eq(actual, expected):
        if actual != expected:
            # Put values on separate lines to make it easy to spot differences
            fail("not equal (expected value last):\n'{}'\n'{}'"
                 .format(actual, expected))

    def verify_streq(actual, expected):
        verify_eq(str(actual), expected)

    warnings = io.StringIO()
    edt = edtlib.EDT("test.dts", ["test-bindings"], warnings)

    # Deprecated features are tested too, which generate warnings. Verify them.
    verify_eq(warnings.getvalue(), """\
warning: The 'properties: compatible: constraint: ...' way of specifying the compatible in test-bindings/deprecated.yaml is deprecated. Put 'compatible: "deprecated"' at the top level of the binding instead.
warning: the 'inherits:' syntax in test-bindings/deprecated.yaml is deprecated and will be removed - please use 'include: foo.yaml' or 'include: [foo.yaml, bar.yaml]' instead
warning: please put 'required: true' instead of 'category: required' in properties: required: ...' in test-bindings/deprecated.yaml - 'category' will be removed
warning: please put 'required: false' instead of 'category: optional' in properties: optional: ...' in test-bindings/deprecated.yaml - 'category' will be removed
warning: 'sub-node: properties: ...' in test-bindings/deprecated.yaml is deprecated and will be removed - please give a full binding for the child node in 'child-binding:' instead (see binding-template.yaml)
warning: "#cells:" in test-bindings/deprecated.yaml is deprecated and will be removed - please put 'interrupt-cells:', 'pwm-cells:', 'gpio-cells:', etc., instead. The name should match the name of the corresponding phandle-array property (see binding-template.yaml)
""")

    #
    # Test interrupts
    #

    verify_streq(edt.get_node("/interrupt-parent-test/node").interrupts,
                 "[<ControllerAndData, name: foo, controller: <Node /interrupt-parent-test/controller in 'test.dts', binding test-bindings/interrupt-3-cell.yaml>, data: OrderedDict([('one', 1), ('two', 2), ('three', 3)])>, <ControllerAndData, name: bar, controller: <Node /interrupt-parent-test/controller in 'test.dts', binding test-bindings/interrupt-3-cell.yaml>, data: OrderedDict([('one', 4), ('two', 5), ('three', 6)])>]")

    verify_streq(edt.get_node("/interrupts-extended-test/node").interrupts,
                 "[<ControllerAndData, controller: <Node /interrupts-extended-test/controller-0 in 'test.dts', binding test-bindings/interrupt-1-cell.yaml>, data: OrderedDict([('one', 1)])>, <ControllerAndData, controller: <Node /interrupts-extended-test/controller-1 in 'test.dts', binding test-bindings/interrupt-2-cell.yaml>, data: OrderedDict([('one', 2), ('two', 3)])>, <ControllerAndData, controller: <Node /interrupts-extended-test/controller-2 in 'test.dts', binding test-bindings/interrupt-3-cell.yaml>, data: OrderedDict([('one', 4), ('two', 5), ('three', 6)])>]")

    verify_streq(edt.get_node("/interrupt-map-test/node@0").interrupts,
                 "[<ControllerAndData, controller: <Node /interrupt-map-test/controller-0 in 'test.dts', binding test-bindings/interrupt-1-cell.yaml>, data: OrderedDict([('one', 0)])>, <ControllerAndData, controller: <Node /interrupt-map-test/controller-1 in 'test.dts', binding test-bindings/interrupt-2-cell.yaml>, data: OrderedDict([('one', 0), ('two', 1)])>, <ControllerAndData, controller: <Node /interrupt-map-test/controller-2 in 'test.dts', binding test-bindings/interrupt-3-cell.yaml>, data: OrderedDict([('one', 0), ('two', 0), ('three', 2)])>]")

    verify_streq(edt.get_node("/interrupt-map-test/node@1").interrupts,
                 "[<ControllerAndData, controller: <Node /interrupt-map-test/controller-0 in 'test.dts', binding test-bindings/interrupt-1-cell.yaml>, data: OrderedDict([('one', 3)])>, <ControllerAndData, controller: <Node /interrupt-map-test/controller-1 in 'test.dts', binding test-bindings/interrupt-2-cell.yaml>, data: OrderedDict([('one', 0), ('two', 4)])>, <ControllerAndData, controller: <Node /interrupt-map-test/controller-2 in 'test.dts', binding test-bindings/interrupt-3-cell.yaml>, data: OrderedDict([('one', 0), ('two', 0), ('three', 5)])>]")

    verify_streq(edt.get_node("/interrupt-map-bitops-test/node@70000000E").interrupts,
                 "[<ControllerAndData, controller: <Node /interrupt-map-bitops-test/controller in 'test.dts', binding test-bindings/interrupt-2-cell.yaml>, data: OrderedDict([('one', 3), ('two', 2)])>]")

    #
    # Test 'reg'
    #

    verify_streq(edt.get_node("/reg-zero-address-cells/node").regs,
                 "[<Register, addr: 0x0, size: 0x1>, <Register, addr: 0x0, size: 0x2>]")

    verify_streq(edt.get_node("/reg-zero-size-cells/node").regs,
                 "[<Register, addr: 0x1, size: 0x0>, <Register, addr: 0x2, size: 0x0>]")

    verify_streq(edt.get_node("/reg-ranges/parent/node").regs,
                 "[<Register, addr: 0x5, size: 0x1>, <Register, addr: 0xe0000000f, size: 0x1>, <Register, addr: 0xc0000000e, size: 0x1>, <Register, addr: 0xc0000000d, size: 0x1>, <Register, addr: 0xa0000000b, size: 0x1>, <Register, addr: 0x0, size: 0x1>]")

    verify_streq(edt.get_node("/reg-nested-ranges/grandparent/parent/node").regs,
                 "[<Register, addr: 0x30000000200000001, size: 0x1>]")

    #
    # Test 'pinctrl-<index>'
    #

    verify_streq(edt.get_node("/pinctrl/dev").pinctrls,
                 "[<PinCtrl, name: zero, configuration nodes: []>, <PinCtrl, name: one, configuration nodes: [<Node /pinctrl/pincontroller/state-1 in 'test.dts', no binding>]>, <PinCtrl, name: two, configuration nodes: [<Node /pinctrl/pincontroller/state-1 in 'test.dts', no binding>, <Node /pinctrl/pincontroller/state-2 in 'test.dts', no binding>]>]")

    #
    # Test Node.parent and Node.children
    #

    verify_eq(edt.get_node("/").parent, None)

    verify_streq(edt.get_node("/parent/child-1").parent,
                 "<Node /parent in 'test.dts', no binding>")

    verify_streq(edt.get_node("/parent/child-2/grandchild").parent,
                 "<Node /parent/child-2 in 'test.dts', no binding>")

    verify_streq(edt.get_node("/parent").children,
                 "OrderedDict([('child-1', <Node /parent/child-1 in 'test.dts', no binding>), ('child-2', <Node /parent/child-2 in 'test.dts', no binding>)])")

    verify_eq(edt.get_node("/parent/child-1").children, {})

    #
    # Test 'include:' and the legacy 'inherits: !include ...'
    #

    verify_streq(edt.get_node("/binding-include").description,
                 "Parent binding")

    verify_streq(edt.get_node("/binding-include").props,
                 "OrderedDict([('foo', <Property, name: foo, type: int, value: 0>), ('bar', <Property, name: bar, type: int, value: 1>), ('baz', <Property, name: baz, type: int, value: 2>), ('qaz', <Property, name: qaz, type: int, value: 3>)])")

    #
    # Test 'child/parent-bus:'
    #

    verify_streq(edt.get_node("/buses/foo-bus/node").binding_path,
                 "test-bindings/device-on-foo-bus.yaml")

    verify_streq(edt.get_node("/buses/bar-bus/node").binding_path,
                 "test-bindings/device-on-bar-bus.yaml")

    #
    # Test 'child-binding:'
    #

    child1 = edt.get_node("/child-binding/child-1")
    child2 = edt.get_node("/child-binding/child-2")
    grandchild = edt.get_node("/child-binding/child-1/grandchild")

    verify_streq(child1.binding_path, "test-bindings/child-binding.yaml")
    verify_streq(child1.description, "child node")
    verify_streq(child1.props, "OrderedDict([('child-prop', <Property, name: child-prop, type: int, value: 1>)])")

    verify_streq(child2.binding_path, "test-bindings/child-binding.yaml")
    verify_streq(child2.description, "child node")
    verify_streq(child2.props, "OrderedDict([('child-prop', <Property, name: child-prop, type: int, value: 3>)])")

    verify_streq(grandchild.binding_path, "test-bindings/child-binding.yaml")
    verify_streq(grandchild.description, "grandchild node")
    verify_streq(grandchild.props, "OrderedDict([('grandchild-prop', <Property, name: grandchild-prop, type: int, value: 2>)])")

    #
    # Test deprecated 'sub-node' key (replaced with 'child-binding') and
    # deprecated '#cells' key (replaced with '*-cells')
    #

    verify_streq(edt.get_node("/deprecated/sub-node").props,
                 "OrderedDict([('foos', <Property, name: foos, type: phandle-array, value: [<ControllerAndData, controller: <Node /deprecated in 'test.dts', binding test-bindings/deprecated.yaml>, data: OrderedDict([('foo', 1), ('bar', 2)])>]>)])")

    #
    # Test Node.props (derived from DT and 'properties:' in the binding)
    #

    verify_streq(edt.get_node("/props").props["int"],
                 "<Property, name: int, type: int, value: 1>")

    verify_streq(edt.get_node("/props").props["existent-boolean"],
                 "<Property, name: existent-boolean, type: boolean, value: True>")

    verify_streq(edt.get_node("/props").props["nonexistent-boolean"],
                 "<Property, name: nonexistent-boolean, type: boolean, value: False>")

    verify_streq(edt.get_node("/props").props["array"],
                 "<Property, name: array, type: array, value: [1, 2, 3]>")

    verify_streq(edt.get_node("/props").props["uint8-array"],
                 r"<Property, name: uint8-array, type: uint8-array, value: b'\x124'>")

    verify_streq(edt.get_node("/props").props["string"],
                 "<Property, name: string, type: string, value: 'foo'>")

    verify_streq(edt.get_node("/props").props["string-array"],
                 "<Property, name: string-array, type: string-array, value: ['foo', 'bar', 'baz']>")

    verify_streq(edt.get_node("/props").props["phandle-ref"],
                 "<Property, name: phandle-ref, type: phandle, value: <Node /props/ctrl-1 in 'test.dts', binding test-bindings/phandle-array-controller-1.yaml>>")

    verify_streq(edt.get_node("/props").props["phandle-refs"],
                 "<Property, name: phandle-refs, type: phandles, value: [<Node /props/ctrl-1 in 'test.dts', binding test-bindings/phandle-array-controller-1.yaml>, <Node /props/ctrl-2 in 'test.dts', binding test-bindings/phandle-array-controller-2.yaml>]>")

    verify_streq(edt.get_node("/props").props["phandle-array-foos"],
                 "<Property, name: phandle-array-foos, type: phandle-array, value: [<ControllerAndData, controller: <Node /props/ctrl-1 in 'test.dts', binding test-bindings/phandle-array-controller-1.yaml>, data: OrderedDict([('one', 1)])>, <ControllerAndData, controller: <Node /props/ctrl-2 in 'test.dts', binding test-bindings/phandle-array-controller-2.yaml>, data: OrderedDict([('one', 2), ('two', 3)])>]>")

    verify_streq(edt.get_node("/props").props["foo-gpios"],
                 "<Property, name: foo-gpios, type: phandle-array, value: [<ControllerAndData, controller: <Node /props/ctrl-1 in 'test.dts', binding test-bindings/phandle-array-controller-1.yaml>, data: OrderedDict([('gpio-one', 1)])>]>")

    #
    # Test <prefix>-map, via gpio-map (the most common case)
    #

    verify_streq(edt.get_node("/gpio-map/source").props["foo-gpios"],
                 "<Property, name: foo-gpios, type: phandle-array, value: [<ControllerAndData, controller: <Node /gpio-map/destination in 'test.dts', binding test-bindings/gpio-dst.yaml>, data: OrderedDict([('val', 6)])>, <ControllerAndData, controller: <Node /gpio-map/destination in 'test.dts', binding test-bindings/gpio-dst.yaml>, data: OrderedDict([('val', 5)])>]>")

    #
    # Test property default values given in bindings
    #

    verify_streq(edt.get_node("/defaults").props,
                 r"OrderedDict([('int', <Property, name: int, type: int, value: 123>), ('array', <Property, name: array, type: array, value: [1, 2, 3]>), ('uint8-array', <Property, name: uint8-array, type: uint8-array, value: b'\x89\xab\xcd'>), ('string', <Property, name: string, type: string, value: 'hello'>), ('string-array', <Property, name: string-array, type: string-array, value: ['hello', 'there']>), ('default-not-used', <Property, name: default-not-used, type: int, value: 234>)])")

    #
    # Test having multiple directories with bindings, with a different .dts file
    #

    edt = edtlib.EDT("test-multidir.dts", ["test-bindings", "test-bindings-2"])

    verify_streq(edt.get_node("/in-dir-1").binding_path,
                 "test-bindings/multidir.yaml")

    verify_streq(edt.get_node("/in-dir-2").binding_path,
                 "test-bindings-2/multidir.yaml")


    #
    # Test dependency relations
    #

    verify_eq(edt.get_node("/").dep_ordinal, 0)
    verify_eq(edt.get_node("/in-dir-1").dep_ordinal, 1)
    if edt.get_node("/") not in edt.get_node("/in-dir-1").depends_on:
        fail("/ should be a direct dependency of /in-dir-1")
    if edt.get_node("/in-dir-1") not in edt.get_node("/").required_by:
        fail("/in-dir-1 should directly depend on /")

    print("all tests passed")


if __name__ == "__main__":
    run()
