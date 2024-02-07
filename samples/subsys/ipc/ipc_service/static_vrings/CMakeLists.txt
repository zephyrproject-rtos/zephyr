#
# Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/../remote/zephyr)

if(NOT (("${BOARD}" STREQUAL "nrf5340dk") OR
        ("${BOARD}" STREQUAL "lpcxpresso55s69") OR
        ("${BOARD}" STREQUAL "mimxrt1160_evk") OR
        ("${BOARD}" STREQUAL "mimxrt1170_evk") OR
        ("${BOARD}" STREQUAL "mimxrt1170_evkb")
       )
    )
  message(FATAL_ERROR "${BOARD} is not supported for this sample")
endif()

project(ipc_service)

if(CONFIG_INCLUDE_REMOTE_DIR)
  target_include_directories(zephyr_interface
    INTERFACE ${REMOTE_ZEPHYR_DIR}/include/public)
endif()

target_sources(app PRIVATE src/main.c)

include(ExternalProject)
