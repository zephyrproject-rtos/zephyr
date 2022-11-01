#
# Copyright (c) 2021 MADMACHINE LIMITED
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_NXP_IMX_RT_BOOT_HEADER)
  zephyr_compile_definitions(XIP_BOOT_HEADER_ENABLE=1)
  zephyr_compile_definitions(XIP_BOOT_HEADER_DCD_ENABLE=1)
  zephyr_compile_definitions(BOARD_FLASH_SIZE=CONFIG_FLASH_SIZE*1024)
  zephyr_sources_ifdef(CONFIG_BOOT_FLEXSPI_NOR flexspi_nor_config.c)
  zephyr_sources_ifdef(CONFIG_DEVICE_CONFIGURATION_DATA mmfeather_sdram_ini_dcd.c)
endif()
