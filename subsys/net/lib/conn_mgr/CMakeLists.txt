# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library()
zephyr_library_sources(
	conn_mgr.c
	events_handler.c
	conn_mgr_connectivity.c
	)

zephyr_linker_sources(DATA_SECTIONS conn_mgr.ld)
zephyr_iterable_section(NAME conn_mgr_conn_binding GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
