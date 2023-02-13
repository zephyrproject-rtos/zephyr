# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library()
zephyr_library_sources(
	conn_mgr.c
	events_handler.c
	)
