# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
  message(FATAL_ERROR "REMOTE_BOARD must be set to a valid board name")
endif()

ExternalZephyrProject_Add(
  APPLICATION remote
  SOURCE_DIR ${APP_DIR}/remote
  BOARD ${SB_CONFIG_REMOTE_BOARD}
  BOARD_REVISION ${BOARD_REVISION}
)

sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} remote)
sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote)

native_simulator_set_child_images(${DEFAULT_IMAGE} remote)
native_simulator_set_final_executable(${DEFAULT_IMAGE})
