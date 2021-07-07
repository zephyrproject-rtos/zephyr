cmake_minimum_required(VERSION 3.13.1)
# Copyright (c) 2019 Linaro Limited
# Copyright (c) 2018-2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

if("${BOARD}" STREQUAL "nrf5340dk_nrf5340_cpunet"
	OR "${BOARD}" STREQUAL "lpcxpresso54114_m0"
	OR "${BOARD}" STREQUAL "mps2_an521_remote"
	OR "${BOARD}" STREQUAL "v2m_musca_b1_ns")
	message(INFO " ${BOARD} compile as slave in this sample")
else()
	message(FATAL_ERROR "${BOARD} was not supported for this sample")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rpmsg_service_remote)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
