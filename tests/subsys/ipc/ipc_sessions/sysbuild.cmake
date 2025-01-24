#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

if("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
  message(FATAL_ERROR "REMOTE_BOARD must be set to a valid board name")
endif()

# Add remote project
ExternalZephyrProject_Add(
  APPLICATION remote
  SOURCE_DIR ${APP_DIR}/remote
  BOARD ${SB_CONFIG_REMOTE_BOARD}
  BOARD_REVISION ${BOARD_REVISION}
)
set_property(GLOBAL APPEND PROPERTY PM_DOMAINS CPUNET)
set_property(GLOBAL APPEND PROPERTY PM_CPUNET_IMAGES remote)
set_property(GLOBAL PROPERTY DOMAIN_APP_CPUNET remote)
set(CPUNET_PM_DOMAIN_DYNAMIC_PARTITION remote CACHE INTERNAL "")

# Add a dependency so that the remote sample will be built and flashed first
sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} remote)
# Add dependency so that the remote image is flashed first.
sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote)
