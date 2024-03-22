#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(NOT (CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP OR
   CONFIG_BOARD_NRF5340BSIM_NRF5340_CPUAPP))
  message(FATAL_ERROR "${BOARD}${BOARD_QUALIFIERS} is not supported for this sample")
endif()

project(log_multidomain)

target_sources(app PRIVATE src/main.c)

if(CONFIG_IPC_SERVICE)
  target_sources(app PRIVATE src/ipc_service.c)
endif()
