# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if (NOT BOARD STREQUAL unit_testing)
	message(FATAL_ERROR "This project can only be used with '-DBOARD=unit_testing'.")
endif()

FILE(GLOB SOURCES
	src/*.c
)

project(bluetooth_ull_llcp_terminate)
find_package(ZephyrUnittest HINTS $ENV{ZEPHYR_BASE})
include(${ZEPHYR_BASE}/tests/bluetooth/controller/common/defaults_cmake.txt)
target_sources(testbinary PRIVATE ${ll_sw_sources} ${mock_sources} ${common_sources})
