#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mcuboot_direct_xip)

if(NOT (DEFINED SYSBUILD))
  message(FATAL_ERROR "This test must be built with sysbuild.")
endif()

target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/hello_world/src/main.c)
