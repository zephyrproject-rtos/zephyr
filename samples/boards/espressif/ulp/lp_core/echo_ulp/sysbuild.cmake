# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2024 Espressif

ExternalZephyrProject_Add(
    APPLICATION echo_ulp_lpcore
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_ULP_REMOTE_BOARD}
  )

sysbuild_add_dependencies(FLASH echo_ulp_lpcore echo_ulp)

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  sysbuild_add_dependencies(FLASH echo_ulp mcuboot)
endif()
