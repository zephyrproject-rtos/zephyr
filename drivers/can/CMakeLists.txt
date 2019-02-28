# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_CAN_STM32	 stm32_can.c)
zephyr_sources_ifdef(CONFIG_CAN_MCP2515  mcp2515.c)
zephyr_sources_ifdef(CONFIG_USERSPACE    can_handlers.c)

if(CONFIG_CAN_NATIVE_POSIX)
	zephyr_library()
	zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/l2)
	zephyr_library_compile_definitions(NO_POSIX_CHEATS)
	zephyr_library_compile_definitions(_BSD_SOURCE)
	zephyr_library_compile_definitions(_DEFAULT_SOURCE)
	zephyr_library_sources(
		canbus_native_posix.c
		canbus_native_posix_adapt.c
		)
endif()
