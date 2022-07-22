# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	${ZEPHYR_BASE}/subsys/settings/src
	${ZEPHYR_BASE}/tests/subsys/settings/ram/src
	)

target_sources(app PRIVATE settings_test_ram.c)

add_subdirectory(../../src settings_test_bindir)
