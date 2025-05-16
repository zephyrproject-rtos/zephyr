# Copyright (c) 2025 TQ-Systems GmbH <oss@ew.tq-group.com>
# SPDX-License-Identifier: Apache 2.0
# Author: Isaac L. L. Yuki

set(TQ_API_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tq-api)

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  zephyr_library()
  zephyr_library_include_directories(.)
  # No dcdc adjustment needed on the tqmba117xl
  zephyr_compile_definitions(SKIP_DCDC_ADJUSTMENT=1)

  if(CONFIG_BOOT_FLEXSPI_NOR)
    # Include flash configuration block for RT1170 EVK from NXP's HAL.
    # This configuration block may need modification if another flash chip is
    # used on your custom board. See NXP AN12238 for more information.
    zephyr_compile_definitions(XIP_BOOT_HEADER_ENABLE=1)
    zephyr_compile_definitions(BOARD_FLASH_SIZE=CONFIG_FLASH_SIZE*1024)
    zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/xip)
    zephyr_library_sources(${CMAKE_CURRENT_SOURCE_DIR}/xip/flexspi_nor_config.c)
  endif()
    if(CONFIG_DEVICE_CONFIGURATION_DATA)
    # Include device configuration data block for RT1170 EVK from NXP's HAL.
    # This configuration block may need modification if another SDRAM chip
    # is used on your custom board.
    zephyr_compile_definitions(XIP_BOOT_HEADER_DCD_ENABLE=1)
    zephyr_library_sources(${CMAKE_CURRENT_SOURCE_DIR}/dcd.c)
  else()
    if(CONFIG_SRAM_BASE_ADDRESS EQUAL 0x80000000)
      message(WARNING "You are using SDRAM as RAM but no device "
        "configuration data (DCD) is included. This configuration may not boot")
    endif()
  endif()
endif()

  if(CONFIG_ADJUST_PMIC)

    zephyr_include_directories(${TQ_API_PATH}/PF5020)
    zephyr_sources(${TQ_API_PATH}/PF5020/PF5020.c)

    zephyr_sources(${CMAKE_CURRENT_SOURCE_DIR}/init.c)

    message("PMIC included")
  endif()

  if(CONFIG_SHIELD_MBA117XL_TM070)
    zephyr_compile_definitions(DISPLAY_TM070=1)
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/display_tm070)
    zephyr_sources(${CMAKE_CURRENT_SOURCE_DIR}/display_tm070/tm070.c)

    zephyr_sources(${CMAKE_CURRENT_SOURCE_DIR}/init.c)
    message("DISPLAY_TM070 included")
  endif()

if(CONFIG_MCUX_GPT_TIMER)
  message(WARNING "You appear to be using the GPT hardware timer. "
    "This timer will enable lower power modes, but at the cost of reduced "
    "hardware timer resolution")
endif()
