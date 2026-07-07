# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rt700_dual_core_pm)

if(NOT CONFIG_BOARD_MIMXRT700_EVK_MIMXRT798S_CM33_CPU0)
  message(FATAL_ERROR "${BOARD}/${BOARD_QUALIFIERS} is not supported by this sample")
endif()

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/../remote/zephyr)

if(CONFIG_SAMPLE_INCLUDE_REMOTE_DIR)
  target_include_directories(zephyr_interface
    INTERFACE ${REMOTE_ZEPHYR_DIR}/include/public)
endif()

target_sources(app PRIVATE src/main.c)
zephyr_library_include_directories(${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk-ng/drivers/mu1)
