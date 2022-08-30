# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(functional_nvs)

# The code is in the library common to several tests.
target_sources(app PRIVATE settings_test_nvs.c)

add_subdirectory(../src func_test_bindir)

if(TEST)
	target_compile_definitions(app PRIVATE
		-DTEST_${TEST}
		)
endif()
