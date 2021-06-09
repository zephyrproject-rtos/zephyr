# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.13.1)

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/ipc_service_remote-prefix/src/ipc_service_remote-build/zephyr)

if("${BOARD}" STREQUAL "nrf5340dk_nrf5340_cpuapp")
  set(BOARD_REMOTE "nrf5340dk_nrf5340_cpunet")
else()
  message(FATAL_ERROR "${BOARD} is not supported for this sample")
endif()

message(INFO " ${BOARD} compile as Master in this sample")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ipc_service)

enable_language(C ASM)

target_sources(app PRIVATE src/main.c)

include(ExternalProject)

ExternalProject_Add(
  ipc_service_remote
  SOURCE_DIR ${APPLICATION_SOURCE_DIR}/remote
  INSTALL_COMMAND ""      # This particular build system has no install command
  CMAKE_CACHE_ARGS -DBOARD:STRING=${BOARD_REMOTE}
  CMAKE_CACHE_ARGS -DDTC_OVERLAY_FILE:STRING=${DTC_OVERLAY_FILE}
  BUILD_BYPRODUCTS "${REMOTE_ZEPHYR_DIR}/${KERNEL_BIN_NAME}"
  # NB: Do we need to pass on more CMake variables?
  BUILD_ALWAYS True
)
