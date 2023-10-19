# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

ExternalZephyrProject_Add(
	APPLICATION nrf53_sync_rtc_net
	SOURCE_DIR  ${APP_DIR}/net
	BOARD       ${SB_CONFIG_NET_CORE_BOARD}
)

if("${BOARD}" STREQUAL "nrf5340bsim_nrf5340_cpuapp")
	# For the simulated board, the application core build will produce the final executable
	# for that, we give it the path to the netcore image
	set(NET_LIBRARY_PATH ${CMAKE_BINARY_DIR}/nrf53_sync_rtc_net/zephyr/zephyr.elf)
	set_property(TARGET nrf53_sync_rtc APPEND_STRING PROPERTY CONFIG
		"CONFIG_NATIVE_SIMULATOR_EXTRA_IMAGE_PATHS=\"${NET_LIBRARY_PATH}\"\n"
	)

	# Let's build the net core library first
	add_dependencies(nrf53_sync_rtc nrf53_sync_rtc_net)

	# Let's meet users expectations of finding the final executable in zephyr/zephyr.exe
	add_custom_target(final_executable
		ALL
		COMMAND
		${CMAKE_COMMAND} -E copy
			${CMAKE_BINARY_DIR}/nrf53_sync_rtc/zephyr/zephyr.exe
			${CMAKE_BINARY_DIR}/zephyr/zephyr.exe
		DEPENDS nrf53_sync_rtc
	)
endif()
