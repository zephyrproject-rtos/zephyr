#
# Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(NOT CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP)
  message(FATAL_ERROR "${BOARD} is not supported for this sample")
endif()

project(ipc_service)

target_sources(app PRIVATE src/main.c)

include(ExternalProject)
