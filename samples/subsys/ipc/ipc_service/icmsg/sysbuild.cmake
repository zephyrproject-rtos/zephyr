# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if ("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
	message(FATAL_ERROR
	"Target ${BOARD} not supported for this sample. "
	"There is no remote board selected in Kconfig.sysbuild")
endif()

ExternalZephyrProject_Add(
	APPLICATION remote
	SOURCE_DIR  ${APP_DIR}/remote
	BOARD       ${SB_CONFIG_REMOTE_BOARD}
)

native_simulator_set_child_images(${DEFAULT_IMAGE} remote)
native_simulator_set_final_executable(${DEFAULT_IMAGE})
