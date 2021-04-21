cmake_minimum_required(VERSION 3.13.1)
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(("${BOARD}" STREQUAL "lpcxpresso54114_m0")
	OR "${BOARD}" STREQUAL "lpcxpresso55s69_cpu1")
	message(INFO " ${BOARD} compiles as remote in this sample")
else()
	message(FATAL_ERROR "${BOARD} was not supported for this sample")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ipm_mcux_remote)

target_sources(app PRIVATE src/main_remote.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
