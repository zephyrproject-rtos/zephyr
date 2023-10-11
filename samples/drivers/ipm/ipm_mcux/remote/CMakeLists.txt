cmake_minimum_required(VERSION 3.20.0)
# Copyright (c) 2017-2022, NXP
#
# SPDX-License-Identifier: Apache-2.0
#


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

message(STATUS "${BOARD} compiles as remote in this sample")
project(ipm_mcux_remote)

target_sources(app PRIVATE src/main_remote.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
