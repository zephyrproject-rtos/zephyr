#!/usr/bin/env python3

# Copyright 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

"""Validate the output of check_init_priorities against a test reference."""

import sys

REFERENCE_OUTPUT = [
        "ERROR: Device initialization priority validation failed, the sequence of initialization calls does not match the devicetree dependencies.",
        "ERROR: /i2c@11112222/test-i2c-dev@10 <NULL> is initialized before its dependency /gpio@ffff <init_fn_0> (PRE_KERNEL_1+0 < PRE_KERNEL_1+1)",
        "ERROR: /i2c@11112222/test-i2c-dev@10 <NULL> is initialized before its dependency /i2c@11112222 <init_fn_1> (PRE_KERNEL_1+0 < PRE_KERNEL_1+2)",
        "INFO: /i2c@11112222/test-i2c-dev@11 <NULL> PRE_KERNEL_1+3 > /gpio@ffff <init_fn_0> PRE_KERNEL_1+1",
        "INFO: /i2c@11112222/test-i2c-dev@11 <NULL> PRE_KERNEL_1+3 > /i2c@11112222 <init_fn_1> PRE_KERNEL_1+2",
]

REFERENCE_OUTPUT_INITLEVELS = [
        "EARLY",
        "PRE_KERNEL_1",
        "__init___device_dts_ord_31: init_fn_0(__device_dts_ord_31)",
        "__init___device_dts_ord_32: init_fn_1(__device_dts_ord_32)",
        "__init___device_dts_ord_33: NULL(__device_dts_ord_33)",
        "__init___device_dts_ord_34: NULL(__device_dts_ord_34)",
        "__init_posix_arch_console_init: posix_arch_console_init(NULL)",
        "PRE_KERNEL_2",
        "__init_sys_clock_driver_init: sys_clock_driver_init(NULL)",
        "POST_KERNEL",
        "APPLICATION",
        "SMP",
]

if len(sys.argv) != 3:
    print(f"usage: {sys.argv[0]} FILE_PATH FILE_PATH_INITLEVELS")
    sys.exit(1)

def check_file(file_name, expect):
    output = []
    with open(file_name, "r") as file:
        for line in file:
            if line.startswith("INFO: check_init_priorities"):
                continue
            output.append(line.strip())

    if sorted(expect) != sorted(output):
        print(f"Mismatched otuput from {file_name}")
        print()
        print("expected:")
        print("\n".join(sorted(expect)))
        print()
        print("got:")
        print("\n".join(sorted(output)))
        print("TEST FAILED")
        sys.exit(1)

check_file(sys.argv[1], REFERENCE_OUTPUT)
check_file(sys.argv[2], REFERENCE_OUTPUT_INITLEVELS)

print("TEST PASSED")
sys.exit(0)
