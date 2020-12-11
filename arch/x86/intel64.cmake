# Copyright (c) 2019 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

zephyr_cc_option(-m64)

set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_ARCH "i386:x86-64")
set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf64-x86-64")
get_property(OUTPUT_ARCH   GLOBAL PROPERTY PROPERTY_OUTPUT_ARCH)
get_property(OUTPUT_FORMAT GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT)

add_subdirectory(core)
