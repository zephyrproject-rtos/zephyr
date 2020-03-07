# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Nordic Semiconductor ASA

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(littlefs)

add_subdirectory(../src littlefs_test_bindir)
target_link_libraries(app PRIVATE settings_littlefs_test)

# The code is in the library common to several tests.
target_sources(app PRIVATE placeholder.c)

if(TEST)
	target_compile_definitions(app PRIVATE
		-DTEST_${TEST}
		)
endif()
