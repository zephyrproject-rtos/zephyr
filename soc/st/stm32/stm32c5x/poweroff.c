/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stm32_common.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_system.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

void z_sys_poweroff(void)
{
	LL_PWR_ClearFlag_WU(LL_PWR_WAKEUP_PIN_ALL);

	LL_PWR_SetPowerMode(LL_PWR_STANDBY_MODE);
	LL_DBGMCU_DisableDBGStandbyMode();

	stm32_enter_poweroff();
}
