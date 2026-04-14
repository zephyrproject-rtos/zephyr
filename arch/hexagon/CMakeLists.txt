# SPDX-License-Identifier: Apache-2.0
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

add_subdirectory(core)

# Add hexagon-specific include directory
zephyr_include_directories(include)

set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-littlehexagon)
