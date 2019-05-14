cmake_minimum_required(VERSION 3.13.1)
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
set(BOARD lpcxpresso54114_m4)

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR}/remote)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(openamp)

if(NOT ("${BOARD}" STREQUAL "lpcxpresso54114_m4"))
  message(FATAL_ERROR "${BOARD} was specified, but this sample only supports lpcxpresso54114_m4")
endif()

enable_language(C ASM)

target_sources(app PRIVATE src/main.c)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
