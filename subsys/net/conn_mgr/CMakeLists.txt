# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library()
zephyr_library_sources(
	conn_mgr_monitor.c
	events_handler.c
	conn_mgr_connectivity.c
	)

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)

zephyr_library_sources_ifdef(
  CONFIG_NET_CONNECTION_MANAGER_ONLINE_CONNECTIVITY_CHECK online_checker.c)

zephyr_linker_sources(DATA_SECTIONS conn_mgr.ld)
zephyr_iterable_section(NAME conn_mgr_conn_binding GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT} SUBALIGN 4)
