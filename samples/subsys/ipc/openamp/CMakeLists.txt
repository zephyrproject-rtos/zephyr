cmake_minimum_required(VERSION 3.13.1)
# Copyright (c) 2018 Nordic Semiconductor ASA
# Copyright (c) 2019 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/openamp_remote-prefix/src/openamp_remote-build/zephyr)

if("${BOARD}" STREQUAL "lpcxpresso54114_m4")
  set(BOARD_REMOTE "lpcxpresso54114_m0")
elseif("${BOARD}" STREQUAL "lpcxpresso55s69_cpu0")
  set(BOARD_REMOTE "lpcxpresso55s69_cpu1")
elseif("${BOARD}" STREQUAL "mps2_an521")
  set(QEMU_EXTRA_FLAGS "-device;loader,file=${REMOTE_ZEPHYR_DIR}/zephyr.elf")
  set(BOARD_REMOTE "mps2_an521_remote")
elseif("${BOARD}" STREQUAL "v2m_musca_b1")
  set(BOARD_REMOTE "v2m_musca_b1_ns")
else()
  message(FATAL_ERROR "${BOARD} was not supported for this sample")
endif()

message(INFO " ${BOARD} compile as Master in this sample")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openamp)

enable_language(C ASM)

target_sources(app PRIVATE src/main.c)

include(ExternalProject)

ExternalProject_Add(
  openamp_remote
  SOURCE_DIR ${APPLICATION_SOURCE_DIR}/remote
  INSTALL_COMMAND ""      # This particular build system has no install command
  CMAKE_CACHE_ARGS -DBOARD:STRING=${BOARD_REMOTE}
  CMAKE_CACHE_ARGS -DDTC_OVERLAY_FILE:STRING=${DTC_OVERLAY_FILE}
  BUILD_BYPRODUCTS "${REMOTE_ZEPHYR_DIR}/${KERNEL_BIN_NAME}"
  # NB: Do we need to pass on more CMake variables?
  BUILD_ALWAYS True
)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
