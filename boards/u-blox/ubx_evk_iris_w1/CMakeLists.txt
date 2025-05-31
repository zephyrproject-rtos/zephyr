#
# Copyright 2022-2025 NXP
# Copyright (c) 2025 u-blox AG
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
if(CONFIG_NXP_RW6XX_BOOT_HEADER)
  zephyr_compile_definitions(BOARD_FLASH_SIZE=${CONFIG_FLASH_SIZE}*1024)

  # Select FCB based on flash type
  if(CONFIG_FLASH_MACRONIX)
    zephyr_library_sources(macronix_flash_config.c)
  elseif(CONFIG_FLASH_FIDELEX)
    zephyr_library_sources(fidelex_flash_config.c)
  elseif(CONFIG_FLASH_MACRONIX AND CONFIG_FLASH_FIDELEX)
    message(FATAL_ERROR "Please disable one of "
    "CONFIG_FLASH_MACRONIX or CONFIG_FLASH_FIDELIX.")
  else()
    message(FATAL_ERROR "No supported flash type,"
     "Define either CONFIG_FLASH_MACRONIX or CONFIG_FLASH_FIDELIX.")
  endif()

if (CONFIG_DT_HAS_NXP_ENET_MAC_ENABLED AND CONFIG_XTAL32K)
	message(FATAL_ERROR "Ethernet and external 32K clock source are "
		"mutually exclusive on FRDM_RW612 due to shared PCB nets "
		"between the ethernet PHY and the external oscillator")
endif()
endif()
