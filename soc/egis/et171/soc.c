/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <et171_aosmu.h>
#include <et171_otp.h>

void soc_early_init_hook(void)
{
#if CONFIG_XIP && CONFIG_ICACHE
	/* Since caching is enabled before z_data_copy(), RAM functions may still be cached
	 * in the d-cache instead of being written to SRAM. In this case, the i-cache will
	 * fetch the wrong content from SRAM. Thus, using "fence.i" to fix it.
	 */
	__asm__ volatile("fence.i" ::: "memory");
#endif

	/* Load and perform analog trimming value from OTP */
	et171_otp_ll_load_analog_config();

	/* AHB ~200Mhz / 3 = 66MHz AHB , ~66Mhz / 2 / 2 = 18Mhz APB */
	WRITE_AOSMU_REG(AOSMU_REG_CLK_SRC,
		    SMU_CLK_SRC_250M | SMU_CLK_SRC_AHB_DIV_3 | SMU_CLK_SRC_APB_DIV_2);

	/* Workaround WFI wakeup issue. AE350_I2C->INTEN = 1;       */
	sys_write32(1, 0xF0A00014U);
}
