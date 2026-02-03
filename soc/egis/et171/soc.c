/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/init.h>
 #include <zephyr/kernel.h>

/* ================================================================== ET171 HAL
 * et171 core
 */
#include <et171_hal/et171.h>
#include <et171_hal/et171_hal_smu.h>
#include <et171_hal/et171_hal_otp.h>

void soc_early_init_hook(void)
{
#if CONFIG_XIP && CONFIG_ICACHE
	/* Since caching is enabled before z_data_copy(), RAM functions may still be cached
	 * in the d-cache instead of being written to SRAM. In this case, the i-cache will
	 * fetch the wrong content from SRAM. Thus, using "fence.i" to fix it.
	 */
	__asm__ volatile("fence.i" ::: "memory");
#endif

	/* Load Analog triming value from OTP */
	HAL_OTP_LoadAnalogConfig();

	/* Load Root clock triming value from OTP */
	HAL_OTP_GetRootClock();

	/* AHB ~200Mhz / 3 = 66MHz AHB , ~66Mhz / 2 / 2 = 18Mhz APB */
	HAL_SMU_SetRootClock(ROOT_CLK_250M, ROOT_CLK_DIV_3, APB_CLK_DIV_2);

	/* Workaround WFI wakeup issue. AE350_I2C->INTEN = 1;       */
	sys_write32(1, 0xF0A00014U);

#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
	extern unsigned int z_clock_hw_cycles_per_sec;

	z_clock_hw_cycles_per_sec = HAL_SMU_GetClock(SMU_CLK_APB);
#endif
}
