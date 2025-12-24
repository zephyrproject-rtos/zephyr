# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

if("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
  message(FATAL_ERROR "Target ${BOARD}${BOARD_QUALIFIERS} not supported for this sample. "
    "There is no remote board selected in Kconfig.sysbuild")
endif()

set(REMOTE_APP remote)

ExternalZephyrProject_Add(
  APPLICATION ${REMOTE_APP}
  SOURCE_DIR ${APP_DIR}/${REMOTE_APP}
  BOARD ${SB_CONFIG_REMOTE_BOARD}
  BOARD_REVISION ${BOARD_REVISION}
)
