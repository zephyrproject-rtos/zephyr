# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(settings_fcb)

add_subdirectory(../src fcb_test_bindir)
target_link_libraries(app PRIVATE settings_fcb_test)

# The code is in the library common to several tests.
target_sources(app PRIVATE placeholder.c)

if(TEST)
	target_compile_definitions(app PRIVATE
		-DTEST_${TEST}
		)
endif()
