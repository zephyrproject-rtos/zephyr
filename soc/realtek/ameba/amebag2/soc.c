/*
 * Copyright (c) 2025 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>

extern void z_arm_reset(void);

struct ram_start_function {
	void (*ram_start_func)(void);
	void (*ram_wakeup_func)(void);
	uint32_t *vector_ns;
};

/* Declared in the Ameba SoC HAL */
IMAGE2_ENTRY_SECTION
struct ram_start_function img2_entry_fun0 = {z_arm_reset, NULL, NULL};

static void soc_ameba_osc131_enable(void)
{
	if (RCC_PeriphClockEnableChk(APBPeriph_RTC_CLOCK)) {
		return;
	}

	/* Wait for RTC detection interrupt, indicating OSC131 is ready. */
	while (RTC_GetDetintr() == 0) {
	}
	RTC_ClearDetINT();

	SDM32K_Enable();
	RCC_PeriphClockCmd(APBPeriph_NULL, APBPeriph_RTC_CLOCK, ENABLE);
	RTC_ClkSource_Select(SDM32K);

	if ((Get_OSC131_STATE() & RTC_BIT_FIRST_PON) == 0) {
		Set_OSC131_STATE(Get_OSC131_STATE() | RTC_BIT_FIRST_PON);

		/* CKE_RTC must be enabled before OSC131 calibration */
		if (SYSCFG_CHIPType_Get() == CHIP_TYPE_ASIC_POSTSIM) {
			OSC131K_Calibration(30000); /* PPM=30000=3% */ /* 7.5ms */
		}
	}
}

void soc_early_init_hook(void)
{
	/*
	 * Cache is enabled by default at reset, disable it before
	 * sys_cache*-functions can enable them.
	 */
	Cache_Enable(DISABLE);
	sys_cache_instr_enable();
	sys_cache_data_enable();
	memset((void *)__rom_bss_start_ns__, 0, (__rom_bss_end_ns__ - __rom_bss_start_ns__));

	RBSS_UDELAY_DIV = 5;

	XTAL_INIT();
	if (SYSCFG_CHIPType_Get() == CHIP_TYPE_ASIC_POSTSIM) {
		OSC4M_Init();
		OSC4M_Calibration(30000);
	}

	soc_ameba_osc131_enable();
}
