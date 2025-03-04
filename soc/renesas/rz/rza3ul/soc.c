/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Renesas RZ/A3UL Group
 */

#include <zephyr/init.h>
#include "soc.h"

/** System Clock Frequency (Core Clock) */
/** TODO: bsp_clock_cfg.h should get its values from device tree? */

uint32_t SystemCoreClock;

void soc_early_init_hook(void)
{
	/* Configure system clocks. */
	bsp_clock_init();

	/* InitFialize SystemCoreClock variable. */
	SystemCoreClockUpdate();
}
