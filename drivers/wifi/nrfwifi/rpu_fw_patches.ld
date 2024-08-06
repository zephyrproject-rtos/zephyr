/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Custom Linker command/script file
 *
 * Custom Linker script for the Cortex-M platforms.
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#if CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP || CONFIG_BOARD_NRF52840DK_NRF52840
/*
 * nRF53/52 series ship an external flash that can be used for XIP using QSPI/SPI.
 *
 * Note: In nRF7002 external flash using is accessible only using SPI but there is no
 *       support for XIP, so, relocation cannot be used.
 */
#if CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP
#define EXTFLASH_BASE_ADDR 0x10000000
#define EXTFLASH_SIZE 0x800000
#elif CONFIG_BOARD_NRF52840DK_NRF52840
#define EXTFLASH_BASE_ADDR 0x12000000
#define EXTFLASH_SIZE 0x800000
#endif /* CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP */

#if USE_PARTITION_MANAGER && PM_EXTERNAL_FLASH_ADDRESS
#include <pm_config.h>
#define EXTFLASH_ADDRESS (EXTFLASH_BASE_ADDR + PM_EXTERNAL_FLASH_ADDRESS)
#undef EXTFLASH_SIZE
#define EXTFLASH_SIZE (PM_EXTERNAL_FLASH_SIZE)
#else
#define EXTFLASH_ADDRESS (EXTFLASH_BASE_ADDR)
#endif /* USE_PARTITION_MANAGER && PM_EXTERNAL_FLASH_ADDRESS */

MEMORY
{
     EXTFLASH (wx) : ORIGIN = EXTFLASH_ADDRESS, LENGTH = EXTFLASH_SIZE
}

#endif /* CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP || CONFIG_BOARD_NRF52840DK_NRF52840 */

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
