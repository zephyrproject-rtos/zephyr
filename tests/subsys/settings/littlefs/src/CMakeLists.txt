# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Nordic Semiconductor ASA

zephyr_library_named(settings_littlefs_test)

add_subdirectory(../../src settings_test_bindir)
add_subdirectory(../../fs/src settings_test_fs_bindir)

target_link_libraries(settings_littlefs_test PRIVATE LITTLEFS)
target_link_libraries(settings_littlefs_test PRIVATE settings_test)
target_link_libraries(settings_littlefs_test PRIVATE settings_test_fs)

zephyr_include_directories(
	${ZEPHYR_BASE}/subsys/settings/include
	${ZEPHYR_BASE}/subsys/settings/src
	${ZEPHYR_BASE}/tests/subsys/settings/fs/include
	${ZEPHYR_BASE}/tests/subsys/settings/littlefs/src
	)


FILE(GLOB mysources *.c)
zephyr_library_sources(${mysources})
