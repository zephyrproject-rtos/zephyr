# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set(REMOTE_APP remote)

ExternalZephyrProject_Add(
	APPLICATION ${REMOTE_APP}_rad
	SOURCE_DIR  ${APP_DIR}/${REMOTE_APP}
	BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpurad
)

# There are sample configurations which do not use PPR.
if(SB_CONFIG_APP_CPUPPR_RUN)
	ExternalZephyrProject_Add(
		APPLICATION ${REMOTE_APP}_ppr
		SOURCE_DIR  ${APP_DIR}/${REMOTE_APP}
		BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpuppr
	)
endif()

# There are sample configurations which do not use FLPR.
if(SB_CONFIG_APP_CPUFLPR_RUN)
	ExternalZephyrProject_Add(
		APPLICATION ${REMOTE_APP}_flpr
		SOURCE_DIR  ${APP_DIR}/${REMOTE_APP}
		BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpuflpr
	)
endif()
