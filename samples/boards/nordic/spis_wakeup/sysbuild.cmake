#
# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#

if(SB_CONFIG_SOC_NRF54H20)
  # Add remote project
  ExternalZephyrProject_Add(
    APPLICATION wakeup_trigger
    SOURCE_DIR ${APP_DIR}/wakeup_trigger
    BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpurad
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()
