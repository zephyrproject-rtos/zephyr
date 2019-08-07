#!/usr/bin/env python3

# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: BSD-3-Clause

import edtlib

# Test suite for edtlib.py. Mostly uses string comparisons via the various
# __repr__() methods. Can be run directly as an executable.
#
# This script expects to be run from the directory its in. This simplifies
# things, as paths in the output can be assumed below.
#
# test.dts is the test file, and test-bindings/ has bindings.


def run():
    """
    Runs all edtlib tests. Immediately exits with status 1 and a message on
    stderr on test suite failures.
    """

    def fail(msg):
        raise Exception("test failed: " + msg)

    def verify_streq(actual, expected):
        actual = str(actual)
        if actual != expected:
            # Put values on separate lines to make it easy to spot differences
            fail("not equal (expected value last):\n'{}'\n'{}'"
                 .format(actual, expected))

    edt = edtlib.EDT("test.dts", ["test-bindings"])

    #
    # Test interrupts
    #

    verify_streq(edt.get_dev("/interrupt-parent-test/node").interrupts,
                 "[<Interrupt, name: foo, target: <Device /interrupt-parent-test/controller in 'test.dts', binding test-bindings/interrupt-3-cell.yaml>, specifier: {'one': 1, 'two': 2, 'three': 3}>, <Interrupt, name: bar, target: <Device /interrupt-parent-test/controller in 'test.dts', binding test-bindings/interrupt-3-cell.yaml>, specifier: {'one': 4, 'two': 5, 'three': 6}>]")

    verify_streq(edt.get_dev("/interrupts-extended-test/node").interrupts,
                 "[<Interrupt, target: <Device /interrupts-extended-test/controller-0 in 'test.dts', binding test-bindings/interrupt-1-cell.yaml>, specifier: {'one': 1}>, <Interrupt, target: <Device /interrupts-extended-test/controller-1 in 'test.dts', binding test-bindings/interrupt-2-cell.yaml>, specifier: {'one': 2, 'two': 3}>, <Interrupt, target: <Device /interrupts-extended-test/controller-2 in 'test.dts', binding test-bindings/interrupt-3-cell.yaml>, specifier: {'one': 4, 'two': 5, 'three': 6}>]")

    verify_streq(edt.get_dev("/interrupt-map-test/node@0").interrupts,
                 "[<Interrupt, target: <Device /interrupt-map-test/controller-0 in 'test.dts', binding test-bindings/interrupt-1-cell.yaml>, specifier: {'one': 0}>, <Interrupt, target: <Device /interrupt-map-test/controller-1 in 'test.dts', binding test-bindings/interrupt-2-cell.yaml>, specifier: {'one': 0, 'two': 1}>, <Interrupt, target: <Device /interrupt-map-test/controller-2 in 'test.dts', binding test-bindings/interrupt-3-cell.yaml>, specifier: {'one': 0, 'two': 0, 'three': 2}>]")

    verify_streq(edt.get_dev("/interrupt-map-test/node@1").interrupts,
                 "[<Interrupt, target: <Device /interrupt-map-test/controller-0 in 'test.dts', binding test-bindings/interrupt-1-cell.yaml>, specifier: {'one': 3}>, <Interrupt, target: <Device /interrupt-map-test/controller-1 in 'test.dts', binding test-bindings/interrupt-2-cell.yaml>, specifier: {'one': 0, 'two': 4}>, <Interrupt, target: <Device /interrupt-map-test/controller-2 in 'test.dts', binding test-bindings/interrupt-3-cell.yaml>, specifier: {'one': 0, 'two': 0, 'three': 5}>]")

    verify_streq(edt.get_dev("/interrupt-map-bitops-test/node@70000000E").interrupts,
                 "[<Interrupt, target: <Device /interrupt-map-bitops-test/controller in 'test.dts', binding test-bindings/interrupt-2-cell.yaml>, specifier: {'one': 3, 'two': 2}>]")

    #
    # Test GPIOs
    #

    verify_streq(edt.get_dev("/gpio-test/node").gpios,
                 "{'': [<GPIO, name: , target: <Device /gpio-test/controller-0 in 'test.dts', binding test-bindings/gpio-1-cell.yaml>, specifier: {'one': 1}>, <GPIO, name: , target: <Device /gpio-test/controller-1 in 'test.dts', binding test-bindings/gpio-2-cell.yaml>, specifier: {'one': 2, 'two': 3}>], 'foo': [<GPIO, name: foo, target: <Device /gpio-test/controller-1 in 'test.dts', binding test-bindings/gpio-2-cell.yaml>, specifier: {'one': 4, 'two': 5}>], 'bar': [<GPIO, name: bar, target: <Device /gpio-test/controller-1 in 'test.dts', binding test-bindings/gpio-2-cell.yaml>, specifier: {'one': 6, 'two': 7}>]}")

    #
    # Test clocks
    #

    verify_streq(edt.get_dev("/clock-test/node").clocks,
                 "[<Clock, name: fixed, frequency: 123, target: <Device /clock-test/fixed-clock in 'test.dts', binding test-bindings/fixed-clock.yaml>, specifier: {}>, <Clock, name: one-cell, target: <Device /clock-test/clock-1 in 'test.dts', binding test-bindings/clock-1-cell.yaml>, specifier: {'one': 1}>, <Clock, name: two-cell, target: <Device /clock-test/clock-2 in 'test.dts', binding test-bindings/clock-2-cell.yaml>, specifier: {'one': 1, 'two': 2}>]")

    #
    # Test PWMs
    #

    verify_streq(edt.get_dev("/pwm-test/node").pwms,
                 "[<PWM, name: zero-cell, target: <Device /pwm-test/pwm-0 in 'test.dts', binding test-bindings/pwm-0-cell.yaml>, specifier: {}>, <PWM, name: one-cell, target: <Device /pwm-test/pwm-1 in 'test.dts', binding test-bindings/pwm-1-cell.yaml>, specifier: {'one': 1}>]")

    #
    # Test IO channels
    #

    verify_streq(edt.get_dev("/io-channel-test/node").iochannels,
                 "[<IOChannel, name: io-channel, target: <Device /io-channel-test/io-channel in 'test.dts', binding test-bindings/io-channel.yaml>, specifier: {'one': 1}>]")

    #
    # Test 'reg'
    #

    verify_streq(edt.get_dev("/reg-zero-address-cells/node").regs,
                 "[<Register, addr: 0x0, size: 0x1>, <Register, addr: 0x0, size: 0x2>]")

    verify_streq(edt.get_dev("/reg-zero-size-cells/node").regs,
                 "[<Register, addr: 0x1, size: 0x0>, <Register, addr: 0x2, size: 0x0>]")

    verify_streq(edt.get_dev("/reg-ranges/parent/node").regs,
                 "[<Register, addr: 0x5, size: 0x1>, <Register, addr: 0xe0000000f, size: 0x1>, <Register, addr: 0xc0000000e, size: 0x1>, <Register, addr: 0xc0000000d, size: 0x1>, <Register, addr: 0xa0000000b, size: 0x1>, <Register, addr: 0x0, size: 0x1>]")

    verify_streq(edt.get_dev("/reg-nested-ranges/grandparent/parent/node").regs,
                 "[<Register, addr: 0x30000000200000001, size: 0x1>]")

    #
    # Test !include in bindings
    #

    verify_streq(edt.get_dev("/binding-include").description,
                 "Parent binding")

    verify_streq(edt.get_dev("/binding-include").props,
                 "{'compatible': <Property, name: compatible, value: ['binding-include-test']>, 'foo': <Property, name: foo, value: 0>, 'bar': <Property, name: bar, value: 1>, 'baz': <Property, name: baz, value: 2>}")

    #
    # Test 'sub-node:' in binding
    #

    verify_streq(edt.get_dev("/parent-with-sub-node/node").description,
                 "Sub-node test")

    verify_streq(edt.get_dev("/parent-with-sub-node/node").props,
                 "{'foo': <Property, name: foo, value: 1>, 'bar': <Property, name: bar, value: 2>}")

    #
    # Test Device.property (derived from DT and 'properties:' in the binding)
    #

    verify_streq(edt.get_dev("/props").props,
                 r"{'compatible': <Property, name: compatible, value: ['props']>, 'int': <Property, name: int, value: 1>, 'array': <Property, name: array, value: [1, 2, 3]>, 'uint8-array': <Property, name: uint8-array, value: b'\x124'>, 'string': <Property, name: string, value: 'foo'>, 'string-array': <Property, name: string-array, value: ['foo', 'bar', 'baz']>}")

    #
    # Test having multiple directories with bindings, with a different .dts file
    #

    edt = edtlib.EDT("test-multidir.dts", ["test-bindings", "test-bindings-2"])

    verify_streq(edt.get_dev("/in-dir-1").binding_path,
                 "test-bindings/multidir.yaml")

    verify_streq(edt.get_dev("/in-dir-2").binding_path,
                 "test-bindings-2/multidir.yaml")


    print("all tests passed")


if __name__ == "__main__":
    run()
