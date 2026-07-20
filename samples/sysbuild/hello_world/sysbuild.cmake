# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(DEFINED SB_CONFIG_REMOTE_BOARD)
  ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_REMOTE_BOARD}
    BOARD_REVISION ${BOARD_REVISION}
  )

  add_dependencies(${DEFAULT_IMAGE} remote)
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote)

  # Additional operations if nordic vpr launcher with loading from array is used.
  if(SB_CONFIG_VPR_LAUNCHER_FROM_ARRAY)
    nordic_vpr_launcher_from_array(${DEFAULT_IMAGE} remote)
  endif()
endif()
