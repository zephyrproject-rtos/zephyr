/*
 * Copyright (c) 2025 Texas Instruments
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>

#include <soc.h>
#include "soc_sysctl.h"

#define MSPM_SYSCTL_BASE DT_REG_ADDR(DT_NODELABEL(sysctl))

/* The BL/BX instructions themselves will be worth 1-3 overhead cycles depending on optimisation */
void __noinline msp_delay_peripheral_startup(void)
{
#define NOP(i, ...)    "nop;"
#define NOP_N_TIMES(n) LISTIFY(n, NOP, ())

	__asm__ volatile(NOP_N_TIMES(CONFIG_MSPM0_PERIPH_STARTUP_DELAY));
}

void soc_early_init_hook(void)
{
	/* Low Power Mode is configured to be SLEEP0 */
	sys_write32(SYSCTL_BORTHRESHOLD_LEVEL_0, MSPM_SYSCTL_BASE + SYSCTL_BORTHRESHOLD_OFFSET);
}
