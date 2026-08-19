# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2025 Espressif

ExternalZephyrProject_Add(
    APPLICATION debug_ulp_lpcore
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_ULP_REMOTE_BOARD}
  )

sysbuild_add_dependencies(FLASH debug_ulp_lpcore debug_ulp)

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  sysbuild_add_dependencies(FLASH debug_ulp mcuboot)
endif()
