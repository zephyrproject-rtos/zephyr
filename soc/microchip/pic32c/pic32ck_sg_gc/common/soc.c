/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file soc.c
 * @brief Microchip PIC32CK SG/GC family initialization code
 */

#include <zephyr/devicetree.h>

#define SRAM0_NODE DT_CHOSEN(zephyr_sram)
#define SRAM0_BASE DT_REG_ADDR(SRAM0_NODE)
#define SRAM0_SIZE DT_REG_SIZE(SRAM0_NODE)

/**
 * @brief Early Reset hook to run SoC-specific initialization.
 *
 * This function performs 32-bit writes to clear the entire SRAM very early at reset.
 *
 * After reset, SRAM content (data + ECC bits) is random and ECC is enabled by default.
 * Any 8-bit or 16-bit write may trigger single or double ECC errors due to the internal
 * read-modify-write of 32-bit words. Therefore, the SRAM must be initialized before use
 * to ensure ECC correctness.
 *
 * Implementing it naked with assembly code guarantees no stack access happens during
 * the initialization.
 */
__attribute__((naked)) void soc_early_reset_hook(void)
{
	/* sram initialization */
	__asm__ volatile(
		"	ldr  r0, =%c[start]\n"
		"	ldr  r1, =%c[end]\n"
		"	movs r2, #0\n"
		"loop:\n"
		"	stmia r0!, {r2}\n"
		"	cmp  r0, r1\n"
		"	blo  loop\n"
		"	bx   lr\n"
		:: [start] "i" (SRAM0_BASE), [end] "i" (SRAM0_BASE + SRAM0_SIZE));
}
