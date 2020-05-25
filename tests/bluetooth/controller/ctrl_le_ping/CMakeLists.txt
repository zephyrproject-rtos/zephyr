# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/tests/bluetooth/controller/common/defaults_cmake.txt)

FILE(GLOB app_sources
        src/*.c
)

if(BOARD STREQUAL unit_testing)
	project(bluetooth_ull_llcp_le_ping)
	list(APPEND SOURCES ${app_sources} ${ll_sw_sources} ${mock_sources}  ${common_sources})
	# we can not use find_package yet
	#	find_package(ZephyrUnittest HINTS $ENV{ZEPHYR_BASE})
	include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
else()
	message("Only use this with -DBOARD='unit_testing'")
endif()
