#
# Copyright 2020-2022 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BOARD_MIMXRT685_EVK_MIMXRT685S_CM33)
zephyr_library()
zephyr_library_sources(init.c)
endif()

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  if(NOT DEFINED CONFIG_BOARD_MIMXRT685_EVK_MIMXRT685S_CM33)
    message(WARNING "It appears you are using the board definition for "
     "the MIMXRT685-EVK, but targeting a custom board. You may need to "
     "update your flash configuration block data")
  endif()
  # Include flash configuration block for R685 EVK from NXP's HAL.
  # This configuration block may need modification if another flash chip is
  # used on your custom board. See NXP AN13386 for more information.
  zephyr_compile_definitions(BOOT_HEADER_ENABLE=1)
  zephyr_compile_definitions(BOARD_FLASH_SIZE=CONFIG_FLASH_SIZE*1024)
  set(RT685_BOARD_DIR
    "${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/boards/evkmimxrt685")
  zephyr_library_sources(${RT685_BOARD_DIR}/flash_config/flash_config.c)
  zephyr_library_include_directories(${RT685_BOARD_DIR}/flash_config)
endif()
