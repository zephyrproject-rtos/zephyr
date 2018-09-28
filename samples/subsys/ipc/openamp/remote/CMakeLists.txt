cmake_minimum_required(VERSION 3.8.2)
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
set(BOARD lpcxpresso54114_m0)

set(PLATFORM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../platform")

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/main_remote.c
	       ${PLATFORM_DIR}/platform.c
	       ${PLATFORM_DIR}/resource_table.c
	       ${PLATFORM_DIR}/platform_ops.c)

target_include_directories(app PRIVATE ${PLATFORM_DIR})
