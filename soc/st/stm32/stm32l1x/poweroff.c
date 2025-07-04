/*
 * Copyright (c) 2025 Oleh Kravchenko <oleh@kaa.org.ua>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stm32l1xx_ll_cortex.h>
#include <stm32l1xx_ll_pwr.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>

void z_sys_poweroff(void)
{
	LL_PWR_ClearFlag_SB();
	LL_PWR_ClearFlag_WU();

	LL_LPM_DisableEventOnPend();
	LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
	LL_LPM_EnableDeepSleep();

	k_cpu_idle();

	/* power consumption here is ~2.2uA on stm32l1_disco board */

	CODE_UNREACHABLE;
}
