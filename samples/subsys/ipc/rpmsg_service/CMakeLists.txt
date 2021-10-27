# Copyright (c) 2019 Linaro Limited
# Copyright (c) 2018-2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/rpmsg_service_remote-prefix/src/rpmsg_service_remote-build/zephyr)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rpmsg_service)

enable_language(C ASM)

target_sources(app PRIVATE src/main.c)

if(CONFIG_BOARD_SUPPORT_REMOTE_ENDPOINT)

  include(ExternalProject)

  ExternalProject_Add(
    rpmsg_service_remote
    SOURCE_DIR ${APPLICATION_SOURCE_DIR}/remote
    INSTALL_COMMAND ""      # This particular build system has no install command
    CMAKE_CACHE_ARGS -DBOARD:STRING=${BOARD_REMOTE}
    CMAKE_CACHE_ARGS -DDTC_OVERLAY_FILE:STRING=${DTC_OVERLAY_FILE}
    BUILD_BYPRODUCTS "${REMOTE_ZEPHYR_DIR}/${KERNEL_BIN_NAME}"
    # NB: Do we need to pass on more CMake variables?
    BUILD_ALWAYS True
  )

endif()
