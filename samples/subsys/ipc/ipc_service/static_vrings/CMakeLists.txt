#
# Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/../remote/zephyr)

if(NOT (CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP OR
        CONFIG_BOARD_NRF5340BSIM_NRF5340_CPUAPP OR
        CONFIG_BOARD_LPCXPRESSO55S69_LPC55S69_CPU0 OR
        CONFIG_BOARD_MIMXRT1160_EVK_MIMXRT1166_CM7 OR
        CONFIG_BOARD_MIMXRT1170_EVK_MIMXRT1176_CM7 OR
        CONFIG_BOARD_FRDM_MCXN947_MCXN947_CPU0
       )
  )
  message(FATAL_ERROR "${BOARD} is not supported for this sample")
endif()

project(ipc_service)

if(CONFIG_INCLUDE_REMOTE_DIR)
  target_include_directories(zephyr_interface
    INTERFACE ${REMOTE_ZEPHYR_DIR}/include/public)
endif()

target_sources(app PRIVATE src/main.c)

include(ExternalProject)
