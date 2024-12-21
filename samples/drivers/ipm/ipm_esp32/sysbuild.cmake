# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2024 Espressif

# Add external project
ExternalZephyrProject_Add(
    APPLICATION ipm_esp32_remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_IPM_REMOTE_BOARD}
  )

# Add dependencies so that the remote sample will be built first
# This is required because some primary cores need information from the
# remote core's build, such as the output image's LMA
add_dependencies(ipm_esp32 ipm_esp32_remote)
sysbuild_add_dependencies(CONFIGURE ipm_esp32 ipm_esp32_remote)

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  # Make sure MCUboot is flashed first
  sysbuild_add_dependencies(FLASH ipm_esp32_remote mcuboot)
endif()
