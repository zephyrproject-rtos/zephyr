/*
 * Copyright (c) 2025 Oleh Kravchenko <oleh@kaa.org.ua>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stm32_common.h>
#include <stm32_ll_pwr.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>

void z_sys_poweroff(void)
{
	LL_PWR_ClearFlag_SB();
	LL_PWR_ClearFlag_WU();

	LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);

	stm32_enter_poweroff();
}
