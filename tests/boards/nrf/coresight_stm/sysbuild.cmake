# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if("${SB_CONFIG_REMOTE_1_BOARD}" STREQUAL "")
	message(FATAL_ERROR
	"Target ${BOARD} not supported for this sample. "
	"There is no remote board selected in Kconfig.sysbuild")
endif()

ExternalZephyrProject_Add(
	APPLICATION remote_1
	SOURCE_DIR  ${APP_DIR}/remote
	BOARD       ${SB_CONFIG_REMOTE_1_BOARD}
)

# There are sample configurations which do not use second remote board.
if(NOT "${SB_CONFIG_REMOTE_2_BOARD}" STREQUAL "")
	ExternalZephyrProject_Add(
		APPLICATION remote_2
		SOURCE_DIR  ${APP_DIR}/remote
		BOARD       ${SB_CONFIG_REMOTE_2_BOARD}
	)
endif()

# There are sample configurations which do not use third remote board.
if(NOT "${SB_CONFIG_REMOTE_3_BOARD}" STREQUAL "")
	ExternalZephyrProject_Add(
		APPLICATION remote_3
		SOURCE_DIR  ${APP_DIR}/remote
		BOARD       ${SB_CONFIG_REMOTE_3_BOARD}
	)
endif()
