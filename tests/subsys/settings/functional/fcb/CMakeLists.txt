# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(functional_fcb)

# The code is in the library common to several tests.
target_sources(app PRIVATE placeholder.c)

add_subdirectory(../src func_test_bindir)
target_link_libraries(app PRIVATE settings_func_test)

if(TEST)
	target_compile_definitions(app PRIVATE
		-DTEST_${TEST}
		)
endif()
