# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
	message(FATAL_ERROR
	"Target ${BOARD} not supported for this sample. "
	"There is no remote board selected in Kconfig.sysbuild")
endif()

set(REMOTE_APP remote)

ExternalZephyrProject_Add(
	APPLICATION ${REMOTE_APP}
	SOURCE_DIR  ${APP_DIR}/${REMOTE_APP}
	BOARD       ${SB_CONFIG_REMOTE_BOARD}
)

if("${BOARD}" STREQUAL "nrf5340bsim_nrf5340_cpuapp")
	# For the simulated board, the application core build will produce the final executable
	# for that, we give it the path to the netcore image
	set(NET_LIBRARY_PATH ${CMAKE_BINARY_DIR}/${REMOTE_APP}/zephyr/zephyr.elf)
	set_property(TARGET ${DEFAULT_IMAGE} APPEND_STRING PROPERTY CONFIG
		"CONFIG_NATIVE_SIMULATOR_EXTRA_IMAGE_PATHS=\"${NET_LIBRARY_PATH}\"\n"
	)

	# Let's build the net core library first
	add_dependencies(${DEFAULT_IMAGE} ${REMOTE_APP})

	# Let's meet users expectations of finding the final executable in zephyr/zephyr.exe
	add_custom_target(final_executable
		ALL
		COMMAND
		${CMAKE_COMMAND} -E copy
			${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/zephyr.exe
			${CMAKE_BINARY_DIR}/zephyr/zephyr.exe
		DEPENDS ${DEFAULT_IMAGE}
	)
endif()
