# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2026 Espressif Systems (Shanghai) Co., Ltd.

ExternalZephyrProject_Add(
    APPLICATION timer_wakeup_lpcore
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_ULP_REMOTE_BOARD}
  )

sysbuild_add_dependencies(FLASH timer_wakeup_lpcore timer_wakeup)

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  sysbuild_add_dependencies(FLASH timer_wakeup mcuboot)
endif()
