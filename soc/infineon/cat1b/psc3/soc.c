/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ifx_cycfg_init.h>
#include <system_cat1b.h>
#include <zephyr/sys/util.h>

void soc_early_init_hook(void)
{
	ifx_cycfg_init();

	if (IS_ENABLED(CONFIG_SOC_DIE_PSC3M6)) {
		SystemCoreClockUpdate();
	}
}
