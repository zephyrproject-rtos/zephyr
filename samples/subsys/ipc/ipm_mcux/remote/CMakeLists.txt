cmake_minimum_required(VERSION 3.13.1)
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
set(BOARD lpcxpresso54114_m0)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(ipm_mcux_remote)

if(NOT ("${BOARD}" STREQUAL "lpcxpresso54114_m0"))
  message(FATAL_ERROR "${BOARD} was specified, but this sample only supports lpcxpresso54114_m0")
endif()

target_sources(app PRIVATE src/main_remote.c)
