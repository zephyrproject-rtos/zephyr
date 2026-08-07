# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# nRF54H20: same multi-image layout as samples/boards/nordic/coresight_stm (rad + ppr + flpr),
# unconditionally (no SB_CONFIG_APP_CPUPPR_RUN / APP_CPUFLPR_RUN).
if("${SB_CONFIG_SOC}" STREQUAL "nrf54h20")
  set(REMOTE_APP remote)

  ExternalZephyrProject_Add(
    APPLICATION ${REMOTE_APP}_rad
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpurad
    BOARD_REVISION ${BOARD_REVISION}
  )

  ExternalZephyrProject_Add(
    APPLICATION ${REMOTE_APP}_ppr
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpuppr
    BOARD_REVISION ${BOARD_REVISION}
  )

  ExternalZephyrProject_Add(
    APPLICATION ${REMOTE_APP}_flpr
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpuflpr
    BOARD_REVISION ${BOARD_REVISION}
  )

  sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE}
    ${REMOTE_APP}_rad ${REMOTE_APP}_ppr ${REMOTE_APP}_flpr)
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE}
    ${REMOTE_APP}_rad ${REMOTE_APP}_ppr ${REMOTE_APP}_flpr)
else()
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
endif()
