# SPDX-FileCopyrightText: 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_TI_MSPI_FLASH_BOOT_MODE_RESET)
  add_subdirectory(${ZEPHYR_BASE}/boards/ti/common/ ${CMAKE_CURRENT_BINARY_DIR}/common)
endif()
