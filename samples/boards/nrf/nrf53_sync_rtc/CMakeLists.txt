# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

set(NET_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/sync_rtc_net-prefix/src/sync_rtc_net-build/zephyr)

if("${BOARD}" STREQUAL "nrf5340dk_nrf5340_cpuapp")
  set(BOARD_REMOTE "nrf5340dk_nrf5340_cpunet")
else()
  message(FATAL_ERROR "${BOARD} is not supported for this sample")
endif()

message(INFO " ${BOARD} compile as Master in this sample")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sync_rtc)

enable_language(C ASM)

target_sources(app PRIVATE src/main.c)

include(ExternalProject)

ExternalProject_Add(
  sync_rtc_net
  SOURCE_DIR ${APPLICATION_SOURCE_DIR}/net
  INSTALL_COMMAND ""      # This particular build system has no install command
  CMAKE_CACHE_ARGS -DBOARD:STRING=${BOARD_REMOTE}
  CMAKE_CACHE_ARGS -DDTC_OVERLAY_FILE:STRING=${NET_DTC_OVERLAY_FILE}
  CMAKE_CACHE_ARGS -DOVERLAY_CONFIG:STRING=${NET_OVERLAY_CONF}
  BUILD_BYPRODUCTS "${NET_ZEPHYR_DIR}/${KERNEL_BIN_NAME}"
  # NB: Do we need to pass on more CMake variables?
  BUILD_ALWAYS True
)
