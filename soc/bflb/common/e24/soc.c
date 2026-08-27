/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MSTATUS_IEN     (1UL << 3)

void soc_prep_hook(void)
{
	int tmp;
	/* Disable IRQs for init to avoid crash, idle thread will re-enable */
	__asm__ volatile ("csrrc %0, mstatus, %1"
			  : "=r" (tmp)
			  : "rK" (MSTATUS_IEN)
			  : "memory");
}
