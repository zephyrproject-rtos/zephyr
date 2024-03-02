# Copyright (c) 2024 Croxel, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(.)
zephyr_library_sources(
	nus.c
	nus_inst.c
)
