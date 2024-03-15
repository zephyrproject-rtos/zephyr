# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if("${SB_CONFIG_NET_CORE_BOARD}" STREQUAL "")
	message(FATAL_ERROR
	"Target ${BOARD} not supported for this sample. "
	"There is no remote board selected in Kconfig.sysbuild")
endif()

set(NET_APP remote)

ExternalZephyrProject_Add(
	APPLICATION ${NET_APP}
	SOURCE_DIR  ${APP_DIR}/${NET_APP}
	BOARD       ${SB_CONFIG_NET_CORE_BOARD}
)

native_simulator_set_child_images(${DEFAULT_IMAGE} ${NET_APP})

native_simulator_set_final_executable(${DEFAULT_IMAGE})
