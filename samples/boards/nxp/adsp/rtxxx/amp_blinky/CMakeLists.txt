# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(amp_blinky)
target_sources(app PRIVATE src/main.c)

include("${CMAKE_CURRENT_LIST_DIR}/../common/dsp-load.cmake")

if(CONFIG_BOARD_MIMXRT700_EVK_MIMXRT798S_CM33_CPU0)
  message(FATAL_ERROR "This example is not supported on the selected board.")
endif()
