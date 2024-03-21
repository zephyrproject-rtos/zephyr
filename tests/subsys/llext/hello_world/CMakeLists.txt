# Copyright (c) 2023 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(llext_hello_world_test)

target_sources(app PRIVATE
  src/test/test_llext_simple.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/include
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
)

# Compile a simple hello world llext to an include file
set(llext_src_file ${PROJECT_SOURCE_DIR}/src/llext/hello_world.c)
set(llext_bin_file ${ZEPHYR_BINARY_DIR}/hello_world.llext)
set(llext_inc_file ${ZEPHYR_BINARY_DIR}/include/generated/hello_world.inc)

add_llext_target(hello_world
  OUTPUT  ${llext_bin_file}
  SOURCES ${llext_src_file}
)

generate_inc_file_for_target(app ${llext_bin_file} ${llext_inc_file})
