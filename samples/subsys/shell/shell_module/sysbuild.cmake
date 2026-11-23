# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# A remote build is only assembled when SB_CONFIG_REMOTE_BOARD has been set
# (auto-defaulted for the Nordic targets in Kconfig.sysbuild, or set
# explicitly by the user).  Boards that mandate sysbuild for unrelated
# reasons (e.g. assembling a multi-image firmware bundle) but do not need
# the remote shell get to build the standalone sample.
if(NOT "${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
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
