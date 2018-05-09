cmake_minimum_required(VERSION 3.13.1)
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
set(BOARD lpcxpresso54114_m4)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(openamp)

if(NOT ("${BOARD}" STREQUAL "lpcxpresso54114_m4"))
  message(FATAL_ERROR "${BOARD} was specified, but this sample only supports lpcxpresso54114_m4")
endif()

enable_language(C ASM)

target_sources(app PRIVATE src/main.c)

include(ExternalProject)

ExternalProject_Add(
  openamp_remote
  SOURCE_DIR ${APPLICATION_SOURCE_DIR}/remote
  INSTALL_COMMAND ""      # This particular build system has no install command
  BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/openamp_remote-prefix/src/openamp_remote-build/zephyr/zephyr.bin"
  # NB: Do we need to pass on more CMake variables?
)
add_dependencies(core_m0_inc_target openamp_remote)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
