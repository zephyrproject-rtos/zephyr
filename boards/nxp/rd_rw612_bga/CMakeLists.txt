#
# Copyright 2022-2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_NXP_RW6XX_BOOT_HEADER)
  if(NOT DEFINED CONFIG_BOARD_RD_RW612_BGA)
    message(WARNING "It appears you are using the board definition for "
      "the RD_RW612_BGA, but targeting a custom board. You may need to "
     "update your flash configuration block data")
  endif()
  zephyr_compile_definitions(BOOT_HEADER_ENABLE=1)
  zephyr_compile_definitions(BOARD_FLASH_SIZE=CONFIG_FLASH_SIZE*1024)
  set(RW612_BOARD_DIR
    "${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/boards/rdrw612bga")
  zephyr_library()
  zephyr_library_sources(${RW612_BOARD_DIR}/flash_config/flash_config.c)
  zephyr_library_include_directories(${RW612_BOARD_DIR}/flash_config)
endif()
