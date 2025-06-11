#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(board.c)

if(CONFIG_FLEXSPI_CONFIG_BLOCK_OFFSET)
  # Include flash configuration block
  zephyr_compile_definitions(XIP_BOOT_HEADER_ENABLE=1)
  set(BOARD_DIR "${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk-ng/boards/frdmmcxn947")
  zephyr_library_sources(${BOARD_DIR}/xip/mcxn_flexspi_nor_config.c)
  zephyr_library_include_directories(${BOARD_DIR}/xip)
endif()
