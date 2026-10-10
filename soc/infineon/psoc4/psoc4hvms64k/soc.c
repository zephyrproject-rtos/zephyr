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

/* Reset causes that mean the supply was lost, so the SRAM content and ECC are not valid */
#define SRAM_LOST_CAUSE                                                                            \
	(CY_SYSLIB_RESET_PORVDDD | CY_SYSLIB_RESET_BODVDDD | CY_SYSLIB_RESET_BODVCCD |             \
	 CY_SYSLIB_RESET_OVDVDDD | CY_SYSLIB_RESET_OVDVCCD | CY_SYSLIB_RESET_BODHVSS)

/*
 * HVMS SRAM has ECC that faults on reads of uninitialised memory, so write the whole SRAM
 * to generate valid ECC. Skip it when the reset left the SRAM intact (e.g. a WDT reset),
 * so that .noinit data survives.
 * Naked pure asm: no stack access is allowed before the SRAM is initialized.
 */
__attribute__((naked)) void soc_early_reset_hook(void)
{
	__asm__ volatile(
		"	ldr  r0, =%c[cause]\n"
		"	ldr  r0, [r0]\n"
		"	cmp  r0, #0\n"
		"	beq  1f\n"
		"	ldr  r1, =%c[lost]\n"
		"	tst  r0, r1\n"
		"	beq  3f\n"
		"1:\n"
		"	ldr  r0, =%c[start]\n"
		"	ldr  r1, =%c[end]\n"
		"	movs r2, #0\n"
		"2:\n"
		"	stmia r0!, {r2}\n"
		"	cmp  r0, r1\n"
		"	blo  2b\n"
		"3:\n"
		"	bx   lr\n"
		:: [cause] "i" ((uintptr_t)&SRSS_RES_CAUSE),
		   [lost] "i" (SRAM_LOST_CAUSE),
		   [start] "i" (SRAM0_BASE),
		   [end] "i" (SRAM0_BASE + SRAM0_SIZE));
}

/* Minimal early initialization for PSOC4 HVMS 64K */
void soc_early_init_hook(void)
{
	/* Initializes the system */
	SystemInit();

	/* RW1C: the causes are sticky, consume the supply-loss ones so later resets are judged alone */
	SRSS_RES_CAUSE = SRAM_LOST_CAUSE;
}
