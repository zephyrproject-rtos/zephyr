# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set(REMOTE_APP remote)

ExternalZephyrProject_Add(
  APPLICATION ${REMOTE_APP}_rad
  SOURCE_DIR ${APP_DIR}/${REMOTE_APP}
  BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpurad
)

ExternalZephyrProject_Add(
  APPLICATION ${REMOTE_APP}_ppr
  SOURCE_DIR ${APP_DIR}/${REMOTE_APP}
  BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpuppr
)

ExternalZephyrProject_Add(
  APPLICATION ${REMOTE_APP}_flpr
  SOURCE_DIR ${APP_DIR}/${REMOTE_APP}
  BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpuflpr
)
