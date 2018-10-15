cmake_minimum_required(VERSION 3.8.2)
# Copyright (c) 2018, Cypress Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

set(BOARD cy8ckit_062_wifi_bt_m0)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

include(ExternalProject)

ExternalProject_Add(
  ipm_psoc6_client
  SOURCE_DIR ${APPLICATION_SOURCE_DIR}/client
  INSTALL_COMMAND ""      # This particular build system has no install command
  BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/ipm_psoc6_client-prefix/src/ipm_psoc6_client-build/zephyr/zephyr.bin"
)

project(ipm_psoc6)

add_dependencies(core_m4_inc_target ipm_psoc6_client)

target_sources(app PRIVATE server/src/main_server.c)