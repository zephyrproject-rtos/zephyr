cmake_minimum_required(VERSION 3.13.1)
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
set(BOARD lpcxpresso54114_m4)


include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

if(NOT ("${BOARD}" STREQUAL "lpcxpresso54114_m4"))
  message(FATAL_ERROR "${BOARD} was specified, but this sample only supports lpcxpresso54114_m4")
endif()

include(ExternalProject)

ExternalProject_Add(
  ipm_mcux_remote
  SOURCE_DIR ${APPLICATION_SOURCE_DIR}/remote
  INSTALL_COMMAND ""      # This particular build system has no install command
  BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/ipm_mcux_remote-prefix/src/ipm_mcux_remote-build/zephyr/zephyr.bin"
)

project(ipm_mcux)

target_sources(app PRIVATE src/main_master.c)
add_dependencies(core_m0_inc_target ipm_mcux_remote)
