# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_settings_nvs_raw)

add_subdirectory(../src nvs_test_bindir)
target_link_libraries(app PRIVATE settings_nvs_test)

# The code is in the library common to several tests.
target_sources(app PRIVATE placeholder.c)

if(TEST)
	target_compile_definitions(app PRIVATE
		-DTEST_${TEST}
		)
endif()
