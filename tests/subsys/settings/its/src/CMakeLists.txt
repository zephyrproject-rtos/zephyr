# Copyright (c) 2019 Intel Corporation
# Copyright (c) 2025 Analog Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	${ZEPHYR_BASE}/subsys/settings/include
	${ZEPHYR_BASE}/subsys/settings/src
	${ZEPHYR_BASE}/tests/subsys/settings/its/src
	)

target_sources(app PRIVATE settings_test_its.c)

add_subdirectory(../../src settings_test_bindir)
