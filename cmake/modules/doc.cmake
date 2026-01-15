# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

#[=======================================================================[.rst:
doc
###

This CMake module will load all Zephyr CMake modules required for a documentation build.

The following CMake modules will be loaded:

* :cmake:module:`extensions`
* :cmake:module:`python`
* :cmake:module:`west`
* :cmake:module:`root`
* :cmake:module:`zephyr_module`

Outcome:
The Zephyr package required for documentation build setup.
#]=======================================================================]

include_guard(GLOBAL)

include(extensions)
include(python)
include(west)
include(root)
include(zephyr_module)
