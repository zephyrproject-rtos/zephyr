# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(CONFIG_BOARD_NRF5340DK_NRF5340_CPUNET OR
   CONFIG_BOARD_NRF5340BSIM_NRF5340_CPUNET)
  message(INFO " ${BOARD}${BOARD_QUALIFIERS} used for Network Core")
else()
  message(FATAL_ERROR "${BOARD}${BOARD_QUALIFIERS} is not supported for this sample")
endif()

project(sync_rtc_net)

target_sources(app PRIVATE src/main.c)
