#
# Copyright 2022-2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(init.c)

if(CONFIG_NXP_RW6XX_BOOT_HEADER)
  zephyr_compile_definitions(BOARD_FLASH_SIZE=CONFIG_FLASH_SIZE*1024)
  # This FCB is specific to the flash on this board, it won't work
  # for boards with different flash chips. If you flash this FCB
  # onto a board with a different flash chip you may break it.
  # See MCUXpresso config tools for making a correct one.
  zephyr_library_sources(W25Q512JVFIQ_FCB.c)
endif()

if (CONFIG_DT_HAS_NXP_ENET_MAC_ENABLED AND CONFIG_XTAL32K)
	message(FATAL_ERROR "Ethernet and external 32K clock source are "
		"mutually exclusive on FRDM_RW612 due to shared PCB nets "
		"between the ethernet PHY and the external oscillator")
endif()
