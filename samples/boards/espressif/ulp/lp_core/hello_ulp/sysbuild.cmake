# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2025 Espressif

# Add external project
ExternalZephyrProject_Add(
    APPLICATION hello_ulp_lpcore
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_ULP_REMOTE_BOARD}
  )

add_dependencies(hello_ulp hello_ulp_lpcore)
sysbuild_add_dependencies(CONFIGURE hello_ulp hello_ulp_lpcore)
sysbuild_add_dependencies(FLASH hello_ulp_lpcore hello_ulp)

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  # Make sure MCUboot is flashed first
  sysbuild_add_dependencies(FLASH hello_ulp mcuboot)
endif()
