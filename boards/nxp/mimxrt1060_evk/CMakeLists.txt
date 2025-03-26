#
# Copyright 2018-2022, 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if (CONFIG_DISPLAY)
message(WARNING "
CONFIG_DISPLAY: Running this firmware on a board without a display may damage the board
")
endif()

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  zephyr_library()
  if((${BOARD_REVISION} STREQUAL "B") OR (${BOARD_REVISION} STREQUAL "C"))
    set(FLASH_CONF evkbmimxrt1060_flexspi_nor_config.c)
    set(BOARD_NAME evkbmimxrt1060)
  elseif(CONFIG_DT_HAS_NXP_IMX_FLEXSPI_NOR_ENABLED)
    set(FLASH_CONF evkmimxrt1060_flexspi_nor_config.c)
    set(BOARD_NAME evkmimxrt1060)
  elseif(CONFIG_DT_HAS_NXP_IMX_FLEXSPI_HYPERFLASH_ENABLED)
    # No flash configuration block exists for the RT1060 with HyperFlash in
    # the SDK, but we can reuse the block for the RT1050 as both boards use
    # the same HyperFlash chip
    set(FLASH_CONF evkbimxrt1050_flexspi_nor_config.c)
    set(BOARD_NAME evkbimxrt1050)
  else()
    message(WARNING "It appears you are using the board definition for "
     "the MIMXRT1060-EVK, but targeting a custom board. You may need to "
     "update your flash configuration or device configuration data blocks")
    # Default EVK configuration uses qspi, so use that file
    set(FLASH_CONF evkbmimxrt1060_flexspi_nor_config.c)
    set(BOARD_NAME evkbmimxrt1060)
  endif()
  set(RT1060_BOARD_DIR
    "${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk-ng/boards/${BOARD_NAME}")
  if(CONFIG_BOOT_FLEXSPI_NOR)
    # Include flash configuration block for RT1060 EVK from NXP's HAL.
    # This configuration block may need modification if another flash chip is
    # used on your custom board. See NXP AN12238 for more information.
    zephyr_compile_definitions(XIP_BOOT_HEADER_ENABLE=1)
    zephyr_compile_definitions(BOARD_FLASH_SIZE=CONFIG_FLASH_SIZE*1024)
    zephyr_library_sources(${RT1060_BOARD_DIR}/xip/${FLASH_CONF})
    zephyr_library_include_directories(${RT1060_BOARD_DIR}/xip)
  endif()
  if(CONFIG_DEVICE_CONFIGURATION_DATA)
    # Include device configuration data block for RT1060 EVK from NXP's HAL.
    # This configuration block may need modification if another SDRAM chip
    # is used on your custom board.
    zephyr_compile_definitions(XIP_BOOT_HEADER_DCD_ENABLE=1)
    zephyr_library_sources(${RT1060_BOARD_DIR}/dcd.c)
  else()
    if(CONFIG_SRAM_BASE_ADDRESS EQUAL 0x80000000)
      message(WARNING "You are using SDRAM as RAM but no device "
        "configuration data (DCD) is included. This configuration may not boot")
    endif()
  endif()
endif()

if(CONFIG_MCUX_GPT_TIMER)
  message(WARNING "You appear to be using the GPT hardware timer. "
    "This timer will enable lower power modes, but at the cost of reduced "
    "hardware timer resolution")
endif()
