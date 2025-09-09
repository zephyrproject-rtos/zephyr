# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(DEFINED SB_CONFIG_REMOTE_BOARD)
  ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_REMOTE_BOARD}
  )

  add_dependencies(${DEFAULT_IMAGE} remote)
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote)
endif()
