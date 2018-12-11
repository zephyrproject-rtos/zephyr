cmake_minimum_required(VERSION 3.13.1)
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
set(BOARD lpcxpresso54114_m0)

set(PLATFORM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../platform")

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(openamp_remote)

if(NOT ("${BOARD}" STREQUAL "lpcxpresso54114_m0"))
  message(FATAL_ERROR "${BOARD} was specified, but this sample only supports lpcxpresso54114_m0")
endif()

target_sources(app PRIVATE src/main_remote.c
	       ${PLATFORM_DIR}/platform.c
	       ${PLATFORM_DIR}/resource_table.c
	       ${PLATFORM_DIR}/platform_ops.c)

target_include_directories(app PRIVATE ${PLATFORM_DIR})
