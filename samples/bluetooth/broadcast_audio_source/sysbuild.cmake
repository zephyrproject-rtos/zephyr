# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(NOT("${SB_CONFIG_NET_CORE_BOARD}" STREQUAL ""))
	# For builds in the nrf5340, we build the netcore image with the controller

	set(NET_APP hci_ipc)
	set(NET_APP_SRC_DIR ${ZEPHYR_BASE}/samples/bluetooth/${NET_APP})

	ExternalZephyrProject_Add(
		APPLICATION ${NET_APP}
		SOURCE_DIR  ${NET_APP_SRC_DIR}
		BOARD       ${SB_CONFIG_NET_CORE_BOARD}
	)

	set(${NET_APP}_CONF_FILE
	 ${NET_APP_SRC_DIR}/nrf5340_cpunet_iso-bt_ll_sw_split.conf
	 CACHE INTERNAL ""
	)

	# Let's build the net core library first
	add_dependencies(${DEFAULT_IMAGE} ${NET_APP})

	if("${BOARD}" STREQUAL "nrf5340bsim_nrf5340_cpuapp")
		# For the simulated board, the application core build will produce the final executable
		# for that, we give it the path to the netcore image
		set(NET_LIBRARY_PATH ${CMAKE_BINARY_DIR}/${NET_APP}/zephyr/zephyr.elf)
		set_property(TARGET ${DEFAULT_IMAGE} APPEND_STRING PROPERTY CONFIG
			"CONFIG_NATIVE_SIMULATOR_EXTRA_IMAGE_PATHS=\"${NET_LIBRARY_PATH}\"\n"
		)
	endif()
endif()

if(("${BOARD}" MATCHES "native") OR ("${BOARD}" MATCHES "bsim"))
	# Let's meet the expectation of finding the final executable in zephyr/zephyr.exe
	add_custom_target(final_executable
		ALL
		COMMAND
		${CMAKE_COMMAND} -E copy
		${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/zephyr.exe
		${CMAKE_BINARY_DIR}/zephyr/zephyr.exe
		DEPENDS ${DEFAULT_IMAGE}
	)
endif()
