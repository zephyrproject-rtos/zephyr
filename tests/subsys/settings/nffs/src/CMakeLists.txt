# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Intel Corporation

zephyr_library_named(settings_nffs_test)

add_subdirectory(../../src settings_test_bindir)
add_subdirectory(../../fs/src settings_test_fs_bindir)

target_link_libraries(settings_nffs_test PRIVATE NFFS)
target_link_libraries(settings_nffs_test PRIVATE settings_test)
target_link_libraries(settings_nffs_test PRIVATE settings_test_fs)

zephyr_include_directories(
	$ENV{ZEPHYR_BASE}/subsys/settings/include
	$ENV{ZEPHYR_BASE}/subsys/settings/src
	$ENV{ZEPHYR_BASE}/tests/subsys/settings/fs/include
	$ENV{ZEPHYR_BASE}/tests/subsys/settings/nffs/src
	)

FILE(GLOB mysources *.c)
zephyr_library_sources(${mysources})
