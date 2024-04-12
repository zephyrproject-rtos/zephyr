# Copyright (c) 2024 Croxel, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(.)
zephyr_library_sources(
	nus.c
	nus_inst.c
)

zephyr_library_sources_ifdef(CONFIG_BT_ZEPHYR_NUS_AUTO_START_BLUETOOTH
	bt_nus_auto_start_bt.c
)
