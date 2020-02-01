#!/usr/bin/env python3

# Copyright (c) 2019 Nordic Semiconductor ASA
# Copyright (c) 2020 Bobby Noelte
# SPDX-License-Identifier: BSD-3-Clause

import io
import os
import sys

import edtlib

# Test suite for edtlib.py. Run it directly as an executable, in this
# directory:
#
#   $ ./testedtlib.py
#
# test.dts is the main test file. test-bindings/ and test-bindings-2/ has
# bindings. The tests mostly use string comparisons via the various __repr__()
# methods.


def run():
    """
    Runs all edtlib tests. Immediately exits with status 1 and a message on
    stderr on test suite failures.
    """

    warnings = io.StringIO()
    edt = edtlib.EDT("test.dts", ["test-bindings"], warnings)

    # Deprecated features are tested too, which generate warnings. Verify them.
    verify_eq(warnings.getvalue(), """\
warning: The 'properties: compatible: constraint: ...' way of specifying the compatible in test-bindings/deprecated.yaml is deprecated. Put 'compatible: "deprecated"' at the top level of the binding instead.
warning: the 'inherits:' syntax in test-bindings/deprecated.yaml is deprecated and will be removed - please use 'include: foo.yaml' or 'include: [foo.yaml, bar.yaml]' instead
warning: 'title:' in test-bindings/deprecated.yaml is deprecated and will be removed (and was never used). Just put a 'description:' that describes the device instead. Use other bindings as a reference, and note that all bindings were updated recently. Think about what information would be useful to other people (e.g. explanations of acronyms, or datasheet links), and put that in as well. The description text shows up as a comment in the generated header. See yaml-multiline.info for how to deal with multiple lines. You probably want 'description: |'.
warning: please put 'required: true' instead of 'category: required' in properties: required: ...' in test-bindings/deprecated.yaml - 'category' will be removed
warning: please put 'required: false' instead of 'category: optional' in properties: optional: ...' in test-bindings/deprecated.yaml - 'category' will be removed
warning: 'sub-node: properties: ...' in test-bindings/deprecated.yaml is deprecated and will be removed - please give a full binding for the child node in 'child-binding:' instead (see binding-template.yaml)
warning: "#cells:" in test-bindings/deprecated.yaml is deprecated and will be removed - please put 'interrupt-cells:', 'pwm-cells:', 'gpio-cells:', etc., instead. The name should match the name of the corresponding phandle-array property (see binding-template.yaml)
""")

    #
    # Test clocks
    #

    verify_streq(edt.get_node("/clock-test/clock-consumer").clocks,
                 "[<ControllerAndData, name: clkin-0, controller: <Node /clock-test/clock-provider in 'test.dts', binding test-bindings/clock-controller.yaml>, data: OrderedDict([('clock-id', 2)])>, <ControllerAndData, name: clkin-1, controller: <Node /clock-test/clock-provider in 'test.dts', binding test-bindings/clock-controller.yaml>, data: OrderedDict([('clock-id', 1)])>]")

    verify_streq(edt.get_node("/clock-test/clock-provider").clock_outputs,
                 "[<ControllerAndData, name: clkout-0, controller: <Node /clock-test/clock-provider in 'test.dts', binding test-bindings/clock-controller.yaml>, data: {}>, <ControllerAndData, name: clkout-1, controller: <Node /clock-test/clock-provider in 'test.dts', binding test-bindings/clock-controller.yaml>, data: {}>, <ControllerAndData, name: clkout-2, controller: <Node /clock-test/clock-provider in 'test.dts', binding test-bindings/clock-controller.yaml>, data: {}>]")

    #
    # Test gio leds
    #

    verify_streq(edt.get_node("/gpio-leds-test/leds-controller").gpio_leds,
                 "[<ControllerAndData, name: led0, controller: <Node /gpio-leds-test/leds-controller in 'test.dts', binding test-bindings/gpio-leds.yaml>, data: {'gpios': [<ControllerAndData, controller: <Node /gpio-leds-test/gpio-0 in 'test.dts', binding test-bindings/gpio-2-cell.yaml>, data: OrderedDict([('gpio_cell_one', 0), ('gpio_cell_two', 1)])>], 'function': 1, 'color': 99, 'default-state': 1, 'led-pattern': [255, 1000, 0, 1000], 'retain-state-suspended': False, 'retain-state-shutdown': False, 'panic-indicator': False}>, <ControllerAndData, name: led1, controller: <Node /gpio-leds-test/leds-controller in 'test.dts', binding test-bindings/gpio-leds.yaml>, data: {'gpios': [<ControllerAndData, controller: <Node /gpio-leds-test/gpio-0 in 'test.dts', binding test-bindings/gpio-2-cell.yaml>, data: OrderedDict([('gpio_cell_one', 1), ('gpio_cell_two', 0)])>], 'function': 2, 'color': 77, 'default-state': 1, 'led-pattern': [255, 2000, 0, 2000], 'retain-state-suspended': False, 'retain-state-shutdown': False, 'panic-indicator': False}>]")

    verify_streq(edt.get_node("/gpio-leds-test/gpio-0").gpio_leds,
                 "[]")

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
    # Test partitions
    #

    verify_streq(edt.get_node("/partition/flash-controller/flash-0").partitions,
                 "[<Partition, name: flash-0 partition-0, flash: <Node /partition/flash-controller/flash-0 in 'test.dts', binding test-bindings/soc-nv-flash.yaml>, controller: <Node /partition/flash-controller in 'test.dts', no binding>, partitions: [], addr: 0, size: 100, attributes: {'read-only': False}>, <Partition, name: flash-0 partition-1, flash: <Node /partition/flash-controller/flash-0 in 'test.dts', binding test-bindings/soc-nv-flash.yaml>, controller: <Node /partition/flash-controller in 'test.dts', no binding>, partitions: [], addr: 100, size: 100, attributes: {'read-only': False}>]")

    verify_streq(edt.get_node("/partition/flash-controller/flash-0/partitions/partition-0-0").flash_controller,
                 "<Node /partition/flash-controller in 'test.dts', no binding>")

    verify_streq(edt.get_node("/partition/flash-controller").flash_controller,
                 "None")

    verify_streq(edt.get_node("/partition/flash-1").partitions,
                 "[<Partition, name: flash-1 partition-0, flash: <Node /partition/flash-1 in 'test.dts', no binding>, controller: <Node /partition/flash-1 in 'test.dts', no binding>, partitions: [], addr: 0, size: 1000, attributes: {'read-only': False}>, <Partition, name: partition-replace-label, flash: <Node /partition/flash-1 in 'test.dts', no binding>, controller: <Node /partition/flash-1 in 'test.dts', no binding>, partitions: [], addr: 1000, size: 1000, attributes: {'read-only': False}>]")

    verify_streq(edt.get_node("/partition/flash-1/partitions/partition-1-0").flash_controller,
                 "<Node /partition/flash-1 in 'test.dts', no binding>")

    #
    # Test 'pinctrl-<index>'
    #

    verify_streq(edt.get_node("/pinctrl/dev").pinctrls,
                 "[<PinCtrl, name: zero, configuration nodes: []>, <PinCtrl, name: one, configuration nodes: [<Node /pinctrl/pincontroller/state-1 in 'test.dts', binding test-bindings/pinctrl-state.yaml>]>, <PinCtrl, name: two, configuration nodes: [<Node /pinctrl/pincontroller/state-1 in 'test.dts', binding test-bindings/pinctrl-state.yaml>, <Node /pinctrl/pincontroller/state-2 in 'test.dts', binding test-bindings/pinctrl-state.yaml>]>]")

    #
    # Test pinctrl states of pin controller node (and other)
    #

    verify_streq(edt.get_node("/pinctrl/pincontroller").pinctrl_states,
                 "[<Node /pinctrl/pincontroller/state-1 in 'test.dts', binding test-bindings/pinctrl-state.yaml>, <Node /pinctrl/pincontroller/state-2 in 'test.dts', binding test-bindings/pinctrl-state.yaml>]")

    verify_streq(edt.get_node("/pinctrl/dev").pinctrl_states,
                 "[]")

    #
    # Test pinctrl states of pin control gpio ranges node (and other)
    #

    verify_streq(edt.get_node("/pinctrl/pincontroller").pinctrl_gpio_ranges,
                 "[<ControllerAndData, controller: <Node /pinctrl/pincontroller/gpio-0 in 'test.dts', binding test-bindings/pinctrl-gpio.yaml>, data: OrderedDict([('gpio-base', 0), ('pinctrl-base', 0), ('count', 16)])>, <ControllerAndData, controller: <Node /pinctrl/pincontroller/gpio-1 in 'test.dts', binding test-bindings/pinctrl-gpio.yaml>, data: OrderedDict([('gpio-base', 0), ('pinctrl-base', 16), ('count', 16)])>]")

    verify_streq(edt.get_node("/pinctrl/pincontroller/gpio-0").pinctrl_gpio_ranges,
                 "[]")

    verify_streq(edt.get_node("/pinctrl/dev").pinctrl_gpio_ranges,
                 "[]")

    #
    # Test pin controller of pin control state node
    #

    verify_streq(edt.get_node("/pinctrl/pincontroller/state-1").pin_controller,
                 "<Node /pinctrl/pincontroller in 'test.dts', no binding>")

    #
    # Test pincfgs of pinctrl state node (and other)
    #

    verify_streq(edt.get_node("/pinctrl/pincontroller/state-1").pincfgs,
                 "[<PinCfg, name: pincfg-1-1, groups: [], pins: [0], muxes: [1], function: None, configs: {'bias-disable': False, 'bias-high-impedance': False, 'bias-bus-hold': False, 'drive-push-pull': False, 'drive-open-drain': False, 'drive-open-source': False, 'input-enable': False, 'input-disable': False, 'input-schmitt-enable': False, 'input-schmitt-disable': False, 'low-power-enable': False, 'low-power-disable': False, 'output-disable': False, 'output-enable': False, 'output-low': False, 'output-high': False, 'power-source': 0, 'slew-rate': 0, 'skew-delay': 0}>, <PinCfg, name: pincfg-1-2, groups: [], pins: [1], muxes: [2], function: None, configs: {'bias-disable': False, 'bias-high-impedance': False, 'bias-bus-hold': False, 'drive-push-pull': False, 'drive-open-drain': False, 'drive-open-source': False, 'input-enable': False, 'input-disable': False, 'input-schmitt-enable': False, 'input-schmitt-disable': False, 'low-power-enable': False, 'low-power-disable': False, 'output-disable': False, 'output-enable': False, 'output-low': False, 'output-high': False, 'power-source': 0, 'slew-rate': 0, 'skew-delay': 0}>]")

    verify_streq(edt.get_node("/pinctrl/pincontroller/state-2").pincfgs,
                 "[<PinCfg, name: pincfg-2-1, groups: ['group'], pins: [], muxes: [], function: None, configs: {'bias-disable': False, 'bias-high-impedance': False, 'bias-bus-hold': False, 'drive-push-pull': False, 'drive-open-drain': False, 'drive-open-source': False, 'input-enable': False, 'input-disable': False, 'input-schmitt-enable': False, 'input-schmitt-disable': False, 'low-power-enable': False, 'low-power-disable': False, 'output-disable': False, 'output-enable': False, 'output-low': False, 'output-high': False, 'power-source': 0, 'slew-rate': 0, 'skew-delay': 0}>]")

    verify_streq(edt.get_node("/pinctrl/pincontroller").pincfgs,
                 "[]")

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
    # Test 'bus:' and 'on-bus:'
    #

    verify_eq(edt.get_node("/buses/foo-bus").bus, "foo")
    # foo-bus does not itself appear on a bus
    verify_eq(edt.get_node("/buses/foo-bus").on_bus, None)
    verify_eq(edt.get_node("/buses/foo-bus").bus_node, None)

    # foo-bus/node is not a bus node...
    verify_eq(edt.get_node("/buses/foo-bus/node").bus, None)
    # ...but is on a bus
    verify_eq(edt.get_node("/buses/foo-bus/node").on_bus, "foo")
    verify_eq(edt.get_node("/buses/foo-bus/node").bus_node.path,
                           "/buses/foo-bus")

    # Same compatible string, but different bindings from being on different
    # buses
    verify_streq(edt.get_node("/buses/foo-bus/node").binding_path,
                 "test-bindings/device-on-foo-bus.yaml")
    verify_streq(edt.get_node("/buses/bar-bus/node").binding_path,
                 "test-bindings/device-on-bar-bus.yaml")

    # foo-bus/node/nested also appears on the foo-bus bus
    verify_eq(edt.get_node("/buses/foo-bus/node/nested").on_bus, "foo")
    verify_streq(edt.get_node("/buses/foo-bus/node/nested").binding_path,
                 "test-bindings/device-on-foo-bus.yaml")

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
    # Test EDT.compat2enabled
    #

    verify_streq(edt.compat2enabled["compat2enabled"], "[<Node /compat2enabled/foo-1 in 'test.dts', no binding>, <Node /compat2enabled/foo-2 in 'test.dts', no binding>]")

    if "compat2enabled-disabled" in edt.compat2enabled:
        fail("'compat2enabled-disabled' should not appear in edt.compat2enabled")

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

    verify_streq(edt.get_node("/props").props["path"],
                 "<Property, name: path, type: path, value: <Node /props/ctrl-1 in 'test.dts', binding test-bindings/phandle-array-controller-1.yaml>>")

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
    warnings = io.StringIO()
    edt = edtlib.EDT("test-multidir.dts", ["test-bindings", "test-bindings-2"], warnings)

    verify_eq(warnings.getvalue(), """\
warning: multiple candidates for binding file 'multidir.yaml': skipping 'test-bindings-2/multidir.yaml' and using 'test-bindings/multidir.yaml'
""")

    verify_streq(edt.get_node("/in-dir-1").binding_path,
                 "test-bindings/multidir.yaml")

    verify_streq(edt.get_node("/in-dir-2").binding_path,
                 "test-bindings/multidir.yaml")

    warnings = io.StringIO()
    edt = edtlib.EDT("test-multidir.dts", ["test-bindings-2", "test-bindings"], warnings)

    verify_eq(warnings.getvalue(), """\
warning: multiple candidates for binding file 'multidir.yaml': skipping 'test-bindings/multidir.yaml' and using 'test-bindings-2/multidir.yaml'
""")

    verify_streq(edt.get_node("/in-dir-1").binding_path,
                 "test-bindings-2/multidir.yaml")

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

    #
    # Test error messages from chosen binding
    #

    verify_error("""
/dts-v1/;

/ {
	chosen {
		error = "error out";
	};
};
""", "'error' appears in /chosen in error.dts, but is not declared in 'properties:' in test-bindings/chosen.yaml",
    ["test-bindings"])

    #
    # Test error messages from _slice()
    #

    verify_error("""
/dts-v1/;

/ {
	#address-cells = <1>;
	#size-cells = <2>;

	sub {
		reg = <3>;
	};
};
""", "'reg' property in <Node /sub in 'error.dts'> has length 4, which is not evenly divisible by 12 (= 4*(<#address-cells> (= 1) + <#size-cells> (= 2))). Note that #*-cells properties come either from the parent node or from the controller (in the case of 'interrupts').")

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
""", "'interrupts' property in <Node /sub in 'error.dts'> has length 4, which is not evenly divisible by 8 (= 4*<#interrupt-cells>). Note that #*-cells properties come either from the parent node or from the controller (in the case of 'interrupts').")

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
""", "'ranges' property in <Node /sub-1 in 'error.dts'> has length 8, which is not evenly divisible by 24 (= 4*(<#address-cells> (= 2) + <#address-cells for parent> (= 1) + <#size-cells> (= 3))). Note that #*-cells properties come either from the parent node or from the controller (in the case of 'interrupts').")

    print("all tests passed")


def verify_error(dts, error, bindings = []):
    # Verifies that parsing a file with the contents 'dts' (a string) raises an
    # EDTError with the message 'error'

    # Could use the 'tempfile' module instead of 'error.dts', but having a
    # consistent filename makes error messages consistent and easier to test.
    # error.dts is kept if the test fails, which is helpful.

    with open("error.dts", "w", encoding="utf-8") as f:
        f.write(dts)
        f.flush()  # Can't have unbuffered text IO, so flush() instead
        try:
            edtlib.EDT("error.dts", bindings)
        except edtlib.EDTError as e:
            if str(e) != error:
                fail(f"expected the EDTError '{error}', got the EDTError '{e}'")
        except Exception as e:
            fail(f"expected the EDTError '{error}', got the {type(e).__name__} '{e}'")
        else:
            fail(f"expected the error '{error}', got no error")

    os.remove("error.dts")


def fail(msg):
    sys.exit("test failed: " + msg)


def verify_eq(actual, expected):
    if actual != expected:
        # Put values on separate lines to make it easy to spot differences
        fail("not equal (expected value last):\n'{}'\n'{}'"
             .format(actual, expected))


def verify_streq(actual, expected):
    verify_eq(str(actual), expected)


if __name__ == "__main__":
    run()
