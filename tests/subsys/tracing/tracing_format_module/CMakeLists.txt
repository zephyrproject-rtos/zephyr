# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tracing_format_module)

# A real out-of-tree format ships this directory on the include path from its
# module CMakeLists; here the test app does it directly. The header named by
# CONFIG_TRACING_FORMAT_HEADER must be resolvable from this path.
zephyr_include_directories(format)

target_sources(app PRIVATE src/main.c)
