cmake_minimum_required(VERSION 3.8.2)
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
set(BOARD lpcxpresso54114_m4)


include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
include(ExternalProject)

ExternalProject_Add(
  ipm_mcux_remote
  SOURCE_DIR ${APPLICATION_SOURCE_DIR}/remote
  INSTALL_COMMAND ""      # This particular build system has no install command
  BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/ipm_mcux_remote-prefix/src/ipm_mcux_remote-build/zephyr/zephyr.bin"
)

project(NONE)

target_sources(app PRIVATE src/main_master.c)
add_dependencies(core_m0_inc_target ipm_mcux_remote)
