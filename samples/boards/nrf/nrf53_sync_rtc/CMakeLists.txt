# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(("${BOARD}" STREQUAL "nrf5340dk") OR
	("${BOARD}${BOARD_IDENTIFIER}" STREQUAL "nrf5340bsim/nrf5340/cpuapp") OR
	("${BOARD}" STREQUAL "nrf5340bsim_nrf5340_cpuapp"))
	message(INFO " ${BOARD}${BOARD_IDENTIFIER} used for Application Core")
else()
	message(FATAL_ERROR "${BOARD} is not supported for this sample")
endif()

project(sync_rtc)

enable_language(C ASM)

target_sources(app PRIVATE src/main.c)
