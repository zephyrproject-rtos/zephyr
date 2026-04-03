/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/init.h>
 #include <zephyr/kernel.h>

void soc_early_init_hook(void)
{
#if CONFIG_XIP && CONFIG_ICACHE
	/* Since caching is enabled before z_data_copy(), RAM functions may still be cached
	 * in the d-cache instead of being written to SRAM. In this case, the i-cache will
	 * fetch the wrong content from SRAM. Thus, using "fence.i" to fix it.
	 */
	__asm__ volatile("fence.i" ::: "memory");
#endif

	/* AHB ~200Mhz / 3 = 66MHz AHB , ~66Mhz / 2 / 2 = 18Mhz APB */
	sys_write32(0x01 | 0x20 | 0x02, 0xF0100004U);

	/* Workaround WFI wakeup issue. AE350_I2C->INTEN = 1;       */
	sys_write32(1, 0xF0A00014U);
}
