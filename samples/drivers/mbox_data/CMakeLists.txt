# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/../remote/zephyr)

if(CONFIG_BOARD_MIMXRT1170_EVK_MIMXRT1176_CM7 OR
   CONFIG_BOARD_MIMXRT1160_EVK_MIMXRT1166_CM7 OR
   CONFIG_BOARD_LPCXPRESSO55S69_LPC55S69_CPU0)
  message(STATUS "${BOARD}${BOARD_QUALIFIERS} compile as Main in this sample")
else()
  message(FATAL_ERROR "${BOARD}${BOARD_QUALIFIERS} is not supported for this sample")
endif()

project(mbox_data_ipc)

enable_language(C ASM)

if(CONFIG_INCLUDE_REMOTE_DIR)
  target_include_directories(zephyr_interface
    INTERFACE ${REMOTE_ZEPHYR_DIR}/include/public)
endif()

target_sources(app PRIVATE src/main.c)
