#!/usr/bin/env python3

# Copyright 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

"""Validate the output of check_init_priorities against a test reference."""

import sys

REFERENCE_OUTPUT = [
        "ERROR: /i2c@11112222/test-i2c-dev@10 PRE_KERNEL_1 0 < /gpio@ffff PRE_KERNEL_1 1",
        "ERROR: /i2c@11112222/test-i2c-dev@10 PRE_KERNEL_1 0 < /i2c@11112222 PRE_KERNEL_1 2",
        "INFO: /i2c@11112222/test-i2c-dev@11 PRE_KERNEL_1 3 > /gpio@ffff PRE_KERNEL_1 1",
        "INFO: /i2c@11112222/test-i2c-dev@11 PRE_KERNEL_1 3 > /i2c@11112222 PRE_KERNEL_1 2",
        "INFO: /i2c@11112222/test-i2c-dev@12 PRE_KERNEL_1 4 > /gpio@ffff PRE_KERNEL_1 1",
        "INFO: /i2c@11112222/test-i2c-dev@12 PRE_KERNEL_1 4 > /i2c@11112222 PRE_KERNEL_1 2",
]

if len(sys.argv) != 2:
    print(f"usage: {sys.argv[0]} FILE_PATH")
    sys.exit(1)

output = []
with open(sys.argv[1], "r") as file:
    for line in file:
        if line.startswith("INFO: check_init_priorities"):
            continue
        output.append(line.strip())

if sorted(REFERENCE_OUTPUT) != sorted(output):
    print("Mismatched otuput")
    print()
    print("expected:")
    print("\n".join(sorted(REFERENCE_OUTPUT)))
    print()
    print("got:")
    print("\n".join(sorted(output)))
    sys.exit(1)

sys.exit(0)
