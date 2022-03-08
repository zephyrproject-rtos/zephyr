# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Intel Corporation

add_subdirectory(../../src settings_test_bindir)

zephyr_include_directories(
	${ZEPHYR_BASE}/subsys/settings/include
	${ZEPHYR_BASE}/subsys/settings/src
	${ZEPHYR_BASE}/tests/subsys/settings/fcb/src
	)

FILE(GLOB mysources *.c)
target_sources(app PRIVATE ${mysources})
