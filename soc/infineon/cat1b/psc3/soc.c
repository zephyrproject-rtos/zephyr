/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <soc.h>
#include <ifx_cycfg_init.h>

void soc_early_init_hook(void)
{
	ifx_cycfg_init();
}

#if defined(CONFIG_CLOCK_CONTROL_IFX_SRSS_CONFIGURE)
BUILD_ASSERT(CONFIG_CLOCK_CONTROL_INIT_PRIORITY <
	     CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/* Refresh the PDL clock variables; remove once all drivers are native. */
static int psc3_core_clock_update(void)
{
	SystemCoreClockUpdate();

	return 0;
}

SYS_INIT(psc3_core_clock_update, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* CONFIG_CLOCK_CONTROL_IFX_SRSS_CONFIGURE */
