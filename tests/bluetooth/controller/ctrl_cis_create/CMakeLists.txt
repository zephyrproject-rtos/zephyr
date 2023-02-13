# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

project(bluetooth_ull_llcp_cis_create)
find_package(ZephyrUnittest HINTS $ENV{ZEPHYR_BASE})
include(${ZEPHYR_BASE}/tests/bluetooth/controller/common/defaults_cmake.txt)
target_sources(testbinary
	PRIVATE
		src/main.c
		${ll_sw_sources}
		${mock_sources}
		${common_sources}
)
