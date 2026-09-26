/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <cy_sysint.h>
#include <system_cat2.h> /* PSoC4 system init header from PDL */
#include <cy_pdl.h>

#define SRAM0_NODE DT_CHOSEN(zephyr_sram)
#define SRAM0_BASE DT_REG_ADDR(SRAM0_NODE)
#define SRAM0_SIZE DT_REG_SIZE(SRAM0_NODE)

/*
 * HVMS SRAM has ECC that faults on reads of uninitialised memory; write the
 * whole SRAM once before the C runtime uses it so valid ECC is generated.
 * Implemented naked in pure asm so no stack access happens before SRAM is
 * initialized (a register-variable C loop is not safe with optimizations off).
 */
__attribute__((naked)) void soc_early_reset_hook(void)
{
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

/* Minimal early initialization for PSOC4 HVMS 64K */
void soc_early_init_hook(void)
{
	/* Initializes the system */
	SystemInit();
}
