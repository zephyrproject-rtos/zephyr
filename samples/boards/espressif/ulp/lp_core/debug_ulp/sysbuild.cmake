# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2025 Espressif

# Add external project
ExternalZephyrProject_Add(
    APPLICATION debug_ulp_lpcore
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_ULP_REMOTE_BOARD}
  )

# Add dependencies so that the remote sample will be built first
# This is required because some primary cores need information from the
# remote core's build, such as the output image's LMA
add_dependencies(debug_ulp debug_ulp_lpcore)
sysbuild_add_dependencies(CONFIGURE debug_ulp debug_ulp_lpcore)
sysbuild_add_dependencies(FLASH debug_ulp_lpcore debug_ulp)

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  # Make sure MCUboot is flashed first
  sysbuild_add_dependencies(FLASH debug_ulp mcuboot)
endif()
