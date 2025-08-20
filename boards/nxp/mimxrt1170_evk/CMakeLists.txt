#
# Copyright 2022-2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  zephyr_library()
  if(NOT ((DEFINED CONFIG_BOARD_MIMXRT1170_EVK_MIMXRT1176_CM7)
    OR (DEFINED CONFIG_BOARD_MIMXRT1170_EVK_MIMXRT1176_CM4)))
    message(WARNING "It appears you are using the board definition for "
     "the MIMXRT1170-EVK, but targeting a custom board. You may need to "
     "update your flash configuration or device configuration data blocks")
  endif()
  if (${BOARD_REVISION} STREQUAL "A")
    set(RT1170_BOARD_NAME "evkmimxrt1170")
  elseif (${BOARD_REVISION} STREQUAL "B")
    set(RT1170_BOARD_NAME "evkbmimxrt1170")
  endif()
  if(CONFIG_BOOT_FLEXSPI_NOR)
    # This flash configuration block may need modification if another
    # flash chip is used on your custom board. See NXP AN12238 for more
    # information.
    zephyr_compile_definitions(XIP_BOOT_HEADER_ENABLE=1)
    zephyr_compile_definitions(BOARD_FLASH_SIZE=CONFIG_FLASH_SIZE*1024)
    zephyr_library_sources(xip/${RT1170_BOARD_NAME}_flexspi_nor_config.c)
    zephyr_library_include_directories(xip)
  endif()
  if(CONFIG_EXTERNAL_MEM_CONFIG_DATA)
    # This external memory configuration data block may need modification
    # if another SDRAM chip is used on your custom board.
    zephyr_compile_definitions(XIP_BOOT_HEADER_XMCD_ENABLE=1)
    zephyr_library_sources(xmcd/xmcd.c)
  else()
    if(CONFIG_SRAM_BASE_ADDRESS EQUAL 0x80000000)
      message(WARNING "You are using SDRAM as RAM but no external memory"
        "configuration data (XMCD) is included. This configuration may not boot")
    endif()
  endif()
endif()

if(CONFIG_MCUX_GPT_TIMER)
  message(WARNING "You appear to be using the GPT hardware timer. "
    "This timer will enable lower power modes, but at the cost of reduced "
    "hardware timer resolution")
endif()
