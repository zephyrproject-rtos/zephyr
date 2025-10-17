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

#define SRAM0_NODE DT_CHOSEN(zephyr_sram)
#define SRAM0_BASE DT_REG_ADDR(SRAM0_NODE)
#define SRAM0_SIZE DT_REG_SIZE(SRAM0_NODE)

/**
 * @brief Initialize (clear) the SRAM region defined by DT node sram0.
 *
 * After reset, SRAM content (data + ECC bits) is random and ECC is enabled by default.
 * Any 8-bit or 16-bit write may trigger single or double ECC errors due to the internal
 * read-modify-write of 32-bit words. Therefore, the SRAM must be initialized before use
 * to ensure ECC correctness.
 *
 * This function performs 32-bit writes to clear the entire SRAM safely.
 * It is intended for reuse across all SoCs that feature ECC-enabled SRAM.
 *
 * @note
 * This SRAM initialization is specific to device families with ECC-enabled SRAM
 * and should not be included in the generic architecture configuration.
 * The function is best invoked from the soc_reset_hook of the relevant device family,
 * which is called before stack initialization and before transitioning to C code.
 */
static void soc_mchp_sram_ecc_initialization(void)
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
	soc_mchp_sram_ecc_initialization();
}
