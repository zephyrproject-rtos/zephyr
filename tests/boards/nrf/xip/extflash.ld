/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#define EXTFLASH_NODE DT_NODELABEL(dut)
#define EXTFLASH_SIZE DT_PROP_OR(EXTFLASH_NODE, size_in_bytes, \
				 DT_PROP(EXTFLASH_NODE, size) / 8)

#if defined(CONFIG_NORDIC_QSPI_NOR) && defined(CONFIG_SOC_NRF5340_CPUAPP)

#define EXTFLASH_ADDR 0x10000000

#elif defined(CONFIG_FLASH_MSPI_NOR) && defined(CONFIG_SOC_NRF54H20_CPUAPP)

#define EXTFLASH_ADDR 0x60000000

#else

#error Unsupported target

#endif

MEMORY
{
	EXTFLASH (rx) : ORIGIN = EXTFLASH_ADDR, LENGTH = EXTFLASH_SIZE
}

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
