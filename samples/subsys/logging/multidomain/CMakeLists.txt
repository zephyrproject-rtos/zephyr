#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(NOT(("${BOARD}" STREQUAL "nrf5340dk")
    OR ("${BOARD}${BOARD_IDENTIFIER}" STREQUAL "nrf5340bsim/nrf5340/cpuapp")
    OR ("${BOARD}" STREQUAL "nrf5340bsim_nrf5340_cpuapp")))
  message(FATAL_ERROR "${BOARD}${BOARD_IDENTIFIER} is not supported for this sample")
endif()

project(log_multidomain)

target_sources(app PRIVATE src/main.c)

if (CONFIG_IPC_SERVICE)
  target_sources(app PRIVATE src/ipc_service.c)
endif()
