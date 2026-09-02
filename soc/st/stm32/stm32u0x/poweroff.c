/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#include <stm32_bitops.h>
#include <stm32_common.h>
#include <stm32_ll_pwr.h>

void z_sys_poweroff(void)
{
	if (IS_ENABLED(CONFIG_STM32_WKUP_PINS)) {
		LL_PWR_EnablePUPDCfg();
	}

	/* No LL function to clear all flags at once; do it ourselves */
	stm32_reg_set_bits(&PWR->SCR,
			   PWR_SCR_CWUF1 | PWR_SCR_CWUF2 | PWR_SCR_CWUF3 |
			   PWR_SCR_CWUF4 | PWR_SCR_CWUF5 | PWR_SCR_CWUF7);
	LL_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);

	stm32_enter_poweroff();
}
