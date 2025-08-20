#
#Copyright 2022-2025 NXP
#Copyright(c)2025 u-blox AG
#SPDX-License-Identifier:Apache-2.0
#

if(CONFIG_NXP_RW6XX_BOOT_HEADER)
	dt_nodelabel(xtal32 NODELABEL "xtal32")
	dt_node_has_status(xtal32_status PATH ${xtal32} STATUS okay)
	zephyr_compile_definitions(BOARD_FLASH_SIZE=${CONFIG_FLASH_SIZE}*1024)

	if(BOARD_REVISION STREQUAL "macronix")
		zephyr_library_sources(macronix_flash_config.c)
	elseif(BOARD_REVISION STREQUAL "fidelex")
		zephyr_library_sources(fidelex_flash_config.c)
	else()
		message(FATAL_ERROR "Unsupported board revision: ${BOARD_REVISION}")
	endif()

	if(CONFIG_DT_HAS_NXP_ENET_MAC_ENABLED AND ${xtal32_status})
		message(FATAL_ERROR
			"Ethernet and external 32K clock source are "
			"mutually exclusive on FRDM_RW612 due to shared PCB nets "
			"between the ethernet PHY and the external oscillator")
	endif()
endif()
