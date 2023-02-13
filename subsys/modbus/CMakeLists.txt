# Copyright (c) 2020 PHYTEC Messtechnik GmbH
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MODBUS)
	zephyr_library()

	zephyr_include_directories(${ZEPHYR_BASE}/subsys/modbus)

	zephyr_library_sources(
		modbus_core.c
	)

	zephyr_library_sources_ifdef(
		CONFIG_MODBUS_SERIAL
		modbus_serial.c
	)

	zephyr_library_sources_ifdef(
		CONFIG_MODBUS_RAW_ADU
		modbus_raw.c
	)

	zephyr_library_sources_ifdef(
		CONFIG_MODBUS_SERVER
		modbus_server.c
	)

	zephyr_library_sources_ifdef(
		CONFIG_MODBUS_CLIENT
		modbus_client.c
	)
endif()
