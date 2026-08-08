# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

if(NOT "${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
  # Add remote project (Approach 2: Dual-Core Emulation)
  ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_REMOTE_BOARD}
    BOARD_REVISION ${BOARD_REVISION}
  )

  # Add dependencies so the remote image gets built first
  sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} remote)
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote)
endif()