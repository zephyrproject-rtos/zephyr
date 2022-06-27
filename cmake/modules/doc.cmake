# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# This CMake module will load all Zephyr CMake modules required for a
# documentation build.
#
# The following CMake modules will be loaded:
# - extensions
# - python
# - west
# - root
# - zephyr_module
#
# Outcome:
# The Zephyr package required for documentation build setup.

include_guard(GLOBAL)

include(extensions)
include(python)
include(west)
include(root)
include(zephyr_module)
