/*
 * Copyright (c) 2025 Jacky Lee <jacky.lee@egistec.com>
 * Copyright 2025 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>

/* redirect from soc_early_init_hook() */
int et171_soc_early_init_hook(void)
{
	/* AHB ~200Mhz / 3 = 66MHz AHB , ~66Mhz / 2 / 2 = 18Mhz APB */
	sys_write32(0x01 | 0x20 | 0x02, 0xF0100004U);

	/* Workaround WFI wakeup issue. AE350_I2C->INTEN = 1;       */
	sys_write32(1, 0xF0A00014U);

	return 0;
}
