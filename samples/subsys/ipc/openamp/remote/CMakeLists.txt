cmake_minimum_required(VERSION 3.20.0)
# Copyright (c) 2018 Nordic Semiconductor ASA
# Copyright (c) 2019 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

if(("${BOARD}" STREQUAL "lpcxpresso54114_m0")
	OR "${BOARD}" STREQUAL "lpcxpresso55s69_cpu1"
	OR "${BOARD}" STREQUAL "mps2_an521_remote"
	OR "${BOARD}" STREQUAL "v2m_musca_b1_ns")
	message(STATUS "${BOARD} compiles as remote in this sample")
else()
	message(FATAL_ERROR "${BOARD} was not supported for this sample")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openamp_remote)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
