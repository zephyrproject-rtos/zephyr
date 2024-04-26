# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if("${SB_CONFIG_RAD_CORE_BOARD}" STREQUAL "")
	message(FATAL_ERROR
	"Target ${BOARD} not supported for this sample. "
	"There is no remote board selected in Kconfig.sysbuild")
endif()

set(RAD_APP remote)

ExternalZephyrProject_Add(
	APPLICATION ${RAD_APP}
	SOURCE_DIR  ${APP_DIR}/${RAD_APP}
	BOARD       ${SB_CONFIG_RAD_CORE_BOARD}
)
