#
# SPDX-License-Identifier: Apache-2.0
#
# Copyright 2023 NXP
#

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  zephyr_library()
  if(CONFIG_BOARD_MIMXRT1062_FMURT6)
    # No flash configuration block exists for the RT1060 with HyperFlash in
    # the SDK, but we can reuse the block for the RT1050 as FMURT6 also uses
    # the same HyperFlash chip
    set(FLASH_CONF evkbimxrt1050_flexspi_nor_config.c)
    set(BOARD_NAME evkbimxrt1050)
  endif()
  set(RT1062_BOARD_DIR
    "${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk-ng/boards/${BOARD_NAME}")
  if(CONFIG_BOOT_FLEXSPI_NOR)
    # Include flash configuration block for RT1050 EVK from NXP's HAL.
    # This configuration block may need modification if another flash chip is
    # used on your custom board. See NXP AN12238 for more information.
    zephyr_compile_definitions(XIP_BOOT_HEADER_ENABLE=1)
    zephyr_compile_definitions(BOARD_FLASH_SIZE=CONFIG_FLASH_SIZE*1024)
    zephyr_library_sources(${RT1062_BOARD_DIR}/xip/${FLASH_CONF})
    zephyr_library_include_directories(${RT1062_BOARD_DIR}/xip)
  endif()
  if(CONFIG_DEVICE_CONFIGURATION_DATA)
    # Include device configuration data block for RT1050 EVK from NXP's HAL.
    # This configuration block may need modification if another SDRAM chip
    # is used on your custom board.
    zephyr_compile_definitions(XIP_BOOT_HEADER_DCD_ENABLE=1)
    zephyr_library_sources(${RT1062_BOARD_DIR}/dcd.c)
  endif()
endif()

if(CONFIG_MCUX_GPT_TIMER)
  message(WARNING "You appear to be using the GPT hardware timer. "
    "This timer will enable lower power modes, but at the cost of reduced "
    "hardware timer resolution")
endif()
