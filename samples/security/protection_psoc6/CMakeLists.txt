cmake_minimum_required(VERSION 3.8.2)
# Copyright (c) 2018, Cypress Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

set(BOARD cy8ckit_062_wifi_bt_m0)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

include(ExternalProject)

ExternalProject_Add(
  prot_psoc6_unsecure
  SOURCE_DIR ${APPLICATION_SOURCE_DIR}/unsecure
  INSTALL_COMMAND ""      # This particular build system has no install command
  BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/prot_psoc6_unsecure-prefix/src/prot_psoc6_unsecure-build/zephyr/zephyr.bin"
)

project(prot_psoc6)

add_dependencies(core_m4_inc_target prot_psoc6_unsecure)

target_sources(app PRIVATE secure/src/main.c)