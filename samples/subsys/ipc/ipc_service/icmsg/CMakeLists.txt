#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(NOT CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP AND
    NOT CONFIG_BOARD_NRF5340BSIM_NRF5340_CPUAPP AND
    NOT CONFIG_BOARD_STM32H747I_DISCO AND
    NOT CONFIG_BOARD_NRF54L15DK_NRF54L15_CPUAPP)
  message(FATAL_ERROR "${BOARD} is not supported for this sample")
endif()

project(ipc_service_host)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common)

target_sources(app PRIVATE src/main.c)

include(ExternalProject)
