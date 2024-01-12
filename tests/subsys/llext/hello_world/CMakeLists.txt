# Copyright (c) 2023 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

if(NOT CONFIG_LLEXT_TEST_HELLO)
  # test disabled, nothing to do
else()

  set(llext_src_file ${PROJECT_SOURCE_DIR}/hello_world.c)
  set(llext_bin_file ${PROJECT_BINARY_DIR}/hello_world.llext)

  add_llext_target(hello_world
    OUTPUT  ${llext_bin_file}
    SOURCES ${llext_src_file}
  )

  set(HELLO_WORLD_LLEXT ${llext_bin_file} PARENT_SCOPE)

endif()
