#
# Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
#
# SPDX-License-Identifier: Apache-2.0
#

set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf32-or1k")

add_subdirectory(core)
zephyr_include_directories(include)
