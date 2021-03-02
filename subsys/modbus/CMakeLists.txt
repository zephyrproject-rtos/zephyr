# Copyright (c) 2020 PHYTEC Messtechnik GmbH
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MODBUS)
	zephyr_library()

	zephyr_include_directories(${ZEPHYR_BASE}/subsys/modbus)

	zephyr_library_sources(
		mb_rtu_core.c
	)

	zephyr_library_sources_ifdef(
		CONFIG_MODBUS_SERVER
		mb_rtu_server.c
	)

	zephyr_library_sources_ifdef(
		CONFIG_MODBUS_CLIENT
		mb_rtu_client.c
	)
endif()
