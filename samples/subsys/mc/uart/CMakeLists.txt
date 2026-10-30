# Copyright (c) 2026 Mahdi Sahebi
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mc_uart_communication)

set(MICRO_CORE_ROOT "" CACHE PATH "Path to the micro_core source tree")
if(NOT EXISTS "${MICRO_CORE_ROOT}/src/io/communication/mc_communication.c")
  message(FATAL_ERROR "Set MICRO_CORE_ROOT to the micro_core source tree")
endif()

target_sources(app PRIVATE
  src/main.c
  src/mc_crc.c
  src/zephyr_time.c
  "${MICRO_CORE_ROOT}/src/io/communication/mc_communication.c"
  "${MICRO_CORE_ROOT}/src/io/communication/mc_frame.c"
  "${MICRO_CORE_ROOT}/src/io/communication/mc_io.c"
  "${MICRO_CORE_ROOT}/src/io/communication/mc_protocol.c"
  "${MICRO_CORE_ROOT}/src/io/communication/mc_window_pool.c"
)

target_include_directories(app PRIVATE
  "${MICRO_CORE_ROOT}/include"
  "${MICRO_CORE_ROOT}/src/io/communication"
  "${CMAKE_CURRENT_LIST_DIR}/include"
)
