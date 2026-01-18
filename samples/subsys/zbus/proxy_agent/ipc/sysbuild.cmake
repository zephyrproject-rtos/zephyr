#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP)
endif()

if("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
  message(FATAL_ERROR
  "Target ${BOARD} not supported for this sample. "
  "There is no remote board selected in Kconfig.sysbuild")
endif()

# Add remote project
ExternalZephyrProject_Add(
  APPLICATION remote_app
  SOURCE_DIR ${APP_DIR}/remote
  BOARD ${SB_CONFIG_REMOTE_BOARD}
  BOARD_REVISION ${BOARD_REVISION}
)

sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} remote_app)
sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote_app)
