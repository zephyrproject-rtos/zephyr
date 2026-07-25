/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-defs.h>

#if defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD) || \
	defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD_WITH_REVERT) || \
	defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_SINGLE_APP_RAM_LOAD)

#if (defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD) || \
	defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD_WITH_REVERT)) && \
	DT_HAS_CHOSEN(mcuboot_image_ram)
#define APPLICATION_LOAD_NODE DT_CHOSEN(mcuboot_image_ram)
#elif defined(CONFIG_MCUBOOT_BOOTLOADER_MODE_SINGLE_APP_RAM_LOAD) && \
	DT_HAS_CHOSEN(mcuboot_ram_load_dev)
#define APPLICATION_LOAD_NODE DT_CHOSEN(mcuboot_ram_load_dev)
#else
#define APPLICATION_LOAD_NODE DT_CHOSEN(zephyr_sram)
#endif

#define APPLICATION_LOAD_START DT_REG_ADDR(APPLICATION_LOAD_NODE)
#define APPLICATION_LOAD_END (APPLICATION_LOAD_START + DT_REG_SIZE(APPLICATION_LOAD_NODE))
#define MCUBOOT_START DT_REG_ADDR(DT_NODELABEL(axisram2))
#define MCUBOOT_END (MCUBOOT_START + DT_REG_SIZE(DT_NODELABEL(axisram2)))

#if (APPLICATION_LOAD_START < MCUBOOT_START) && (APPLICATION_LOAD_END > MCUBOOT_START)
ASSERT((__rom_region_start + _flash_used) <= MCUBOOT_START,
       "
       Image load range overlaps the MCUboot region in AXISRAM2.
       ")
#elif (APPLICATION_LOAD_START < MCUBOOT_END) && (APPLICATION_LOAD_END > MCUBOOT_START)
ASSERT(0,
       "
       Application load region overlaps the MCUboot region in AXISRAM2.
       ")
#endif

#endif /* MCUboot RAM-load modes */
