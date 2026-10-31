/*
 * Copyright (c) 2017, Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

void soc_early_reset_hook(void)
{
	/*
	 * WDT_A comes out of reset armed, clocked from SMCLK with a 2^15
	 * count interval: about 10.9 ms at the 3 MHz DCO default.
	 * SystemInit() stops it, but only after BSS zeroing and data copy,
	 * which take longer than that on images with a large BSS. Stop it
	 * here, before any RAM initialization, so it cannot fire mid boot.
	 */
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;
}

void soc_early_init_hook(void)
{

	SystemInit();
}
