/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Cortex-M platforms.
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#if defined(CONFIG_NORDIC_QSPI_NOR) && defined(CONFIG_SOC_NRF5340_CPUAPP)

/* On nRF5340, external flash is mapped in XIP region at 0x1000_0000. */

#define EXTFLASH_NODE	DT_INST(0, nordic_qspi_nor)
#define EXTFLASH_ADDR	0x10000000
#define EXTFLASH_SIZE	DT_PROP_OR(EXTFLASH_NODE, size_in_bytes, \
				   DT_PROP(EXTFLASH_NODE, size) / 8)

#elif defined(CONFIG_STM32_MEMMAP) && DT_NODE_EXISTS(DT_INST(0, st_stm32_ospi_nor))
/* On stm32 OSPI, external flash is mapped in XIP region at address given by the reg property. */

#define EXTFLASH_NODE	DT_INST(0, st_stm32_ospi_nor)
#define EXTFLASH_ADDR	DT_REG_ADDR(DT_INST(0, st_stm32_ospi_nor))
#define EXTFLASH_SIZE	DT_REG_SIZE(DT_INST(0, st_stm32_ospi_nor))

#elif defined(CONFIG_STM32_MEMMAP) && DT_NODE_EXISTS(DT_INST(0, st_stm32_qspi_nor))
/* On stm32 QSPI, external flash is mapped in XIP region at address given by the reg property. */

#define EXTFLASH_NODE	DT_INST(0, st_stm32_qspi_nor)
#define EXTFLASH_ADDR	DT_REG_ADDR(DT_INST(0, st_stm32_qspi_nor))
#define EXTFLASH_SIZE	DT_REG_SIZE(DT_INST(0, st_stm32_qspi_nor))

#elif defined(CONFIG_STM32_MEMMAP) && DT_NODE_EXISTS(DT_INST(0, st_stm32_xspi_nor))
/* On stm32 XSPI, external flash is mapped in XIP region at address given by the reg property. */

#define EXTFLASH_NODE	DT_INST(0, st_stm32_xspi_nor)
#define EXTFLASH_ADDR	DT_REG_ADDR(DT_INST(0, st_stm32_xspi_nor))
#define EXTFLASH_SIZE	DT_REG_SIZE(DT_INST(0, st_stm32_xspi_nor))

#elif defined(CONFIG_FLASH_MSPI_NOR) && defined(CONFIG_SOC_NRF54H20_CPUAPP)

#define EXTFLASH_NODE	DT_INST(0, jedec_mspi_nor)
#define EXTFLASH_ADDR	0x60000000
#define EXTFLASH_SIZE	DT_PROP_OR(EXTFLASH_NODE, size_in_bytes, \
				   DT_PROP(EXTFLASH_NODE, size) / 8)

#else

/*
 * Add another fake portion of FLASH to simulate a secondary or external FLASH
 * that we can do XIP from.
 */
#define EXTFLASH_ADDR	0x7000
#define EXTFLASH_SIZE	(CONFIG_FLASH_SIZE * 1K - EXTFLASH_ADDR)

#endif

MEMORY
{
	EXTFLASH (rx) : ORIGIN = EXTFLASH_ADDR, LENGTH = EXTFLASH_SIZE
}

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
