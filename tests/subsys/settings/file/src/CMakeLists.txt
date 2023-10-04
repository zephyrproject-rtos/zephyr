# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Nordic Semiconductor ASA

add_subdirectory(../../src settings_test_bindir)

zephyr_include_directories(
	${ZEPHYR_BASE}/subsys/settings/include
	${ZEPHYR_BASE}/subsys/settings/src
	${ZEPHYR_BASE}/tests/subsys/settings/file/include
	)

target_sources(app PRIVATE
  settings_test_compress_file.c
  settings_test_empty_file.c
  settings_test_file.c
  settings_test_multiple_in_file.c
  settings_test_save_in_file.c
  settings_test_save_one_file.c
  settings_test_small_file.c
  settings_test_fs.c
)
target_sources_ifdef(CONFIG_FILE_SYSTEM_LITTLEFS app PRIVATE settings_setup_littlefs.c)
