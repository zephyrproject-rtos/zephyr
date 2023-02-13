# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	${ZEPHYR_BASE}/subsys/settings/include
	${ZEPHYR_BASE}/subsys/settings/src
	)

target_sources(app PRIVATE settings_basic_test.c)
