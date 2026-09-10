# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2026 Espressif Systems (Shanghai) Co., Ltd.

ExternalZephyrProject_Add(
    APPLICATION gpio_wakeup_lpcore
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_ULP_REMOTE_BOARD}
  )

sysbuild_add_dependencies(FLASH gpio_wakeup_lpcore gpio_wakeup)

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  sysbuild_add_dependencies(FLASH gpio_wakeup mcuboot)
endif()
