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

#include <autoconf.h>
#include <linker/sections.h>
#include <devicetree.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

/*
 * Add another fake portion of FLASH to simulate a secondary or external FLASH
 * that we can do XIP from.
 */
#define EXTFLASH_ADDR	0x5000
#define EXTFLASH_SIZE	(CONFIG_FLASH_SIZE * 1K - EXTFLASH_ADDR)

MEMORY
{
     EXTFLASH (wx) : ORIGIN = 0x5000, LENGTH = EXTFLASH_SIZE
}

#include <arch/arm/aarch32/cortex_m/scripts/linker.ld>
