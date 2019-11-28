# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Intel Corporation

zephyr_library_named(settings_nvs_test)

zephyr_include_directories(
	$ENV{ZEPHYR_BASE}/subsys/settings/include
	$ENV{ZEPHYR_BASE}/subsys/settings/src
	$ENV{ZEPHYR_BASE}/tests/subsys/settings/nvs/src
	)

zephyr_library_sources(settings_test_nvs.c)

add_subdirectory(../../src settings_test_bindir)
target_link_libraries(settings_nvs_test PRIVATE settings_test)
