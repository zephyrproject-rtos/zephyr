# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

include("${CMAKE_CURRENT_LIST_DIR}/../common/remote-dsp-board-select.cmake")

ExternalZephyrProject_Add(
  APPLICATION remote
  SOURCE_DIR ${APP_DIR}/remote
  BOARD ${REMOTE_BOARD}
  BUILD_ONLY TRUE
)

add_dependencies(${DEFAULT_IMAGE} remote)
sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} remote)
