# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/ipm_mcux_remote-prefix/src/ipm_mcux_remote-build/zephyr)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ipm_mcux)

enable_language(C ASM)

target_sources(app PRIVATE src/main_master.c)

if(CONFIG_BOARD_SUPPORT_REMOTE_ENDPOINT)

  include(ExternalProject)

  ExternalProject_Add(
    ipm_mcux_remote
    SOURCE_DIR ${APPLICATION_SOURCE_DIR}/remote
    INSTALL_COMMAND ""      # This particular build system has no install command
    CMAKE_CACHE_ARGS -DBOARD:STRING=${BOARD_REMOTE}
    CMAKE_CACHE_ARGS -DDTC_OVERLAY_FILE:STRING=${DTC_OVERLAY_FILE}
    BUILD_BYPRODUCTS "${REMOTE_ZEPHYR_DIR}/${KERNEL_BIN_NAME}"
    BUILD_ALWAYS True
  )

  add_dependencies(second_core_inc_target ipm_mcux_remote)

  target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

endif()
