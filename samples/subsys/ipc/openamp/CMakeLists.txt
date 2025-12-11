cmake_minimum_required(VERSION 3.20.0)
# Copyright (c) 2018 Nordic Semiconductor ASA
# Copyright (c) 2019 Linaro Limited
# Copyright 2022 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/../openamp_remote/zephyr)


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openamp)

message(STATUS "${BOARD} compile as Master in this sample")

if(CONFIG_INCLUDE_REMOTE_DIR)
  target_include_directories(zephyr_interface
    INTERFACE ${REMOTE_ZEPHYR_DIR}/include/public)
endif()

enable_language(C ASM)

target_sources(app PRIVATE src/main.c)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
