# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(kernel_layout)

# One C and one C++ translation unit on purpose: the test compares what the
# two languages see of the same kernel structures.
target_sources(app PRIVATE src/layout_c.c src/main.cpp)
