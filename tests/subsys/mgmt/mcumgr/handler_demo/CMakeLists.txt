# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# This adds the example module to the list of extra zephyr modules
list(APPEND ZEPHYR_EXTRA_MODULES "${CMAKE_CURRENT_SOURCE_DIR}/example_as_module")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(handler_demo)

target_sources(app PRIVATE src/main.c)

# Include handler files
if(CONFIG_MCUMGR_GRP_EXAMPLE_APP)
  target_sources(app PRIVATE example_as_module/src/example_mgmt.c)
  zephyr_include_directories(example_as_module/include)
endif()
