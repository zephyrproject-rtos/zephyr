/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file soc.c
 * @brief Microchip PIC32CM JH family initialization code
 */

#include <zephyr/devicetree.h>

#define SRAM0_NODE DT_NODELABEL(sram0)
#define SRAM0_BASE DT_REG_ADDR(SRAM0_NODE)
#define SRAM0_SIZE DT_REG_SIZE(SRAM0_NODE)

/**
 * @brief Initialize (clear) the SRAM region defined by DT node sram0.
 */
static void ram_initialize(void)
{
	volatile uint32_t *ram_start = (uint32_t *)SRAM0_BASE;
	volatile uint32_t *ram_end = (uint32_t *)(SRAM0_BASE + SRAM0_SIZE);

	for (; ram_start < ram_end; ++ram_start) {
		*ram_start = 0U;
	}
}

/**
 * @brief Reset hook to run SoC-specific initialization.
 *
 * This is invoked very early at reset.
 */
void soc_reset_hook(void)
{
	ram_initialize();
}
