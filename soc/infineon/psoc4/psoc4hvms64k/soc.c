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

/* HVMS SRAM has ECC that faults on reads of uninitialised memory; write the
 * whole SRAM once before the C runtime uses it so valid ECC is generated.
 */
void soc_reset_hook(void)
{
	register unsigned r0 __asm("r0") = DT_REG_ADDR(DT_CHOSEN(zephyr_sram));
	register unsigned r1 __asm("r1") =
		DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) + DT_REG_SIZE(DT_CHOSEN(zephyr_sram));

	for (; r0 < r1; r0 += 4) {
		*(unsigned int *)r0 = 0;
	}
}

/* Minimal early initialization for PSOC4 HVMS 64K */
void soc_early_init_hook(void)
{
	/* Initializes the system */
	SystemInit();
}
