#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BOARD_MIMXRT700_EVK_MIMXRT798S_CM33_CPU0 OR CONFIG_BOARD_MIMXRT700_EVK_MIMXRT798S_CM33_CPU1)
  zephyr_library()
  zephyr_library_sources(board.c)
endif()

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  if(NOT ((DEFINED CONFIG_BOARD_MIMXRT700_EVK_MIMXRT798S_CM33_CPU0)
  OR (DEFINED CONFIG_BOARD_MIMXRT700_EVK_MIMXRT798S_CM33_CPU1)))
  message(WARNING "It appears you are using the board definition for "
    "the MIMXRT7xx-EVK, but targeting a custom board. You may need to "
    "update your flash configuration block data")
  endif()
  # Include flash configuration block for RT7xx EVK from NXP's HAL.
  # This configuration block may need modification if another flash chip is
  # used on your custom board. See NXP AN13304 for more information.
  zephyr_compile_definitions(BOOT_HEADER_ENABLE=1)
  zephyr_compile_definitions(BOARD_FLASH_SIZE=CONFIG_FLASH_SIZE*1024)
  set(RT7XX_BOARD_DIR
    "${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/boards/mimxrt700evk")
  zephyr_library_sources(${RT7XX_BOARD_DIR}/flash_config/flash_config.c)
  zephyr_library_include_directories(${RT7XX_BOARD_DIR}/flash_config)
endif()
