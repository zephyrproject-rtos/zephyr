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


if("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "nrf54h20dk/nrf54h20/cpuppr")
  set(DTC_OVERLAY_FILE "${APP_DIR}/boards/nrf54h20dk_nrf54h20_cpuapp_cpuppr.overlay" CACHE STRING "" FORCE)

elseif("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "nrf54h20dk/nrf54h20/cpuppr/xip")
  set(DTC_OVERLAY_FILE "${APP_DIR}/boards/nrf54h20dk_nrf54h20_cpuapp_cpuppr_xip.overlay" CACHE STRING "" FORCE)

elseif("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "nrf54h20dk/nrf54h20/cpuflpr")
  set(DTC_OVERLAY_FILE "${APP_DIR}/boards/nrf54h20dk_nrf54h20_cpuapp_cpuflpr.overlay" CACHE STRING "" FORCE)

elseif("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "nrf54h20dk/nrf54h20/cpuflpr/xip")
  set(DTC_OVERLAY_FILE "${APP_DIR}/boards/nrf54h20dk_nrf54h20_cpuapp_cpuflpr_xip.overlay" CACHE STRING "" FORCE)

endif()

# Add remote project
ExternalZephyrProject_Add(
  APPLICATION remote_app
  SOURCE_DIR ${APP_DIR}/remote
  BOARD ${SB_CONFIG_REMOTE_BOARD}
  BOARD_REVISION ${BOARD_REVISION}
)

# Configure BabbleSim multi-core setup for nrf5340bsim targets
if("${SB_CONFIG_REMOTE_BOARD}" MATCHES "nrf5340bsim")
  native_simulator_set_child_images(${DEFAULT_IMAGE} remote_app)
  native_simulator_set_final_executable(${DEFAULT_IMAGE})
endif()

# Setup PM partitioning for remote
set_property(GLOBAL APPEND PROPERTY PM_DOMAINS REMOTE)
set_property(GLOBAL APPEND PROPERTY PM_REMOTE_IMAGES remote_app)
set_property(GLOBAL PROPERTY DOMAIN_APP_REMOTE remote_app)
set(REMOTE_PM_DOMAIN_DYNAMIC_PARTITION remote_app CACHE INTERNAL "")

sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} remote_app)
sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote_app)
