/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include "reg_protection.h"
#if CONFIG_HAS_RENESAS_RX_RDP
#include "r_bsp_cpu.h"
#endif

#define PRCR_KEY    (0xA500)
#define SYSTEM_PRCR (*(volatile uint16_t *)0x000803FE)

#ifndef CONFIG_HAS_RENESAS_RX_RDP
static volatile uint16_t protect_counters[RENESAS_RX_REG_PROTECT_TOTAL_ITEMS];

static const uint16_t prcr_masks[RENESAS_RX_REG_PROTECT_TOTAL_ITEMS] = {
	0x0001, /* PRC0. */
	0x0002, /* PRC1. */
	0x0004, /* PRC2. */
	0x0008, /* PRC3. */
};


void renesas_rx_register_protect_open(void)
{
	int i;

	for (i = 0; i < RENESAS_RX_REG_PROTECT_TOTAL_ITEMS; i++) {
		protect_counters[i] = 0;
	}
}
#endif

void renesas_rx_register_protect_enable(renesas_rx_reg_protect_t regs_to_protect)
{
#if CONFIG_HAS_RENESAS_RX_RDP
	R_BSP_RegisterProtectEnable(regs_to_protect);
#else
	int key;

	/*
	 * Set IPL to the maximum value to disable all interrupts,
	 * so the scheduler can not be scheduled in critical region.
	 * Note: Please set this macro more than IPR for other FIT module interrupts.
	 */
	key = irq_lock();

	/* Is it safe to disable write access? */
	if (0 != protect_counters[regs_to_protect]) {
		/* Decrement the protect counter */
		protect_counters[regs_to_protect]--;
	}

	/* Is it safe to disable write access? */
	if (0 == protect_counters[regs_to_protect]) {
		/*
		 * Enable protection using PRCR register.
		 * When writing to the PRCR register the upper 8-bits must be the correct key. Set
		 * lower bits to 0 to disable writes. b15:b8 PRKEY - Write 0xA5 to upper byte to
		 * enable writing to lower byte b7:b4  Reserved (set to 0) b3     PRC3  - Please
		 * check the user's manual. b2     PRC2  - Please check the user's manual. b1 PRC1
		 * - Please check the user's manual. b0     PRC0  - Please check the user's manual.
		 */
		SYSTEM_PRCR = (uint16_t)((SYSTEM_PRCR | PRCR_KEY) & (~prcr_masks[regs_to_protect]));
	}

	/* Restore the IPL. */
	irq_unlock(key);
#endif
}

void renesas_rx_register_protect_disable(renesas_rx_reg_protect_t regs_to_unprotect)
{
#if CONFIG_HAS_RENESAS_RX_RDP
	R_BSP_RegisterProtectDisable(regs_to_unprotect);
#else
	int key;

	/*
	 * Set IPL to the maximum value to disable all interrupts,
	 * so the scheduler cannot be scheduled in the critical region.
	 * Note: Please set this macro more than IPR for other FIT module interrupts.
	 */
	key = irq_lock();

	/* Is it safe to enable write access? */
	if (0 == protect_counters[regs_to_unprotect]) {
		/* Disable protection using PRCR register */
		SYSTEM_PRCR = (uint16_t)((SYSTEM_PRCR | PRCR_KEY) | prcr_masks[regs_to_unprotect]);
	}

	/* Increment the protect counter */
	protect_counters[regs_to_unprotect]++;

	/* Restore the IPL */
	irq_unlock(key);
#endif
} /* End of function renesas_register_protect_disable() */
