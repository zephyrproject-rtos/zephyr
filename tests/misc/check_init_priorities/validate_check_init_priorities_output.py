#!/usr/bin/env python3

# Copyright 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

"""Validate the output of check_init_priorities against a test reference."""

import sys

REFERENCE_OUTPUT = [
        "INFO: /i2c@11112222/test-i2c-dev@12 PRE_KERNEL_1 51 31 > /gpio@ffff PRE_KERNEL_1 50 27",
        "INFO: /i2c@11112222/test-i2c-dev@12 PRE_KERNEL_1 51 31 > /i2c@11112222 PRE_KERNEL_1 50 28",
        "ERROR: /i2c@11112222/test-i2c-dev@10 PRE_KERNEL_1 49 29 < /gpio@ffff PRE_KERNEL_1 50 27",
        "ERROR: /i2c@11112222/test-i2c-dev@10 PRE_KERNEL_1 49 29 < /i2c@11112222 PRE_KERNEL_1 50 28",
        "INFO: /i2c@11112222/test-i2c-dev@11 PRE_KERNEL_1 50 30 > /gpio@ffff PRE_KERNEL_1 50 27",
        "INFO: /i2c@11112222/test-i2c-dev@11 PRE_KERNEL_1 50 30 > /i2c@11112222 PRE_KERNEL_1 50 28",
]

if len(sys.argv) != 2:
    print(f"usage: {sys.argv[0]} FILE_PATH")
    sys.exit(1)

output = []
with open(sys.argv[1], "r") as file:
    for line in file:
        if line.startswith("INFO: check_init_priorities build_dir:"):
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
