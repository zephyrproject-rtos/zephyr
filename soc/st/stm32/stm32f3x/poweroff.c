/*
 * Copyright (c) 2025 Enes Albay <albayenes@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stm32_common.h>
#include <stm32_ll_pwr.h>
#include <zephyr/sys/poweroff.h>

void z_sys_poweroff(void)
{
	LL_PWR_ClearFlag_SB();
	LL_PWR_ClearFlag_WU();

	LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
	LL_DBGMCU_DisableDBGStandbyMode();

	stm32_enter_poweroff();
}
