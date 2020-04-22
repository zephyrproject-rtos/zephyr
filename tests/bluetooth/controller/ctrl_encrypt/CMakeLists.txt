# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/tests/bluetooth/controller/common/defaults_cmake.txt)

FILE(GLOB app_sources
        src/*.c
)

if(BOARD STREQUAL unit_testing)
	project(bluetooth_ctrl_ull_conn)
	list(APPEND SOURCES ${app_sources} ${ll_sw_sources} ${mock_sources} ${common_sources})
	# we can not use find_package yet
	#	find_package(ZephyrUnittest HINTS $ENV{ZEPHYR_BASE})
	include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
else()
	message("This must be used with '-DBOARD=unit_testing'\n")
endif()
