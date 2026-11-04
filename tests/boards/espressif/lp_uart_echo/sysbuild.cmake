# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.

ExternalZephyrProject_Add(
    APPLICATION lp_uart_echo_remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_ULP_REMOTE_BOARD}
  )

sysbuild_add_dependencies(FLASH lp_uart_echo_remote lp_uart_echo)

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  sysbuild_add_dependencies(FLASH lp_uart_echo mcuboot)
endif()
