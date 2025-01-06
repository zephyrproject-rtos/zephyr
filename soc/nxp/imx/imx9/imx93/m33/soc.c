/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

#ifdef CONFIG_SOC_EARLY_INIT_HOOK
void soc_early_init_hook(void)
{
	/* Configure secure access to pin registers */
	GPIO1->PCNS = 0x0;
	GPIO2->PCNS = 0x0;
	GPIO3->PCNS = 0x0;
	GPIO4->PCNS = 0x0;
}
#endif
